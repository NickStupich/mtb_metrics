import os
import signal
import subprocess
import time

if __name__ == "__main__":
    cmd = ['./pmw3389', '/home/nick/accel_data/python_temp_data.csv']
    pro =subprocess.Popen(cmd,  preexec_fn=os.setsid)

    print('running')

    time.sleep(3)
    os.killpg(os.getpgid(pro.pid), signal.SIGTERM)
    time.sleep(2)
    print('killed')
