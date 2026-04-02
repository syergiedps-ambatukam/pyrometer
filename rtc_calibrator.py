import serial.tools.list_ports
import serial
import time
from datetime import datetime


def list_available_serial_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found.")
    else:
        print("Available serial ports:")
        for port in ports:
            print(f"- [{port.device}] {port.description}")


list_available_serial_ports()

port = input("masukkan port pyro : ")

ser = serial.Serial(port,9600, timeout=1)

time.sleep(10)  # tunggu Arduino reset


now = datetime.now()

data = now.strftime("%H:%M:%S,%d:%m:%Y\n")

print("Sent:", data)

ser.write(data.encode())


# baca respon Arduino selama 5 detik
start = time.time()

while time.time() - start < 5:
    if ser.in_waiting > 0:
        line = ser.readline().decode().strip()
        print("pyro:", line)


ser.close()
