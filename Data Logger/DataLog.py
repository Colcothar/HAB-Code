

import sys
import Adafruit_DHT
from time import sleep
import picamera
import datetime
from Adafruit_BMP085 import BMP085

data = open("data.txt" , "w")
datetime.datetime(2009, 1, 6, 15, 8, 24, 78)
bmp = BMP085(0x77)

def getInfo():
    global temp, pressure, altitude
    temp = bmp.readTemperature()
    pressure = bmp.readPressure()
    altitude = bmp.readAltitude()

with picamera.PiCamera() as camera:
    camera.resolution = (1920, 1080)
    for filename in camera.capture_continuous('/home/pi/Time-Lapse/Img {timestamp:%H-%M-%S}.jpg'):
        data = open("data.txt" , "a")
        getInfo()

        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.AM2302, 4)
        a = datetime.datetime.now()
        a = str(a)
        total = a[0:22] + "    Temperature: " +  str(temperature)[0:4] + "  Humidity: " + str(humidity)[0:4] + "    $
        data.write(total)
        print(total)


        sleep(1)
        data.close()

#Temp in C
#pressure in hpa
