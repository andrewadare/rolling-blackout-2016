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

function print(client::WebSockets.WebSocket)
    println("Connected to WebSocket client")
    println("    client.id: ",         client.id)
    println("    client.socket: ",     client.socket)
    println("    client.is_closed: ",  client.is_closed)
    println("    client.sent_close: ", client.sent_close)
end

