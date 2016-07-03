using HttpServer
using WebSockets
using LibSerialPort
import JSON

include("server_utils.jl")

const MAX_LEFT_ADC = 577
const MAX_RIGHT_ADC = 254
const MAX_LEFT_DEG = -30
const MAX_RIGHT_DEG = 30

"""
Rotation angle alpha of vehicle velocity vector as a function of steering angle
delta:

  alpha = atan(a * tan(delta)/b)

where b is the wheelbase and a is from rear of vehicle to CM. To a decent
approximation, a = b/2. Then at the max steering angle of pi/6, the max heading
change is atan2(tan(pi/6), 2) = 0.2810 rad.
"""
const MAX_HEADING_CHANGE_RAD = atan2(tan(pi/180 * MAX_RIGHT_DEG), 2)

# Abstract type representing a microcontroller "unit", defined here as the
# microcontroller itself + the devices under its direct control (sensors,
# actuators, etc.)
abstract Mcu

# Mcu singleton subtypes (for parametrization and dispatch)
type LidarMcu <: Mcu end
type SteerMcu <: Mcu end
type AngleMcu <: Mcu end

# Data type for nodes on the serial "network"
immutable SerialNode{T <: Mcu}
    sp::SerialPort
    keys::Array{AbstractString}
    message_name::AbstractString
end

function send_command(node::SerialNode)
    # Send an update request
    write(node.sp, "u\n")
end

"""
Generates a linear mapping function given a domain (x1,x2) and a range(y1,y2).
"""
function linear_map(x1,x2,y1,y2)
    @assert x2 >= x1
    function f(x)
        m = (y2-y1)/(x2-x1)
        # return clamp(y1 + m*(x - x1), y1, y2)
        return y1 + m*(x - x1)
    end
    return f
end

"""
Functions to convert between steering angle in degrees and in ADC units.
"""
deg_to_adc = linear_map(MAX_LEFT_DEG, MAX_RIGHT_DEG, MAX_LEFT_ADC, MAX_RIGHT_ADC)
adc_to_deg = linear_map(MAX_RIGHT_ADC, MAX_LEFT_ADC, MAX_RIGHT_DEG, MAX_LEFT_DEG)

"""
Compute steering angle for a target vehicle heading, given the current heading.
Heading angles should be in provided in degrees, increasing CW from north = 0.
Target steering angle is returned in degrees.
"""
function target_steer_angle(target_heading, vehicle_heading)
    # @assert 0 < target_heading < 360 "target_heading outside allowed range"
    # @assert 0 < vehicle_heading < 360 "vehicle_heading outside allowed range"

    # Angle [rad] of velocity vector from vehicle centerline at CM. L < 0; R > 0.
    a = pi/180*(target_heading - vehicle_heading)

    # Redefine to be inside a range of -pi to pi
    a = (a < -pi) ? a + 2*pi : (a > pi) ? a - 2*pi : a

    a > MAX_HEADING_CHANGE_RAD && return MAX_RIGHT_DEG
    a < -MAX_HEADING_CHANGE_RAD && return MAX_LEFT_DEG

    # Steering angle delta from steering model: delta = atan(2*tan(a)).
    delta = atan(2*tan(a))

    # A Taylor expansion to cubic order could be used instead
    # (good to 1% in worst case).
    # return 2*a*(1 - a*a)

    return delta*180/pi
end

"""
Methods to process streams from microcontrollers. Take a line of text, do any
needed processing, and return a dict suitable for conversion to JSON.
"""
function read_reply(sn::SerialNode{SteerMcu})
    line = readuntil(sn.sp, '\n', 100)
    d = csv2dict(line, sn.keys)
    if keys_ok(d, sn.keys)
        # TODO: map adc to degrees here, using limits from steering.ino?
        # int maxLeftADC = 577; int maxRightADC = 254;
    else
        println("Missing key(s) - skipping: ", strip(line))
    end
    return d
end

function read_reply(sn::SerialNode{AngleMcu})
    line = readuntil(sn.sp, '\n', 100)
    d = csv2dict(line, sn.keys)
    if keys_ok(d, sn.keys)

        # Get quaternion from dict
        q = qnorm(d)

        # Add Euler/Tait-Bryan angles
        d["roll"], d["pitch"], d["yaw"] = to_euler(q)
    else
        println("Missing key(s) - skipping", strip(line))
    end
    # Useful for debugging:
    # println(strip(line))
    return d
end

function read_reply(sn::SerialNode{LidarMcu})
    # TODO
end

"""
Read streams from all MCUs in the list, process and format to JSON messages, then
send to WebSocket client.
"""
# function process_streams(client::WebSockets.WebSocket, mcu_nodes::Array{SerialNode})
#     imu, str = mcu_nodes
#     while true
#         map(send_command, mcu_nodes)

#         sleep(0.04) # From trial and error on laptop

#         for node in mcu_nodes
#             message_dict = read_reply(node)
#             if keys_ok(message_dict, node.keys)
#                 send_json(node.message_name, message_dict, client)
#             end
#         end
#         # Send steer command
#         write(str.sp, "s 500\n")
#     end
# end

function process_streams(client::WebSockets.WebSocket, mcu_nodes::Array{SerialNode})
    imu, str = mcu_nodes
    while true

        # List of most recent messages as Dict objects
        messages = []

        # Send out request updates
        map(send_command, mcu_nodes)

        # Read replies and forward them to monitoring page in JSON format
        for node in mcu_nodes
            message_dict = read_reply(node)
            push!(messages, message_dict)
            if keys_ok(message_dict, node.keys)
                send_json(node.message_name, message_dict, client)
            end
        end

        d_imu = messages[1]
        if keys_ok(d_imu, imu.keys)
            target_heading = 270 # Whatever, just testing

            # Current heading
            h = 90 - 180/pi*d_imu["yaw"]
            if h < 0
                h += 360
            end

            st = target_steer_angle(target_heading, h)
            adc = round(Int, deg_to_adc(st))
            println("Current: $h, target_heading: $target_heading, target_steer_angle: $st, adc: $adc")
            # Send steer command
            write(str.sp, "s $adc\n")
        end
    end
end

function readuntil(sp::SerialPort, delim::Char, timeout_ms::Integer)
    out = Char[]
    start_time = time_ns()
    while !eof(sp)
        (time_ns() - start_time)/1e6 > timeout_ms && break
        if nb_available(sp) > 0
            c = read(sp, Char)
            push!(out, c)
            c == delim && break
        end
    end
    return join(out)
end

"""
Send data to stdout instead of browser. Useful for debugging.
"""
function process_streams(mcu_nodes)
    while true
        map(send_command, mcu_nodes)

        sleep(0.02)

        for node in mcu_nodes
            d = read_reply(node)
            keys_ok(d, node.keys) && [Base.print("$k:$(d[k]) ") for k in keys(d)]
            println()
        end
    end
    return nothing
end

function process_streams_test(mcu_nodes)
    imu, str = mcu_nodes
    while true
        # Send update requests
        write(imu.sp, "u\n")
        write(str.sp, "u\n")

        sleep(0.04)

        d_imu = read_reply(imu)
        d_str = read_reply(str)

        # for node in mcu_nodes
        #     d = read_reply(node)
        #     keys_ok(d, node.keys) && [Base.print("$k:$(d[k]) ") for k in keys(d)]
        #     println()
        # end
        if keys_ok(d_imu, imu.keys)
            target_heading = 270 # Whatever, just testing

            # Current heading
            h = -180/pi*d_imu["yaw"]
            # h = (h < 0) ? h + 360 : (h > 360) ? h - 360 : h

            st = target_steer_angle(target_heading, h)
            adc = round(Int, deg_to_adc(st))
            println("Current: $h, target_heading: $target_heading, target_steer_angle: $st, adc: $adc")
            # Send steer command
            write(str.sp, "s $adc\n")
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

function run(httph::Function, tcp_port::Integer, app::Function, mcu_nodes::Array{SerialNode})

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

    # TODO: make this a Dict so nodes can be accessed by name
    mcu_nodes = [
        SerialNode{AngleMcu}(imu_port, imu_keys, "quaternions")
        SerialNode{SteerMcu}(str_port, str_keys, "steering")
        # TODO SerialNode{LidarMcu}(ldr_port, ldr_keys, "lidar")
    ]

    # Single-MCU version
    # run(httph, 8000, send_sensor_data, imu_port, imu_keys)

    # Setup http/websocket server and send streaming MCU data
    run(httph, 8000, process_streams, mcu_nodes)

    # Just print streaming data to stdout
    # process_streams_test(mcu_nodes)

end

main()