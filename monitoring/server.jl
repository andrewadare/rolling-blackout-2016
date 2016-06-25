using HttpServer
using WebSockets
using LibSerialPort
import JSON

include("server_utils.jl")

# Abstract type representing a microcontroller node in the network
abstract McuNode

# McuNode singleton subtypes (for parametrization and dispatch)
type LidarNode <: McuNode end
type SteerNode <: McuNode end
type AngleNode <: McuNode end

# Node metadata for processing streams from MCU nodes
immutable NodeMD{N <: McuNode}
    sp::SerialPort
    keys::Array{AbstractString}
    message_name::AbstractString
end

function send_command(node)
    write(node.sp, "status 0\n")
end

"""
Methods to process streams from microcontrollers. Take a line of text, do any
needed processing, and return a dict suitable for conversion to JSON.
"""
function read_reply(md::NodeMD{SteerNode})
    line = readuntil(md.sp, '\n')
    d = csv2dict(line, md.keys)
    if keys_ok(d, md.keys)
        # TODO: map adc to degrees here, using limits from steering.ino?
        # int maxLeftADC = 577; int maxRightADC = 254;
    else
        println("Missing key(s) - skipping: ", strip(line))
    end
    return d
end

function read_reply(md::NodeMD{AngleNode})
    line = readuntil(md.sp, '\n')
    d = csv2dict(line, md.keys)
    if keys_ok(d, md.keys)

        # Get quaternion from dict
        q = qnorm(d)

        # Add Euler/Tait-Bryan angles
        d["roll"], d["pitch"], d["yaw"] = to_euler(q)
    else
        println("Missing key(s) - skipping")
    end
    # Useful for debugging:
    # println(strip(line))
    return d
end

function read_reply(md::NodeMD{LidarNode})
    # TODO
end

"""
Read streams from all MCUs in the list, process and format to JSON messages, then
send to WebSocket client.
"""
function process_streams(client::WebSockets.WebSocket, mcu_nodes)
    while true
        map(send_command, mcu_nodes)

        sleep(0.05)

        for node in mcu_nodes
            message_dict = read_reply(node)
            if keys_ok(message_dict, node.keys)
                send_json(node.message_name, message_dict, client)
            end
        end
    end
end

function readuntil(sp::SerialPort, delim::Char)
    result = Char[]
    while Int(nb_available(sp)) > 0
        byte = Base.readbytes(sp, 1)[1]
        push!(result, byte)
        byte == delim && break
    end
    return join(result)
end


"""
Send data to stdout instead of browser. Useful for debugging.
"""
function process_streams(mcu_nodes)
    while true
        map(send_command, mcu_nodes)

        # TODO: find a way to read back when reply is ready
        sleep(0.04)

        for node in mcu_nodes
            d = read_reply(node)
            keys_ok(d, node.keys) && [Base.print("$k:$(d[k]) ") for k in keys(d)]
            println()
        end
    end
    return nothing
end

"""
Read delimited data lines streaming from serial port and pass them on as
JSON-formatted WebSocket messages.
"""
function send_sensor_data(client::WebSockets.WebSocket, sp::SerialPort, csvkeys)
    while true
        line = readline(sp)
        d = csv2dict(line, csvkeys)
        if keys_ok(d, csvkeys)

            # Get quaternion from dict
            q = qnorm(d)

            # Add Euler/Tait-Bryan angles
            d["roll"], d["pitch"], d["yaw"] = to_euler(q)

            send_json("quaternions", d, client)
        else
            println("Missing key(s) - skipping")
        end
        # Useful for debugging:
        # println(strip(line))
    end
end

"""
Create and configure a SerialPort object
"""
function open_serial_port(port_address, speed)
    sp = SerialPort(port_address)
    open(sp)
    set_speed(sp, speed)
    set_frame(sp, ndatabits=8, parity=SP_PARITY_NONE, nstopbits=1)
    return sp
end

"""
Create and configure a SerialPort object based on command-line args
"""
function open_serial_port()
    validate_args()
    port_address = ARGS[1]
    sp = SerialPort(port_address)
    open(sp)
    set_speed(sp, parse(Int, ARGS[2]))
    set_frame(sp, ndatabits=8, parity=SP_PARITY_NONE, nstopbits=1)
    return sp
end

"""
Return a unit quaternion from components of q in d
"""
function qnorm(d)
    q = [d["qw"],d["qx"],d["qy"],d["qz"]]
    return q/sqrt(sum(q .* q))
end

"""
https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
"""
function to_euler(q)
    q0,q1,q2,q3 = q

    roll = atan2(2(q0*q1 + q2*q3), 1 - 2*(q1^2 + q2^2))
    pitch = asin(2(q0*q2 - q3*q1))
    yaw = atan2(2(q0*q3 + q1*q2), 1 - 2*(q2^2 + q3^2))

    return roll, pitch, yaw
end

"""
Callback to take HTTP requests and return responses.
It is meant to be passed to an HttpHandler constructor.
The response parameter may be eventually removed as a parameter and
instantiated inside the function instead, see
https://github.com/JuliaWeb/HttpServer.jl/issues/74
"""
function httph(request::Request, response::Response)

    files = ["vehicle/index.html"; find("vehicle", ".js");
             "orientation/index.html"; find("orientation/js", ".js")]

    for file in files
        if startswith("/$file", request.resource)
            println("serving /", file)
            response = open(readall, file)
            return response
        end
    end

    response.status = 404
    response.data  = "<h3>$(request.resource[2:end])</h3> <h3>not found.</h3>"
    return response
end

# function run(httph::Function, tcp_port::Integer, app::Function, mcu_nodes::Array{McuNode})
function run(httph::Function, tcp_port::Integer, app::Function, mcu_nodes)

    wsh = WebSocketHandler() do req, client
        print(client)
        while true
            # Read string from client, decode, and parse to Dict
            msg = JSON.parse(bytestring(read(client)))
            if haskey(msg, "text") && msg["text"] == "ready"
                println("Received update from client: ready")
                app(client, mcu_nodes)
            end
        end
    end

    server = Server(HttpHandler(httph), wsh)
    HttpServer.run(server, tcp_port)
end

function run(httph::Function, tcp_port::Integer, app::Function, sp::SerialPort, csvkeys)

    wsh = WebSocketHandler() do req, client
        print(client)
        while true
            # Read string from client, decode, and parse to Dict
            msg = JSON.parse(bytestring(read(client)))
            if haskey(msg, "text") && msg["text"] == "ready"
                println("Received update from client: ready")
                app(client, sp, csvkeys)
            end
        end
    end

    server = Server(HttpHandler(httph), wsh)
    HttpServer.run(server, tcp_port)
end

function main()

    # Streaming data from orientation sensor unit has lines like this:
    # t:135992,AMGS:0333,qw:11737,qx:-107,qy:401,qz:-11424
    imu_port = open_serial_port("/dev/cu.wchusbserial1420", 115200)
    imu_keys = ["AMGS","qw","qx","qy","qz"]

    # Time, ADC from pot, steering angle, pulses sent to stepper
    # t: 38400 adc: 384 sa: 0.10 steps: 18750
    str_port = open_serial_port("/dev/cu.usbmodem1411", 115200)
    str_keys = ["adc", "steps"]

    mcu_nodes = [
        NodeMD{AngleNode}(imu_port, imu_keys, "quaternions")
        NodeMD{SteerNode}(str_port, str_keys, "steering")
        # TODO NodeMD{LidarNode}(ldr_port, ldr_keys, "lidar")
    ]

    # Single-MCU version
    # run(httph, 8000, send_sensor_data, imu_port, imu_keys)

    # Setup http/websocket server and send streaming MCU data
    run(httph, 8000, process_streams, mcu_nodes)

    # Just print streaming data to stdout
    # process_streams(mcu_nodes)

end

main()