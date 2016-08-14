"""
From a line of text like "10 20 30", and a set of label keys like
["k1", "k2", "k3"], return a dictionary like this:
Dict{AbstractString,Float64}("k3"=>30.0,"k1"=>10.0,"k2"=>20.0)
"""
function line2dict(line::AbstractString, keys::AbstractVector; delimiter::AbstractString=" ", )
    d = Dict{String, Float64}()
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
    d = Dict{String, Float64}()
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
    return d
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
