#ifndef IMU_HANDLER_H
#define IMU_HANDLER_H
#include <stdint.h> 
#include <math.h>

typedef enum {
    IMU_ACC,
    IMU_GYR,
    IMU_MAG
} imu_device_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} accel_data_t;

typedef struct {
    float roll;
    float pitch;
} angle_data_t;


#define RAD_TO_DEG (180.0f / M_PI)

void imu_init(void);
void imu_setup(void);
void imu_write_register(imu_device_t dev, uint8_t reg, uint8_t value);
uint8_t imu_read_register(imu_device_t dev, uint8_t reg);
uint8_t imu_read_chip_id(imu_device_t dev);
int16_t imu_read_mag_x(void);

void imu_set_sleep(imu_device_t dev);
void imu_set_active(imu_device_t dev);

// functions Assignment1 
void imu_read_acc(accel_data_t *data);
void imu_set_bandwidth(uint8_t bandwidth_value);
void imu_roll_pitch(const accel_data_t *acc, angle_data_t *angles);

#endif