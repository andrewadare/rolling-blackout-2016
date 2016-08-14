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
