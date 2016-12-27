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
