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

function wsserver(http_responder, callback, source, fields)

    wsh = WebSocketHandler() do req, client
        while true
            # Read string from client, decode, and parse to Dict
            msg = JSON.parse(String(read(client)))
            if haskey(msg, "text") && msg["text"] == "ready"
                println("\nReceived update from client: ready")
                callback(source, fields, client)
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
    if destination == "console"
        [Base.print("$k:$(d[k]) ") for k in keys(d)]
        println()
    else
        # TODO File output - use destination as a file name
        open(destination, "w") do io
            write(io, data)
        end
    end
end

function send_output(d, client::WebSockets.WebSocket)
    send_json("quaternions", d, client)
end

function process_stream(source, fields, destination)
    while true
        msg = get_message(source)
        d = csv2dict(msg, fields)
        if keys_ok(d, fields)
            # Get quaternion from dict
            q = qnorm(d)

            # Add Euler/Tait-Bryan angles
            d["roll"], d["pitch"], d["yaw"] = to_euler(q)

            send_output(d, destination)
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
        svr = wsserver(http_responder, process_stream, data_source, fields)

        println("Serving http://localhost:8000/monitoring/vehicle/index.html")
        HttpServer.run(svr, args["tcp_port"])
    else
        process_stream(data_source, fields, "console")
    end
end

main()