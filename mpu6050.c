#include <iostream>
#include <errno.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include "mpu6050.h"

using namespace std;

#define MPU6050_I2CADDR_DEFAULT                                                \
  0x68                         ///< MPU6050 default i2c address w/ AD0 high
#define MPU6050_DEVICE_ID 0x68 ///< The correct MPU6050_WHO_AM_I value

#define MPU6050_SELF_TEST_X                                                    \
  0x0D ///< Self test factory calibrated values register
#define MPU6050_SELF_TEST_Y                                                    \
  0x0E ///< Self test factory calibrated values register
#define MPU6050_SELF_TEST_Z                                                    \
  0x0F ///< Self test factory calibrated values register
#define MPU6050_SELF_TEST_A                                                    \
  0x10 ///< Self test factory calibrated values register
#define MPU6050_SMPLRT_DIV 0x19  ///< sample rate divisor register
#define MPU6050_CONFIG 0x1A      ///< General configuration register
#define MPU6050_GYRO_CONFIG 0x1B ///< Gyro specfic configuration register
#define MPU6050_ACCEL_CONFIG                                                   \
  0x1C ///< Accelerometer specific configration register
#define MPU6050_INT_PIN_CONFIG 0x37 ///< Interrupt pin configuration register
#define MPU6050_INT_ENABLE 0x38     ///< Interrupt enable configuration register
#define MPU6050_INT_STATUS 0x3A     ///< Interrupt status register
#define MPU6050_WHO_AM_I 0x75       ///< Divice ID register
#define MPU6050_SIGNAL_PATH_RESET 0x68 ///< Signal path reset register
#define MPU6050_USER_CTRL 0x6A         ///< FIFO and I2C Master control register
#define MPU6050_PWR_MGMT_1 0x6B        ///< Primary power/sleep control register
#define MPU6050_PWR_MGMT_2 0x6C ///< Secondary power/sleep control register
#define MPU6050_TEMP_H 0x41     ///< Temperature data high byte register
#define MPU6050_TEMP_L 0x42     ///< Temperature data low byte register
#define MPU6050_ACCEL_OUT 0x3B  ///< base address for sensor data reads
#define MPU6050_MOT_THR 0x1F    ///< Motion detection threshold bits [7:0]
#define MPU6050_MOT_DUR                                                        \
  0x20 ///< Duration counter threshold for motion int. 1 kHz rate, LSB = 1 ms

int setup_mpu6050(uint8_t addr) {
   
   int fd, result;
   //fd = wiringPiI2CSetup(addr);
   fd = wiringPiI2CSetupInterface("/dev/i2c-0", addr);

   //extern int wiringPiI2CReadReg8       (int fd, int reg) ;

   int whoami = wiringPiI2CReadReg8(fd, MPU6050_WHO_AM_I);
   printf("whoami: %x\n", whoami);

   //Adafruit_BusIO_Register(i2c_dev, MPU6050_SMPLRT_DIV, 1);




  wiringPiI2CWriteReg8(fd, MPU6050_PWR_MGMT_1, 0x81);

  while((wiringPiI2CReadReg8(fd, MPU6050_PWR_MGMT_1) & 0x80) != 0){
	  delay(1);
	  printf("resetting...\n");
  }
  printf("done reset\n");
  delay(100);

  wiringPiI2CWriteReg8(fd, MPU6050_SIGNAL_PATH_RESET, 0x07);

  delay(100);

   //printf("whoami V2: %x\n", wiringPiI2CReadReg8(fd, MPU6050_WHO_AM_I));
   //uint8_t pwr_mgmt_1 = wiringPiI2CReadReg8(fd, MPU6050_PWR_MGMT_1) | 0x01;
   //printf("pwr_mgmt_1: %x\n", pwr_mgmt_1);
   //wiringPiI2CWriteReg8(fd, MPU6050_PWR_MGMT_1, pwr_mgmt_1); //gyro x reference for pll



   wiringPiI2CWriteReg8(fd, MPU6050_SMPLRT_DIV, 0);

   wiringPiI2CWriteReg8(fd, MPU6050_CONFIG, 0x0);//default already
   //tGyroRange(MPU6050_RANGE_500_DEG);
   
   wiringPiI2CWriteReg8(fd, MPU6050_GYRO_CONFIG, 0x08); //+/- 500deg/s
   wiringPiI2CWriteReg8(fd, MPU6050_ACCEL_CONFIG, 0x18);  //+/-16g

   wiringPiI2CWriteReg8(fd, MPU6050_PWR_MGMT_1, 0x01);

   printf("ACCEL CONFIG: %x\n", wiringPiI2CReadReg8(fd, MPU6050_ACCEL_CONFIG));  
   printf("GYRO CONFIG: %x\n", wiringPiI2CReadReg8(fd, MPU6050_GYRO_CONFIG));

   //printf("whoami V3: %x\n", wiringPiI2CReadReg8(fd, MPU6050_WHO_AM_I));
  delay(100);



   return fd;
}


void gyroAccelRead(int fd, imu_data_t* data) {

 //  int wiringPiI2CReadBlockData  (int fd, int reg, uint8_t *values, uint8_t size);


   uint8_t buffer[14];
   if(wiringPiI2CReadBlockData(fd, MPU6050_ACCEL_OUT, buffer, 14) < 0) {
	   printf("ReadBlockData() failed\n");
   }

  //for(int i=0;i<14;i++) { printf("%02x\t", buffer[i]);}
  //printf("\n");

  int16_t rawAccX = buffer[0] << 8 | buffer[1];
  int16_t rawAccY = buffer[2] << 8 | buffer[3];
  int16_t rawAccZ = buffer[4] << 8 | buffer[5];

  int16_t rawTemp = buffer[6] << 8 | buffer[7];

  int16_t rawGyroX = buffer[8] << 8 | buffer[9];
  int16_t rawGyroY = buffer[10] << 8 | buffer[11];
  int16_t rawGyroZ = buffer[12] << 8 | buffer[13];

  float accel_scale = 16384;
  // setup range dependant scaling
  data->accX = ((float)rawAccX) / accel_scale;
  data->accY = ((float)rawAccY) / accel_scale;
  data->accZ = ((float)rawAccZ) / accel_scale;


  float gyro_scale = 131;
  data->gyroX = ((float)rawGyroX) / gyro_scale;
  data->gyroY = ((float)rawGyroY) / gyro_scale;
  data->gyroZ = ((float)rawGyroZ) / gyro_scale;



  //printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", accX, accY, accZ, gyroX, gyroY, gyroZ);
}

