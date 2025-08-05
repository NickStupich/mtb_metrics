#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <unistd.h>
#include <pigpio.h>
#include <signal.h>
#include <stdbool.h>


#include "pmw3389.h"
#include "mpu6050.h"




int main(int argc, char* argv[])
{
  FILE * outputFp = NULL;
  struct timeval initialTime;


  PMW3389_Setup();

  if(argc > 1)
  {
	 printf("writing to file: %s\n", argv[1]);
	outputFp = fopen(argv[1], "w");
  }
  else
  {
	  printf("Using default output file\n");
  	outputFp = fopen("/home/nick/accel_data/temp.csv", "w");
  }
  if(outputFp == NULL)
  {
	  printf("Failed to open output file\n");
	  return -1;
  }


  fprintf(outputFp, "t,x,y,squal,motion,acc_l_x,acc_l_y,acc_l_z,gyro_l_x,gyro_l_y,gyro_l_z,acc_u_x,acc_u_y,acc_u_z,gyro_u_x,gyro_u_y,gyro_u_z\n");

  int lower_fd = setup_mpu6050(0x68);
  int upper_fd = setup_mpu6050(0x69);

  if(lower_fd < 0 || upper_fd < 0) {
  	printf("Failed to open mpu6050(s)\n");
  	return -1;
  }

  struct sigaction sigact;
  sigact.sa_handler = intHandler;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);

 struct timeval currenttime;
 gettimeofday(&initialTime, 0);

  volatile int32_t x_pos=0, y_pos=0;
  volatile uint8_t squal=0, motion=0;

  imu_data_t lower_imu, upper_imu;

for(int i=0;!shutdown;i++)
  {
    ReadMotion(&x_pos, &y_pos, &squal, &motion);
    gyroAccelRead(lower_fd, &lower_imu);
    gyroAccelRead(upper_fd, &upper_imu);

  if(outputFp != NULL)
  {
    struct timeval currentTime;
    gettimeofday(&currentTime, 0);
    uint64_t elapsed_us = (currentTime.tv_usec - initialTime.tv_usec) + 1000000L * ((uint64_t)(currentTime.tv_sec - initialTime.tv_sec));
  
    //fprintf(outputFp, "%lld,%d,%d\n", elapsed_us, x_pos, y_pos);
    fprintf(outputFp, "%lld,%d,%d,%d,%x,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,\n", elapsed_us, x_pos, y_pos, squal, motion,
    		lower_imu.accX,lower_imu.accY,lower_imu.accZ,lower_imu.gyroX,lower_imu.gyroY,lower_imu.gyroZ,
    		upper_imu.accX,upper_imu.accY,upper_imu.accZ,upper_imu.gyroX,upper_imu.gyroY,upper_imu.gyroZ);
  }


   if(i % 1000 == 0)
		   {
	printf("%d\t%d\n", x_pos, y_pos);
    }
   
   if(i % 10000 == 0)
   {
	gettimeofday(&currenttime, 0);
	int samplesPerSecond = (int)((i * 1000) / (1000 * (currenttime.tv_sec - initialTime.tv_sec) + (currenttime.tv_sec - initialTime.tv_sec) / 1000));
	printf("SPS: %d\n", samplesPerSecond);

   }

  }

  printf("before shutdown\n");
  fclose(outputFp);
  pmw_spiClose();
  printf("\nshutdown\n");


  return 0;
}



void intHandler(int) {
  printf("action handler\n");
        shutdown=true;
}
