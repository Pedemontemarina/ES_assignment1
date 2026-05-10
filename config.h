#ifndef CONFIG_H
#define CONFIG_H

#include "xc.h"
#include "stdlib.h"
#include "stdio.h"

// UART1 pin remapping
#define UART1_RX_RPIN   75   // UART1 RX mapped to RPI75 (RD11, input)
#define UART1_TX_RPIN   64   // UART1 TX mapped to RP64  (RD0,  output)

// SPI1 pin remapping
#define SPI1_MISO_RPIN   17   // MISO        mapped to RPI17 (RA1,  input)
#define SPI1_MOSI_RPIN   109  // MOSI        mapped to RP109 (RF13, output)
#define SPI1_SCK_RPIN    108  // SCK1        mapped to RP108 (RF12, output)

// Chip Select

#define ACC_CS_LAT     LATBbits.LATB3   // Accelerometer CS → output latch  (RB3)
#define ACC_CS_TRIS    TRISBbits.TRISB3 // Accelerometer CS → direction register
#define ACC_CHIP_ID    0xFA             // Expected chip ID (datasheet: 0xFA, read: 0xFD)

#define GYR_CS_LAT     LATBbits.LATB4   // Gyroscope CS → output latch  (RB4)
#define GYR_CS_TRIS    TRISBbits.TRISB4 // Gyroscope CS → direction register
#define GYR_CHIP_ID    0x0F             // Expected chip ID (datasheet: 0x0F, read: 0x07)

#define MAG_CS_LAT     LATDbits.LATD6   // Magnetometer CS → output latch  (RD6)
#define MAG_CS_TRIS    TRISDbits.TRISD6 // Magnetometer CS → direction register
#define MAG_CHIP_ID    0x32             // Expected chip ID (datasheet: 0x32, read: 0xFF)


// LED
#define LD1_LAT     LATAbits.LATA0       // LD1 connected to RA0       
#define LD1_TRIS    TRISAbits.TRISA0

#define LD2_LAT     LATGbits.LATG9       // LD2 connected to RG9        
#define LD2_TRIS    TRISGbits.TRISG9

#endif

// SPI1
// SCK/RF12  -> RP108 
// MISO/RA1  -> RPI17 
// MOSI/RF13 -> RP109 
// SDA/RD9   -> RPI74
// SCL/RD10  -> RPI73


// SPI2
// MISO/RG7 -> RPI119
// MOSI/RG8 -> RP120
// SCK/RG6  -> RP118
// SCL/RD10 -> RPI74
// SDA/RD9  -> RPI73