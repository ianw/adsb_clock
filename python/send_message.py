import serial
import sys

ser = serial.Serial('/dev/ttyACM0', 9600)

message = sys.argv[1]

ser.write("m%02d%s" % (len(message), message))
