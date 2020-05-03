import subprocess
import os
from time import sleep

path = "C:\\Users\\danie\\Google Drive\\WWU Classes\\Capstone Project\\Photon"

os.system("particle usb dfu")
flashfile = 0
for file in os.listdir(path):
    if file.endswith(".bin"):
        flashfile = file
        break

sleep(2)
os.system("particle flash --usb " + flashfile)
sleep(2)
os.system("particle serial monitor")
