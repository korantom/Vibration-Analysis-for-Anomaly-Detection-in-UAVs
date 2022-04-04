"""
Zephyr app:
- provides command line api
- can:
    - controls hw
    - ...
    - ...
- should provide some wrapper for 1 test/measurement
    - pwm_set_throttle
    - enable_accelerometer
    - sleep
    - disable_accelerometer
    - pwm_set_throttle
    - DUMP DATA TO CONSOLE


What do I need:

"""

def check_uart(line):
    return 'uart:~$' in line.decode("utf-8")

import serial
import time

# open serial port
ser = serial.Serial(
    '/dev/cu.usbmodemC1E9B7D03',
    baudrate=115200,
    timeout=1
    )

print(f'{ser=}')

time.sleep(5)

uart_ready = False
while True:
    b=ser.inWaiting()
    print(b)

    # read a '\n' terminated line
    line = ser.readline()
    b=ser.inWaiting()
    print(f'{line=}')
    print(f'{type(line)=}')

    if check_uart(line):
        uart_ready = True

    if uart_ready and b==0:
        print('uart:~$:')
        print()
        print()
        cmd = input()
        cmd += '\r\n'
        print(f'uart:~$:{cmd=}')
        ser.write(cmd.encode('utf-8'))

# close port
ser.close()


def init():
    pass
