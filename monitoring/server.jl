using HttpServer
using WebSockets
using LibSerialPort
import JSON

include("server_utils.jl")

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
Send data to stdout instead of browser. Useful for debugging.
"""
function console(csvkeys)
    sp = open_serial_port()
    while true
        line = readline(sp)
        d = csv2dict(line, csvkeys)
        if keys_ok(d, csvkeys)

            q = qnorm(d)

            # Replace quaternion with versor
            d["qw"], d["qx"], d["qy"], d["qz"] = q

            # Add Euler/Tait-Bryan angles
            d["roll"], d["pitch"], d["yaw"] = to_tait_bryan(q)

            println(d)
        else
            println(line)
        end
    end
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
    # Streaming data has lines that look like this:
    # t:135992,AMGS:0333,qw:11737,qx:-107,qy:401,qz:-11424
    csvkeys = ["t","AMGS","qw","qx","qy","qz"]

    sp = open_serial_port()

    run(httph, 8000, send_sensor_data, sp, csvkeys)
end

# console(["t","AMGS","qw","qx","qy","qz"])
main()