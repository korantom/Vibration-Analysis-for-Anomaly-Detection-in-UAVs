import serial
import time

# Open serial port
ser = serial.Serial("/dev/cu.usbmodemC1E9B7D03", baudrate=115200, timeout=1.0)
time.sleep(2)

# Start an infinite print loop
while input("start tester_infinite_print? (y/n): ") != "y":
    pass
ser.write("tester_infinite_print\r".encode("ascii"))

while True:
    time.sleep(1)

    # Check how many bytes in buffer (doesn't wait)
    buffered_byte_count = ser.in_waiting
    print(f"{buffered_byte_count=}")

    # Read a '\n' terminated line, timeouts otherwise
    line = ser.readline()
    print(f"{type(line), line=}")
    print()


# close port
ser.close()
