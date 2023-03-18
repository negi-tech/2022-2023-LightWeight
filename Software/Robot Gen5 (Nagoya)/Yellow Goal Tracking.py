# Untitled - By: nkoji - 月 3 6 2023

import sensor, image, time, math, pyb, ustruct
from pyb import UART

center = [192, 93] #円錐の中心(x,y)
#xの減少方向が正面、増加方向が後ろ
#yの減少方向が右、増加方向が左(上から俯瞰して)
thresholds = [(60, 80, -20, -5, 40, 75)]

uart = UART(3, 115200)

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)

clock = time.clock()

while(True):
    clock.tick()
    img = sensor.snapshot()
    goal = [0,0]
    for blob in img.find_blobs(thresholds, pixels_threshold=200, area_threshold=200):
        #img.draw_rectangle(blob.rect())
        #img.draw_cross(blob.cx(), blob.cy())
        goal[0] = blob.cx()
        goal[1] = blob.cy()
    goal_theta = 0
    abs_goal = [goal[0] - center[0], goal[1] - center[1]]
    if (goal[0] != 0) and (goal[1] != 0):
        goal_theta = math.atan2(abs_goal[0], abs_goal[1])
        goal_theta += math.pi / 2
        if goal_theta > math.pi:
            goal_theta = math.pi * 2 - goal_theta
        elif goal_theta < -math.pi:
            goal_theta = math.pi * 2 + goal_theta
    else:
        goal_theta = math.pi
    #print("角度: ", end="")
    print(goal_theta/math.pi*180)
    #print("°\tFPS: ", end="")
    #print(clock.fps())
    x = int((goal_theta + math.pi) * 100)
    y = int(x) & 0b0000000001111111
    z = int(x) >> 7
    try:
        uart.write(ustruct.pack('B',255))
        uart.write(ustruct.pack('B',int(y)))
        uart.write(ustruct.pack('B',int(z)))
    except OSError as err:
        pass