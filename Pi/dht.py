### DHT HAB Script ###

from os import error
import Adafruit_DHT
import datetime
from time import sleep

#SETUP
datetime.datetime(2009, 1, 6, 15, 8, 24, 78)
currentTime = datetime.datetime.now()
fileLocation = '/home/pi/data/dht_data_' + str(currentTime)[0:19] + '.csv'
data = open(fileLocation, "w")
data.close()


while True:
    try:
        allData = open( fileLocation , "a")
        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.AM2302, 4)
        currentTime = datetime.datetime.now()
        
        total = str(currentTime)[0:22] + ", ,Temperature: ," +  str(temperature)[0:4] + ", ,Humidity: ," + str(humidity)[0:4]
        
        allData.write(total)
        allData.write('\n')
        print(total)

        allData.close()

        sleep(2)
    
    except:
        sleep(1)