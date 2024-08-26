"""
Example for a BLE 4.0 Server
"""
import sys
import logging
import asyncio
import threading

from typing import Any, Union

import os
import signal
import subprocess
import time


from bless import (  # type: ignore
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(name=__name__)


my_service_uuid = "A07498CA-AD5B-474E-940D-16F1FBE7E8CD"
stop_char_uuid = "51FF12BB-3ED9-46E5-B4F9-D64E2FEC021B"
start_char_uuid = "51FF12BB-3ED8-46E5-B4F9-D64E2FEC021B"


cmd = '/home/nick/mtb_metrics/pmw3389'
pro = None


def start_recording(fn):
    global pro

    if pro is not None:
        logger.debug('Error: Recording already in progress')
        return

    i=0
    while True:
        full_filename = os.path.join('/home/nick/accel_data', fn + '_%d.csv' % i)
        if not os.path.exists(full_filename): break
        i+=1
    logger.debug(f'Full filename: {full_filename}')

    full_cmd = [cmd, full_filename]

    pro = subprocess.Popen(full_cmd, preexec_fn = os.setsid)
    logger.debug('Started recording')

def stop_recording():
    global pro
    if pro is None:
        logger.debug('Error: NO recording in progress')
        return

    os.killpg(os.getpgid(pro.pid), signal.SIGTERM)
    time.sleep(2)

    logger.debug("Stopped")
    pro = None

    return 'summary_placeholder'



# NOTE: Some systems require different synchronization methods.
trigger: Union[asyncio.Event, threading.Event]
if sys.platform in ["darwin", "win32"]:
    trigger = threading.Event()
else:
    trigger = asyncio.Event()


#def read_request(characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
#    logger.debug(f"Reading {characteristic.value}")
#    return characteristic.value


def write_request(characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
    logger.debug(characteristic.uuid)
    characteristic.value = value
    logger.debug(f"Char value set to {characteristic.value}")
    if characteristic.uuid.lower() == start_char_uuid.lower():
        fn = characteristic.value.decode('ascii')
        logger.debug(f"Start recording, filename: {fn}")
        start_recording(fn)

    elif characteristic.uuid.lower() == stop_char_uuid.lower():
        logger.debug("STOP")
        response = stop_recording()

async def run(loop):
    trigger.clear()
    # Instantiate the server
    my_service_name = "MTB Metrics"
    server = BlessServer(name=my_service_name, loop=loop)
    #server.read_request_func = read_request
    server.write_request_func = write_request

    # Add Service
    await server.add_new_service(my_service_uuid)

    # Add a Characteristic to the service
    char_flags = (
        GATTCharacteristicProperties.write
    )
    

    #permissions = GATTAttributePermissions.readable | GATTAttributePermissions.writeable
    permissions = GATTAttributePermissions.writeable

    await server.add_new_characteristic(
        my_service_uuid, start_char_uuid, char_flags, None, permissions
    )

    await server.add_new_characteristic(
        my_service_uuid, stop_char_uuid, char_flags, None, permissions
    )

    await server.start()
    logger.debug("Advertising")
    if trigger.__module__ == "threading":
        trigger.wait()
    else:
        await trigger.wait()

    await asyncio.sleep(2)
    logger.debug("Updating")
    server.get_characteristic(my_char_uuid)
    server.update_value(my_service_uuid, "51FF12BB-3ED8-46E5-B4F9-D64E2FEC021B")
    await asyncio.sleep(5)
    await server.stop()


loop = asyncio.get_event_loop()
loop.run_until_complete(run(loop))
