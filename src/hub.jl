#!/usr/bin/env julia

using ArgParse
using HttpServer
using LibSerialPort
using WebSockets
using ZMQ
import JSON

include("util/conversions.jl")
include("util/find.jl")
include("util/math.jl")

# Create an ArgParseSettings object and populate it with an argument table
arg_parse_settings = ArgParseSettings(
    description = """
    hub.jl: Connector between vehicle controller and navigation computer.
    Handles communication over USB serial connection to controller and serves
    bidirectional WebSocket messages to a monitoring/control webpage.
    """)
@add_arg_table arg_parse_settings begin
    "-a", "--serial-port-address"
        help = "Specify serial port address"
        dest_name = "port_address"
        arg_type = AbstractString
        default = "/dev/cu.usbmodem1219331"
    "-b", "--baud"
        help = "Specify serial data rate"
        arg_type = Int
        default = 115200
        range_tester = x -> x > 0
    "-m", "--monitor"
        help = "Serve data to monitoring page (default is terminal output)"
        action = :store_true
    "-o", "--output-file"
        help = "Save incoming serial data to output text file [TODO]"
        dest_name = "file"
        arg_type = AbstractString
        default = "out.txt"
    "-p", "--tcp-port"
        help = "If monitoring page is requested (-m), serve page over the specified TCP port"
        dest_name = "tcp_port"
        default = 8000
    "-t", "--test"
        help = "Test this program with randomly-generated data instead of serial input"
        action = :store_true
end

"""
Callback to take HTTP requests and return responses.
It is meant to be passed to an HttpHandler constructor.
"""
function http_responder(request::Request, response::Response)

    files = ["monitoring/vehicle/index.html"; find("monitoring/vehicle", ".js")]

    for file in files
        if startswith("/$file", request.resource)
            println("serving /", file)
            response = open(readstring, file)
            return response
        end
    end

    response.status = 404
    response.data  = "<h3>$(request.resource[2:end])</h3> <h3>not found.</h3>"
    return response
end

function wsserver(http_responder, callback, source, fields, buffered_fields, buffer_length)

    wsh = WebSocketHandler() do req, client
        while true
            # Read string from client, decode, and parse to Dict
            msg = JSON.parse(String(read(client)))
            if haskey(msg, "text") && msg["text"] == "ready"
                println("\nReceived update from client: ready")
                callback(source, fields, client, buffered_fields, buffer_length)
            end
        end
    end

    server = Server(HttpHandler(http_responder), wsh)
    return server
end

"""
Write JSON-formatted message to websocket client.
Recipients expect the schema defined here, so modify with care.
Data can be an array, vector, dictionary, or any other type that is convertible
to JSON by the JSON package.
"""
function send_json(name::AbstractString, data::Any, client::WebSockets.WebSocket)
    msg = Dict{AbstractString, Any}()
    msg["type"] = name
    msg["data"] = data
    msg["timestamp"] = time()
    write(client, JSON.json(msg))
end

get_message(socket::ZMQ.Socket) = unsafe_string(ZMQ.recv(socket))
get_message(sp::SerialPort) = readuntil(sp, '\n', 100)

function send_output(d, destination::String)
    msg = Dict{AbstractString, Any}()
    msg["data"] = d
    json_msg = JSON.json(msg)

    if destination == "console"
        println(json_msg)
        # [Base.print("$k:$(d[k]) ") for k in keys(d)]
        println()
    else
        # TODO File output - use destination as a file name
        open(destination, "w") do io
            write(io, data)
        end
    end
end

function send_output(d, client::WebSockets.WebSocket)
    send_json("vehicle-state", d, client)
end

"""
    source: data source (SerialPort, ZMQ.Socket)
    fields: array of keys in message
    destination: message recipient (WebSocket client, output file)
    buffered_fields: subset of fields to collect into an array
    buffer_length: number of messages to use for buffering

    Example:
        If fields = ["a", "b", "c"],
        buffered_fields = ["c"],
        and buffer_length = 3,
        then the resulting JSON output would be structured like this:
            {a: [1.23], b: [2.34], c: [1.01, 2.56, 3.23]}
        where the values for a and b are from the most recent message.
"""
function process_stream(source, fields, destination, buffered_fields=[], buffer_length=1)
    output = Dict{String, Vector{AbstractFloat}}()
    for field in fields
        output[field] = []
    end
    while true
        msg = get_message(source)
        d = csv2dict(msg, fields)
        if keys_ok(d, fields)

            # For the first buffer_length-1 input messages, only fill output
            # with buffered_fields. On final input message, fill all fields.
            if length(output[buffered_fields[1]]) < buffer_length - 1
                for field in buffered_fields
                    push!(output[field], d[field])
                end
            else
                # Get quaternion from dict, then Euler/Tait-Bryan angles.
                q = qnorm(d)
                roll, pitch, yaw = to_euler(q)

                roll = round(roll*180/pi, 2)
                pitch = round(pitch*180/pi, 2)
                yaw = round(yaw*180/pi, 2)

                output["roll"] = [roll]
                output["pitch"] = [pitch]
                output["yaw"] = [yaw]

                for field in fields
                    push!(output[field], d[field])
                end
                send_output(output, destination)
                for field in fields
                    output[field] = []
                end
            end

        else
            println("skip $msg")
        end
    end
end

function main()

    # Parse built-in ARGS array according to arg_parse_settings. Returns a Dict.
    args = parse_args(ARGS, arg_parse_settings)

    println("\nConfiguration parameters:")
    [println("  $key  =>  $(repr(val))") for (key, val) in args]

    # Labels for various quantities in incoming data stream
    fields = ["t","AMGS","qw","qx","qy","qz","sa","odo","r","b"]
    buffered_fields = ["r", "b"]
    buffer_length = 5

    # Use fake-data-publisher.jl as data source (test mode) or serial port to
    # vehicle controller module, depending on args
    if args["test"]
        context = Context()
        socket = Socket(context, SUB)

        # Connect as a subscriber
        println("Connecting to fake data publisher at localhost:5556")
        ZMQ.connect(socket, "tcp://localhost:5556")

        # Subscribe to published messages, filtering on provided string
        ZMQ.set_subscribe(socket, "t:")
        data_source = socket
    else
        data_source = try
            open(args["port_address"], args["baud"])
        catch
            println("Could not open serial port at ", args["port_address"])
            quit()
        end
    end

    if args["monitor"]
        svr = wsserver(http_responder, process_stream, data_source, fields, buffered_fields, buffer_length)

        println("Serving http://localhost:8000/monitoring/vehicle/index.html")
        HttpServer.run(svr, args["tcp_port"])
    else
        process_stream(data_source, fields, "console", buffered_fields, buffer_length)
    end
end

main()