from gpiozero import LED, Button
from signal import pause
import os
import signal
import subprocess
import time

cmd = '/home/nick/mtb_metrics/pmw3389'


led = LED(13)
button = Button(19)


isRecording=False
pro=None

def button_press(fn='button'):
	global isRecording
	global pro



	if isRecording:
		os.killpg(os.getpgid(pro.pid), signal.SIGTERM)
		time.sleep(2)

		print("Stopped")
		pro = None


		led.off()
		isRecording=False

	else:
		
		i=0
		while True:
			full_filename = os.path.join('/home/nick/accel_data', fn + '_%d.csv' % i)
			if not os.path.exists(full_filename): break
			i+=1
		print(f'Full filename: {full_filename}')

		full_cmd = [cmd, full_filename]

		pro = subprocess.Popen(full_cmd, preexec_fn = os.setsid)
		
		led.on()
		isRecording=True


button.when_pressed = button_press

pause()
