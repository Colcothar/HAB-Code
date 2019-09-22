import sys
import Adafruit_DHT
from time import sleep
import picamera
import datetime
from Adafruit_BMP085 import BMP085

data = open("data.txt" , "w")
datetime.datetime(2009, 1, 6, 15, 8, 24, 78915)
bmp = BMP085(0x77)

def getInfo():
    temp = bmp.readTemperature()
    pressure = bmp.readPressure()
    altitude = bmp.readAltitude()
    global temp, pressure, altitude

with picamera.PiCamera() as camera:
    camera.resolution = (1920, 1080)
    for filename in camera.capture_continuous('/home/pi/time-lapse/img{timestamp:%H-%M-%S-%f}.jpg'):
        data = open("data.txt" , "a")
        getInfo()
        
        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.AM2302, 4)
        datetime.datetime.now()
        
        data.write(datetime.datetime.now(), humidity, temperature, "Temperature: %.2f C" % temp, "Pressure:    %.2f hPa" % (pressure / 100.0)), "Altitude:    %.2f" % altitude)
        
        
        sleep(1)
        data.close()




