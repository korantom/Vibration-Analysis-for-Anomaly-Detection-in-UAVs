import serial
import time

# Open serial port
ser = serial.Serial(
    "/dev/cu.usbmodemC1E9B7D03",
    # baudrate=115200,
    baudrate=9600,
    timeout=0.5,
    # Number of data bits
    bytesize=serial.EIGHTBITS,
    # bytesize=serial.SEVENBITS,
    # Parity check
    # parity=serial.PARITY_NONE,
    # parity=serial.PARITY_ODD,
    # parity=serial.PARITY_EVEN,
    # Stop bits
    # stopbits=serial.STOPBITS_ONE,
    # stopbits=serial.STOPBITS_ONE_POINT_FIVE,
    # stopbits=serial.STOPBITS_TWO,
)
time.sleep(2)


def check_shell_prompt(line: str):
    return "uart:~$" in line


print(f"{ser=}")

while True:
    # time.sleep(1)

    # Check how many bytes in buffer (at given moment, doesn't wait)
    buffered_byte_count = ser.in_waiting

    # Read a '\n' terminated line (timeouts)
    line = ser.readline()

    # Skip if nothing read
    if line == bytes():
        continue

    # Try to decode read line
    try:
        line_str = line.decode("utf-8")
        print(line_str)

    except:
        line_str = ""
        print(f"unable to decode ascii")
        print(line)
    print()

    # Check line for prompt
    if check_shell_prompt(line_str):

        # Wait for input
        while (cmd := input("uart:~$")) == "":
            pass

        cmd += "\n"
        ser.write(cmd.encode("ascii"))


# close port
ser.close()
