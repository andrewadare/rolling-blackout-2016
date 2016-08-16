#!/usr/bin/env julia

# Fake data generator. Binds PUB socket to tcp://*:5556 and publishes
# messages in the same format as the vehicle controller updates

using ZMQ

context = Context()
socket = Socket(context, PUB)
ZMQ.bind(socket, "tcp://*:5556")

time_ms() = round(Int, time_ns()/1e6)

# Return a random point on the unit sphere
function sphere_xyz()
    phi = 2pi*rand()
    z = 2*rand() - 1
    theta = acos(z)
    x = sin(theta)*cos(phi)
    y = sin(theta)*sin(phi)
    return x, y, z
end

# Return a quaternion with random orientation
function rand_quat()
    x,y,z = sphere_xyz()
    # phi = 2pi*rand()
    phi = 0.01*rand() # Keep narrow so output is not so crazy looking
    s = sin(phi)
    q = [cos(phi), s*x, s*y, s*z]

    # Make components integral to emulate sensor output
    return [round(Int, 10000*x) for x in q]
end

start = time_ms()
steering_angle = 0

while true
    t = time_ms() - start
    a = rand(0:3)
    qw, qx, qy, qz = rand_quat()
    steering_angle += 0.5*rand(-1:1)
    steering_angle = clamp(steering_angle, -45, 45)
    line = "t:$t,AMGS:$(a)333,qw:$qw,qx:$qx,qy:$qy,qz:$qz,sa:$steering_angle,odo:0,r:0,b:0"
    ZMQ.send(socket, line)
    sleep(0.025)
end

# Never reached
ZMQ.close(socket)
ZMQ.close(context)
