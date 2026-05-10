#include "IMU_handler.h"
#include "SPI_handler.h"
#include "UART_handler.h"
#include "config.h"
#include "timer.h"

/*
  Configure SPI clock polarity/phase for Bosch BMX055
  Must be called before imu_init ? BMX055 requires clock idle LOW (CKP=0, CKE=1)
 */

void imu_setup(void) {
    // Bosch BMX055 SPI protocol require idle LOW
    SPI1STATbits.SPIEN = 0;     // disable SPI to change settings
    SPI1CON1bits.CKE = 1;       // Output data changes on transition from active to idle
    SPI1CON1bits.CKP = 0;       // Idle state for clock is a low level
    SPI1STATbits.SPIEN = 1;     // re-enable SPI
    tmr_wait_ms(TIMER2, 100);
}

/*
  Initialize all three IMU sensors and verify chip IDs.
  Magnetometer starts in sleep mode and must be explicitly woken up, 
  Accelerometer and Gyroscope start in normal mode.

  Then for we checks IDs to see proper initialization and we 
  set active mode for all, default bandwidth for the accelerometer 
  is 1000 Hz (value 15 in register 0x10)
 */


void imu_init(void) {

    uart_send_string("IMU INIT START\r\n");

    // Deselect all sensors (CS idle = high)
    ACC_CS_LAT = 1;
    GYR_CS_LAT = 1;
    MAG_CS_LAT = 1;

     // Magnetometer starts in suspend mode ? must be woken up explicitly
    imu_write_register(IMU_MAG, 0x4B, 0x01);  // Enter sleep mode first
    tmr_wait_ms(TIMER1, 10);                   // Wait for mode transition
    imu_write_register(IMU_MAG, 0x4C, 0x00);  // Enter active mode
    tmr_wait_ms(TIMER1, 10);                   // Wait for stabilization

    // Debug: read and print chip IDs to verify communication
    uint8_t ACC_ID = imu_read_chip_id(IMU_ACC);
    uint8_t GYR_ID = imu_read_chip_id(IMU_GYR);
    uint8_t MAG_ID = imu_read_chip_id(IMU_MAG);


    // check accelerometer ID from datasheet 1111 1010 -> 0xFA
     if (ACC_ID != ACC_CHIP_ID) {
        uart_send_string("Incorrect accelerometer Chip ID\r\n");
        return; // Abort initialization
    }

    // check gyroscope ID from datasheet 0000 1111 -> 0x0F
    if (GYR_ID != GYR_CHIP_ID) {
        uart_send_string("Incorrect gyroscope Chip ID\r\n");
        return;
    }

    // check magnetometer ID from datasheet -> 0x32
    if (MAG_ID != MAG_CHIP_ID) {
        uart_send_string("Incorrect magnetometer Chip ID\r\n");
        return;
    } 

    // if we reach this point, all IDs are correct
    uart_send_string("All Chip IDs are CORRECT!\r\n");

    imu_set_bandwidth(15);  // 1000 Hz default
}


/*
  Select one IMU sensor by pulling its CS low
  Always deselects all sensors first to avoid bus conflicts
  dev -> Sensor to select (IMU_ACC, IMU_GYR, IMU_MAG)
 */

static void imu_select(imu_device_t dev)  
{
    // Deselect all sensors first (CS idle = high)
    ACC_CS_LAT = 1;
    GYR_CS_LAT = 1;
    MAG_CS_LAT = 1;

    // activate only the selected sensor (CS active = low)
    switch (dev) {
        case IMU_ACC: ACC_CS_LAT = 0; break;
        case IMU_GYR: GYR_CS_LAT = 0; break;
        case IMU_MAG: MAG_CS_LAT = 0; break;
    }
}

/*
 Write one byte to a sensor register via SPI
  dev ->  Target sensor
  reg -> Register address (MSB will be forced to 0 = write)
  value -> Value to write
 */

void imu_write_register(imu_device_t dev, uint8_t reg, uint8_t value)
{
    imu_select(dev);
    spi_write(reg & 0x7F);   // write -> MSB = 0
    spi_write(value);
    // deselect
    ACC_CS_LAT = 1;
    GYR_CS_LAT = 1;
    MAG_CS_LAT = 1;
}

/*
  Read one byte from a sensor register via SPI
  dev -> Target sensor
  reg -> Register address (MSB will be forced to 1 = read)
  return Register value
 */

uint8_t imu_read_register(imu_device_t dev, uint8_t reg)
{
    imu_select(dev);
    spi_write(reg | 0x80);   // MSB=1 -> read operation
    uint8_t value = spi_write(0x00); // Clock out zeros to receive data
    // deselect
    ACC_CS_LAT = 1;
    GYR_CS_LAT = 1;
    MAG_CS_LAT = 1;
    return value;
}

/*
  Read chip ID from a sensor
  Magnetometer chip ID is at register 0x40, others at 0x00
  dev -> Target sensor
  return Chip ID byte
 */

uint8_t imu_read_chip_id(imu_device_t dev)
{
    if (dev == IMU_MAG)
        return imu_read_register(dev, 0x40);
    else
        return imu_read_register(dev, 0x00);
}


/*
  Read raw X axis value from magnetometer (magentic field along X)
  Data is 12-bit signed, stored in registers 0x42 (LSB) and 0x43 (MSB)
  return Signed 13-bit value
 */


int16_t imu_read_mag_x(void)
{
    uint8_t lsb = imu_read_register(IMU_MAG, 0x42);
    uint8_t msb = imu_read_register(IMU_MAG, 0x43);
    
    lsb &= 0xF8;  // Mask lower 3 bits (not data)
    int16_t raw = ((int16_t)msb << 8) | lsb; // Combine MSB and LSB
    return raw >> 3;  // Shift right 3 to get 13 useful bits
}

/*
  Read all three accelerometer axes via SPI
  Data is 12-bit signed (two's complement), stored in register pairs:
  X: 0x02 (LSB), 0x03 (MSB)
  Y: 0x04 (LSB), 0x05 (MSB)
  Z: 0x06 (LSB), 0x07 (MSB)

  LSB lower 4 bits are not data ? masked and shifted out
  data -> Pointer to struct where results will be stored

  To get filtered values you have to set 0x00 the 0x13 register (default mode)
 */

void imu_read_acc(accel_data_t *data)
{
    uint8_t lsb, msb;

    // x axis
    lsb = imu_read_register(IMU_ACC, 0x02);
    msb = imu_read_register(IMU_ACC, 0x03);
    lsb &= 0xF0;  // Mask lower 4 bits (not data)
    data->x = ((int16_t)msb << 8 | lsb) >> 4; // Combine and shift to get 12-bit value
    
    // y axis
    lsb = imu_read_register(IMU_ACC, 0x04);
    msb = imu_read_register(IMU_ACC, 0x05);
    lsb &= 0xF0;
    data->y = ((int16_t)msb << 8 | lsb) >> 4;

    // z axis
    lsb = imu_read_register(IMU_ACC, 0x06);
    msb = imu_read_register(IMU_ACC, 0x07);
    lsb &= 0xF0;
    data->z = ((int16_t)msb << 8 | lsb) >> 4;
}

/*
  Set accelerometer low-pass filter bandwidth
  Written to register 0x10 (ACC_BW) pag.58 datasheet, valid values 8?15:
  8=7.81Hz, 9=15.63Hz, 10=31.25Hz, 11=62.5Hz,
  12=125Hz, 13=250Hz, 14=500Hz, 15=1000Hz
  bandwidth_value -> Value between 8 and 15 chosen by the user via UART command
 */

void imu_set_bandwidth(uint8_t bandwidth_value)
{
    imu_write_register(IMU_ACC, 0x10, bandwidth_value);
}


/*
  Compute roll and pitch angles from accelerometer data
  Uses atan2 for full 360� range, result in degrees
  Uses pointers to avoid unnecessary memory copies
   acc  ->  Pointer to raw accelerometer data
   angles -> Pointer to struct where angles will be stored
 */

void imu_roll_pitch(const accel_data_t *acc, angle_data_t *angles)
{
    float ax = (float)acc->x;
    float ay = (float)acc->y;
    float az = (float)acc->z;

     // Roll: rotation around X axis
    angles->roll  = atan2f(ay, az) * RAD_TO_DEG;
    // Pitch: rotation around Y axis
    // Uses sqrt of ay�+az� as denominator for stability near �90�
    angles->pitch = atan2f(-ax, sqrtf(ay*ay + az*az)) * RAD_TO_DEG;
}


