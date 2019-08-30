import sys
import Adafruit_DHT
from time import sleep
import picamera

data = open("data.txt" , "w")

with picamera.PiCamera() as camera:
    camera.resolution = (1920, 1080)
    for filename in camera.capture_continuous('/home/pi/time-lapse/img{timestamp:%H-%M-%S-%f}.jpg'):
        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.AM2302, 4)
        data.write(humidity, temperature)
        sleep(1)

data.close()


