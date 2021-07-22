### CPU Temp HAB Logger ###

import os
from time import sleep
import datetime

datetime.datetime(2009, 1, 6, 15, 8, 24, 78)
currentTime = datetime.datetime.now()
fileLocation = '/home/pi/data/cpu_data_' + str(currentTime)[0:19] + '.csv'
data = open(fileLocation, "w")
data.close()


def getCPUTemp():
    temp=os.popen("vcgencmd measure_temp").readline()
    return (temp.replace("temp=","").replace("'C", ""))
    
def getCPUFreq():
    cpu_freq_raw = os.popen("cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq").readline()
    return cpu_freq_raw



while True:
    try:
        allData = open(fileLocation, "a")
        currentTime = datetime.datetime.now()
        total = str(currentTime)[0:22] + ", ,CPU  TEMP: ," + getCPUTemp()[0:-1] + ", ,CPU  FREQ: ," + getCPUFreq()
        
        allData.write(total)
        allData.write('\n')
        print(total)

        allData.close()

        sleep(2)
        
    except Exception as e:
        print(e)
        sleep(1)
