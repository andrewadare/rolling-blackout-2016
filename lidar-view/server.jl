using HttpServer
using WebSockets
using LibSerialPort
import JSON

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

"""
Find files recursively in `dir` containing `substr`, e.g.
    find("js", ".js")
Returns result as an array of strings.
"""
function find(dir::AbstractString, substr::AbstractString)
    local out = Vector{ASCIIString}()

    function findall(dir, substr)
        f = readdir(abspath(dir))
        for i in f
            path = joinpath(dir, i)
            if isfile(path) && contains(path, substr)
                push!(out, path)
            end
            if isdir(path)
                findall(path, substr)
            end
        end
    end

    findall(dir, substr)

    out
end

"""
From a line of text like "10 20 30", and a set of label keys like
["k1", "k2", "k3"], return a dictionary like this:
Dict{AbstractString,Float64}("k3"=>30.0,"k1"=>10.0,"k2"=>20.0)
"""
function line2dict(line::AbstractString, keys::AbstractVector; delimiter::AbstractString=" ", )
    d = Dict{ASCIIString, Float64}()
    a = split(line, delimiter)
    length(a) >= length(keys) || return d

    for (i, item) in enumerate(a)
        try
            value = parse(Float64, item)
            d[keys[i]] = value
        end
    end
    d
end

function validate_args()

    if length(ARGS) != 2
        println("Usage:")
        println("julia server.jl address speed")
        println("\nAvailable ports:\n")
        list_ports()
        exit()
    end

    port_address = ARGS[1]
    if !ischardev(port_address)
        error("\n\n$port_address is not a valid character device.\n\n")
    end

    bps = ARGS[2]
    if !isnumber(bps)
        error("\n\n$bps is not a valid connection speed.\n\n")
    end
end

"""
Read delimited data lines streaming from serial port and pass them on as
JSON-formatted WebSocket messages.
"""
function send_sensor_data(client::WebSockets.WebSocket)
    validate_args()
    port_address = ARGS[1]
    sp = SerialPort(port_address)
    open(sp)
    set_speed(sp, parse(Int, ARGS[2]))
    set_frame(sp, ndatabits=8, parity=SP_PARITY_NONE, nstopbits=1)

    sleep(1)

    # Empirically-determined period of rangefinder encoder (ticks/rev)
    T = 3568

    # Streaming data has columns described by these two labels
    keys = ["enc", "rcm"]

    while true
        line = readline(sp)
        d = line2dict(line, keys)

        # Dictionary QA check
        keys_ok = true
        for key in keys
            if !haskey(d, key)
                keys_ok = false
            end
        end

        if keys_ok

            d["phi"] = 2*pi/T * d["enc"]

            send_json("sensor_data", d, client)
        else
            println("Missing key(s) - skipping")
        end
    end

end

wsh = WebSocketHandler() do req, client
    println("Handling WebSocket client")
    println("    client.id: ",         client.id)
    println("    client.socket: ",     client.socket)
    println("    client.is_closed: ",  client.is_closed)
    println("    client.sent_close: ", client.sent_close)

    while true

        # Read string from client, decode, and parse to Dict
        msg = JSON.parse(bytestring(read(client)))

        if haskey(msg, "text") && msg["text"] == "ready"
            println("Received update from client: ready")

            send_sensor_data(client)
        end
    end
end

"""
Serve page(s) and supporting files over HTTP.
"""
httph = HttpHandler() do req::Request, res::Response

    files = ["index.html"; find("js", ".js")]

    for file in files
        if startswith("/$file", req.resource)
            println("serving /", file)
            return Response(open(readall, file))
        end
    end

    Response(404)
end

httph.events["error"]  = (client, err) -> println(err)
httph.events["listen"] = (port)        -> println("Listening on $port...")

# validate_args()

# Instantiate and start a websockets/http server
server = Server(httph, wsh)

println("Starting WebSocket server.")
run(server, 8000)
