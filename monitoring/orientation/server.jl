using HttpServer
using WebSockets
using LibSerialPort
import JSON


# Streaming data has lines that look like this:
# t:135992,AMGS:0333,qw:11737,qx:-107,qy:401,qz:-11424
# TODO: reduce scope.
csvkeys = ["t","AMGS","qw","qx","qy","qz"]

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
    return d
end

"""
From a csv line like this:

Time:67549,H:254.37,R:-1.44,P:3.81,A:3,M:3,G:3,S:3

create and return a Dict object. The keys array contains the items to be
included in the Dict. If the line can't be split into at least length(keys),
an empty Dict is returned.
"""
function csv2dict(line::AbstractString, keys::AbstractVector)
    d = Dict{ASCIIString, Float64}()
    a = split(line, ",")
    length(a) >= length(keys) || return d
    for item in a
        contains(item, ":") || return d
        k,v = split(item, ":")
        if in(k, keys)
            try
                f = parse(Float64, v)
                d[k] = f
            end
        end
    end
    d
end

"""
Dictionary QA check
"""
function keys_ok(dict, keylist)
    result = true
    for key in keylist
        if !haskey(dict, key)
            result = false
        end
    end
    return result
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

function validate_args()

    if length(ARGS) != 2
        println("Usage:")
        println("julia run_server.jl address speed")
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
function send_sensor_data(client::WebSockets.WebSocket, sp::SerialPort)
    while true
        line = readline(sp)
        d = csv2dict(line, csvkeys)
        if keys_ok(d, csvkeys)
            send_json("quaternions", d, client)
        else
            println("Missing key(s) - skipping")
        end
        # println(strip(line))
    end
end

function print(client::WebSockets.WebSocket)
    println("Connected to WebSocket client")
    println("    client.id: ",         client.id)
    println("    client.socket: ",     client.socket)
    println("    client.is_closed: ",  client.is_closed)
    println("    client.sent_close: ", client.sent_close)
end

"""
Serve page(s) and supporting files over HTTP.
"""
http_handler = HttpHandler() do req::Request, res::Response

    files = ["index.html"; find("js", ".js")]

    for file in files
        if startswith("/$file", req.resource)
            println("serving /", file)
            return Response(open(readall, file))
        end
    end

    Response(404)
end
http_handler.events["error"]  = (client, err) -> println(err)
http_handler.events["listen"] = (port)        -> println("Listening on $port...")

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
Send data to stdout instead of browser. Useful for debugging.
"""
function console()
    sp = open_serial_port()
    while true
        line = readline(sp)
        d = csv2dict(line, csvkeys)
        if keys_ok(d, csvkeys)
            println(d)
        else
            println(line)
        end
    end
end

function main()
    sp = open_serial_port()

    websocket_handler = WebSocketHandler() do req, client
        print(client)
        while true
            # Read string from client, decode, and parse to Dict
            msg = JSON.parse(bytestring(read(client)))
            if haskey(msg, "text") && msg["text"] == "ready"
                println("Received update from client: ready")
                send_sensor_data(client, sp)
            end
        end
    end

    wsserver = Server(http_handler, websocket_handler)
    println("Starting WebSocket server.")
    run(wsserver, 8000)
end

# console()
main()