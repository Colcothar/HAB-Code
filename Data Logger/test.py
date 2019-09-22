import sys 
import Adafruit_DHT 
from time import sleep

while True:
	data = open ("data.txt", "a")
	humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.AM2302, 4)
	data.write('Temp={0:0.1f}*  Humidity={1:0.1f}% \n'.format(temperature, humidity))
	print('Temp={0:0.1f}*  Humidity={1:0.1f}% \n'.format(temperature, humidity))
	sleep(1)
	data.close()

