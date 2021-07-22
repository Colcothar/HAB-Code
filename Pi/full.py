import sys
import Adafruit_DHT
import picamera
import datetime
from Adafruit_BMP085 import BMP085
import os
import time
import RPi.GPIO as GPIO


#SETUP
fileLocation = "/home/pi/HAB-Code/Data Logger/Data/data.csv"
data = open(fileLocation, "w")
data.close()
datetime.datetime(2009, 1, 6, 15, 8, 24, 78)
bmp = BMP085(0x77)

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(26, GPIO.OUT)

for i in range(5):
        GPIO.output(26, GPIO.HIGH)
        time.sleep(1)
        GPIO.output(26, GPIO.LOW)
        time.sleep(1)



#FUNCTIONS
def getData():
    temp = bmp.readTemperature()
    pressure = bmp.readPressure()
    altitude = bmp.readAltitude()
    return temp, pressure, altitude

def getCpuTemp():
        temp=os.popen("vcgencmd measure_temp").readline()
        return (temp.replace("temp=","").replace("'C", ""))



#MAIN
with picamera.PiCamera() as camera:
    camera.resolution = (1920, 1080)
    for filename in camera.capture_continuous('/home/pi/HAB-Code/Data Logger/Images/Img {timestamp:%H:%M:%S}.jpg'):
        allData = open( fileLocation , "a")
        temp, pressure, altitude = getData()
        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.AM2302, 4)
        currentTime = datetime.datetime.now()

        total = str(currentTime)[0:22] + ", ,Temperature: ," +  str(temperature)[0:4] + ", ,Humidity: ," + str(humidity)[0:4] + ", ,BMPTemp: ," + str(temp) + ", ,Pressure: ," + str(pressure) + ", ,Altitude: ," + str(altitude) +  ", ,CPU  TEMP: ," + getCpuTemp()
        
        allData.write(total)
        print(total)

        allData.close()

        time.sleep(1)

#Temp in C
#pressure in Pa

