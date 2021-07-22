### BMP HAB Script ###

from Adafruit_BMP085 import BMP085
import datetime
from time import sleep

datetime.datetime(2009, 1, 6, 15, 8, 24, 78)

#SETUP
currentTime = datetime.datetime.now()
fileLocation = '/home/pi/data/bmp_data_' + str(currentTime)[0:19] + '.csv'
print(fileLocation)
data = open(fileLocation, "w")
data.close()

bmp = BMP085(0x77)

#FUNCTIONS
def getData():
    temp = bmp.readTemperature()
    pressure = bmp.readPressure()
    altitude = bmp.readAltitude()
    return temp, pressure, altitude

while True:
    try:
        allData = open(fileLocation, "a")
        temp, pressure, altitude = getData()
        currentTime = datetime.datetime.now()
        total = str(currentTime)[0:22] + ", ,BMPTemp: ," + str(temp) + ", ,Pressure: ," + str(pressure) + ", ,Altitude: ," + str(altitude)
        
        allData.write(total)
        allData.write('\n')
        print(total)
        allData.close()
        
        sleep(2)
    
    except:
        sleep(1)