### Camera HAB Script ###

import picamera
import datetime
from time import sleep

#SETUP
datetime.datetime(2009, 1, 6, 15, 8, 24, 78)

camera = picamera.PiCamera()
camera.resolution = (1920, 1080)

# Camera warm-up time
sleep(2)

while True: 
    try:
        currentTime = datetime.datetime.now()
        filepath = '/home/pi/images/Img ' + str(currentTime)[0:19] + '.jpg'
        camera.capture(filepath)
        sleep(2)
    
    except:
        sleep(1)
