#include <iostream>
#include <errno.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>

#include "mpu6050.h"


int main()
{
   if(wiringPiSetup() == -1) {
      printf("wiringPiSetup() failed\n");
      return -1;
   }

   int fd, result;

   // Initialize the interface by giving it an external device ID.
   // The MCP4725 defaults to address 0x60.   
   //
   // It returns a standard file descriptor.
   // 
   fd = setup_mpu6050(0x68);

   cout << "Init result: "<< fd << endl;

   uint8_t time_before = millis();

   for(int i = 0; i < 10000; i++)
   {
      imu_data_t data;

      gyroAccelRead(fd, &data);

      printf("%.2f\t%.2f\t%.2f\t\t%.2f\t%.2f\t%.2f\n", data.accX, data.accY, data.accZ, data.gyroX, data.gyroY, data.gyroZ);

      delay(1000);
   }

   printf("Elapsed millis: %d\n", millis() - time_before);

   return 0;
}
