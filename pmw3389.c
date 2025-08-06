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
#include "SROM.h"

int spi_fd;
char spiDevice[50] = "/dev/spidev0.0";
static uint8_t spiBPW = 8;
static uint32_t spiSpeed = 2000000;


static int PIN_CS = 22;

volatile int32_t x_pos = 0, y_pos = 0;
int32_t readCount = 0;

uint8_t readReg(uint8_t reg);
void writeReg(uint8_t reg, uint8_t val);

int pmw_spiClose()
{
	gpioTerminate();
  return close(spi_fd);
}

int pmw_spiOpen(char* dev)
{
  if((spi_fd = open(dev, O_RDWR)) < 0)
  {
    printf("error opening spi :%s\n",dev);
    return -1;
  }


   char spiMode = SPI_MODE_3;
   if (ioctl(spi_fd, SPI_IOC_WR_MODE, &spiMode) < 0)
   {
	  printf("Failed to set spi mode\n");
      close(spi_fd);
      return -1;
   }

   char msbFirst = 0;
   if(ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &msbFirst) < 0)
   {
	   printf("Failed to set msb first\n");
	   close(spi_fd);
	   return -1;
   }

   if(gpioInitialise() < 0)
   {
	   printf("Failed to init pigpio\n");
	   return -1;
   }

   if(gpioSetMode(PIN_CS, PI_OUTPUT) != 0)
   {
	   printf("Failed to set CS Pin to Output\n");
	   return -1;
   }

  return 0;
}

void delayMicroseconds(uint32_t us)
{
	struct timeval start, current;
	gettimeofday(&start, 0);

	while(1){
		gettimeofday(&current, 0);
		uint32_t elapsed_us = (current.tv_usec - start.tv_usec) + 1000000 * (current.tv_sec - start.tv_sec);
		if(elapsed_us >= us) break;
	}
}

uint8_t SPIWrite(uint8_t val)
{
	uint8_t spiBufTx[4];
	uint8_t spiBufRx[4];

	struct spi_ioc_transfer spi;
	memset(&spi, 0, sizeof(spi));
	spiBufTx[0] = val;
	spi.tx_buf = (unsigned long) spiBufTx;
	spi.rx_buf = (unsigned long) spiBufRx;
	spi.len = 1;
	spi.delay_usecs = 0;
	spi.speed_hz = spiSpeed;
	spi.bits_per_word = spiBPW;

	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi);

	return spiBufRx[0];
}
		
		
void writeReg(uint8_t reg, uint8_t data)
{
	gpioWrite(PIN_CS, 0);

	SPIWrite(reg | 0x80);
	delayMicroseconds(10);

	SPIWrite(data);

	delayMicroseconds(35);
	gpioWrite(PIN_CS, 1);
	delayMicroseconds(180);
}

void upload_firmware()
{
	printf("Uploading firmware...\n");

	writeReg(Config2, 0x20);
	writeReg(SROM_Enable, 0x1d);
	delayMicroseconds(10000);
	writeReg(SROM_Enable, 0x18);

	gpioWrite(PIN_CS, 0);
	
	SPIWrite(SROM_Load_Burst | 0x80);
	delayMicroseconds(15);

	for(int i=0;i<firmware_length;i++)
	{
		SPIWrite(firmware_data[i]);
		delayMicroseconds(15);
	}

	readReg(SROM_ID);
	writeReg(Config2, 0x00);
	writeReg(Config1, 0x15);

	gpioWrite(PIN_CS, 1);
	printf("Done\n");
}

void performStartup()
{
	writeReg(Power_Up_Reset, 0x5a);
	delayMicroseconds(50000);

	readReg(Motion);
	readReg(Delta_X_L);
	readReg(Delta_X_H);
	readReg(Delta_Y_L);
	readReg(Delta_Y_H);

	upload_firmware();
	delayMicroseconds(10);

	int cpi = 50000 / 50;
	writeReg(Resolution_L, (cpi & 0xff));
	writeReg(Resolution_H, (cpi >> 8) & 0xff);
}

uint8_t readReg(uint8_t reg)
{
  
  gpioWrite(PIN_CS, 0);

  SPIWrite(reg & 0x7F);
  
  delayMicroseconds(100);
 
  uint8_t result = SPIWrite(0);
  delayMicroseconds(1);
  gpioWrite(PIN_CS, 1);
  delayMicroseconds(19);

  return result;
}

int32_t convTwosComp16(int32_t x)
{
	if(x & 0x8000){
		x = -((x ^ 0xffff)+1);
	}

	return x;
}

int PMW3389_Setup()
{

	  if(pmw_spiOpen(spiDevice)!= 0)
  {
	  printf("failed to open!\n");
	  pmw_spiClose();
	  return 0;
  }

  performStartup();

  uint8_t regToRead[3] = {0x0, 0x01, 0x3f};
  for(int i=0;i<3;i++){
    uint8_t x = readReg(regToRead[i]);
    printf("Read[%x] = %x\n", regToRead[i], x);
  }


  uint8_t regTest = 0x10;
  printf("Read[%x] = %x\n", regTest, readReg(regTest));

  writeReg(regTest, 0x42);
  printf("Write[%x] = 0x42\n");

  printf("Read[%x] = %x\n", regTest, readReg(regTest));

  writeReg(regTest, 0x20);
  printf("Write[%x] = 0x20\n");
  printf("Read[%x] = %x\n", regTest, readReg(regTest));


 writeReg(Motion_Burst, 0x00);
 return 1;
 
}

void ReadMotion(int *x, int *y, uint8_t *squal, uint8_t *motion)
{
  gpioWrite(PIN_CS, 0);

  SPIWrite(Motion_Burst);
  delayMicroseconds(35);

  uint8_t burstBuffer[12] = {0};
 
  for(int i=0;i<12;i++)
  {
	  burstBuffer[i] = SPIWrite(0);
  }
  delayMicroseconds(1);

  gpioWrite(PIN_CS, 1);
  
  
  uint16_t xl = burstBuffer[2];
  uint16_t xh = burstBuffer[3];
  uint16_t yl = burstBuffer[4];
  uint16_t yh = burstBuffer[5];

  *squal = burstBuffer[6];
  uint8_t rawdataSum = burstBuffer[7];
  *motion = burstBuffer[0];

  int32_t dx = convTwosComp16(xl | (xh << 8));
  int32_t dy = convTwosComp16(yl | (yh << 8));

  x_pos += dx;
  y_pos += dy;
  readCount++;

  *x = x_pos;
  *y = y_pos;


}


void PMW3389_Shutdown(){

	
  pmw_spiClose();
}
