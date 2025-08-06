#include <iostream>
#include <errno.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>

struct imu_data_t {
	float accX, accY, accZ;
	float gyroX, gyroY, gyroZ;

} ;

int setup_mpu6050(uint8_t addr);

void gyroAccelRead(int fd, imu_data_t* data);

