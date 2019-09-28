import serial
import struct
from ctypes import (Union, Array, c_uint8, c_uint32, cdll, CDLL) 

class uint8_array(Array):
	_type_= c_uint8
	_length_ = 4
	def __init__(self,len):
		uint8_array._length_ = len

class u_type(Union):
	_fields_ = ("data", c_uint32), ("chunk", uint8_array)

# load printf function from Dynamic Linked Libary libc.so.6 (I'm use linux)
libc = CDLL(cdll.LoadLibrary('libc.so.6')._name)
printf = libc.printf

if __name__ == "__main__":
    # initialize union
    _32bitsdata = u_type()
    # set values to chunk
    _32bitsdata.chunk[:] = (1, 2, 3, 4)
    # and print it
    printf(b"Data in 32 bits: %d\n", _32bitsdata.data)


class MsgTaoTaoBoard:
	def _init__(self):
		self.SOM = 0 #start of the msg
		self.CI  = 0 #continuity conunter
		self.len = 0 #len is len of bytes to follow, NOT including CS
		self.cmd = 0 #read or write
		self.code= 0 #coe of  value to write
		self.pwm1= 0 #absolute value ranging from -1000 to 1000 .. Duty Cycle * 10 for first wheel
		self.pwm2= 0 #absolute value ranging from -1000 to 1000 .. Duty Cycle * 10 for second wheel
		self.CS  = 0 #checksum
class TaoTaoBoard:
	def __init__(self,name,age):
		self.msg = {'SOM':0,'CI':0,'len' :0,'cmd':0,'code':0,'pwm1':0,'pwm2':0,'CS':0}
		self.CI  = 0
	
	def write_pwm_msg(self, pwm_l, pwm_r):
		self.msg.SOM = 4 # start of the message, 4 for No ACKs
		self.CI = self.CI + 1
		self.msg.CI = self.CI # Message continuity Indicator. Subsequent Message with the same CI are discarded, need to be incremented.
		self.msg.len = 1 + 1 + 4 + 4 # cmd(1), code(1), pwm1(4) and pwm2(4)
		self.msg.cmd  = 'r' # Pretend to send answer to read request. This way HB will not reply. Change to 'W' to get confirmation from board
		self.code = 0x0E # simpler PWM
		self.pwm_l = pwm_l
		self.pwm_r = pwm_r
		self.CS = 0
		#for 
def main():
	port = "/dev/serial0"
	port = serial.Serial(port,9600,timeout = 2)
	#port = serial.Serial("/dev/ttyACM0", baudrate=115200, timeout=3.0)
	#port.write(struct.pack('>B', 1))
	port.flushInput()
	port.flushOutput()
	bytes = port.write("Hello".encode('utf-8'))
	rec = port.read(bytes)
	print(rec)
	print('Send OK')
	port.close()



def test_port():
	test_string = "Testing 1 2 3 4".encode('utf-8')
	#test_string = b"Testing 1 2 3 4" ### Will also work

	port_list = ["/dev/serial0", "/dev/ttyAMA0", "/dev/serial1", "/dev/ttyS0"]

	for port in port_list:
		try:
			serialPort = serial.Serial(port, 9600, timeout = 2)
			serialPort.flushInput()
			serialPort.flushOutput()
			print("Opened port", port, "for testing:")
			bytes_sent = serialPort.write(test_string)
			print ("Sent", bytes_sent, "bytes")
			loopback = serialPort.read(bytes_sent)
			print(loopback)
			if loopback == test_string:
				print ("Received", len(loopback), "valid bytes, Serial port", port, "working \n")
			else:
				print ("Received incorrect data", loopback, "over Serial port", port, "loopback\n")
			serialPort.close()
		except IOError:
			print ("Failed at", port, "\n")

if __name__ == '__main__':
	#test_port()
	main()

