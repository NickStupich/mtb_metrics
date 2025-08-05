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


int main(int argc, char* argv[])
{
  FILE * outputFp = NULL;
  struct timeval initialTime;

  volatile int32_t x_pos=0, y_pos=0;

  PMW3389_Setup();

  if(argc > 2 && strcmp(argv[1], "print") == 0)
  {
	  bool squal = 0;


     if(strcmp(argv[2], "squal") == 0) {
	     squal = 1;
     } else {
	     printf("print for '%s' not supported\n", argv[2]);
	     return 1;
     }

     while(1) {
	if(squal) {
		uint8_t squal = readReg(SQUAL);
		printf("%d\n", squal);
	}

	sleep(1);
     }


  }

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


  //fprintf(outputFp, "t,x,y\n");
  fprintf(outputFp, "t,x,y,squal,motion\n");


  struct sigaction sigact;
  sigact.sa_handler = intHandler;
  sigaction(SIGINT, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);



 struct timeval currenttime;
 gettimeofday(&initialTime, 0);

for(int i=0;!shutdown;i++)
  {
    ReadMotion(&x_pos, &y_pos);

  if(outputFp != NULL)
  {
    struct timeval currentTime;
    gettimeofday(&currentTime, 0);
    uint64_t elapsed_us = (currentTime.tv_usec - initialTime.tv_usec) + 1000000L * ((uint64_t)(currentTime.tv_sec - initialTime.tv_sec));
  
    //fprintf(outputFp, "%lld,%d,%d\n", elapsed_us, x_pos, y_pos);
    fprintf(outputFp, "%lld,%d,%d,%d,%x\n", elapsed_us, x_pos, y_pos, squal, motion);
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
