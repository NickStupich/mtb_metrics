#include <iostream>
#include <errno.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>

typedef struct {
	float accX, accY, accZ;
	float gyroX, gyroY, gyroZ;

} imu_data_t;

int setup_mpu6050(uint8_t addr);

void gyroAccelRead(int fd, imu_data_t* data);

