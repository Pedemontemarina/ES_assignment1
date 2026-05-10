#include "config.h"
#include "SPI_handler.h"


// initialize SPI1 pins and peripheral configuration
void spi_init_pins(void)
{
    // // MISO  input, mapped to RPI17 (RA1)
    TRISAbits.TRISA1 = 1;
    RPINR20bits.SDI1R = SPI1_MISO_RPIN;

    // MOSI   output, mapped to RP109 (RF13)
    TRISFbits.TRISF13 = 0;
    RPOR12bits.RP109R = 0b000101; // MOSI code function

    // SCK1  output, mapped to RP108 (RF12)
    TRISFbits.TRISF12 = 0;
    RPOR11bits.RP108R = 0b000110; // SCK1 output

    // Chip Select pins ? output, all deactivated (CS idle = high)
    ACC_CS_TRIS = 0; 
    GYR_CS_TRIS = 0; 
    MAG_CS_TRIS = 0; 

    ACC_CS_LAT = 1;
    GYR_CS_LAT = 1;
    MAG_CS_LAT = 1;

    SPI1STATbits.SPIEN = 0;   // disable SPI before configuration

    SPI1CON1bits.MSTEN = 1;   // master mode
    SPI1CON1bits.MODE16 = 0;  // 8-bit mode
    SPI1CON1bits.SMP = 0;     // sample in middle of clock period
    // if you set 1, sampling is at the end of the clock period.
    
    // Already 0 by default:
    //SPI1CON1bits.CKP = 0;     // clock idle low
    //SPI1CON1bits.CKE = 1;     // data changes on rising edge 
  
    // Fspi =Fcy / (PPRE * SPRE) = 72 MHz / (1 * 2) = 36 MHz
    /* SPI1CON1bits.SPRE = 3; //0b110;  secondary prescaler 2:1 
    SPI1CON1bits.PPRE = 3; //0b11;   primary prescaler 1:1  */

    // IMU can support Fsck up to 15 MHz, I choose to use 4.5 MHz
    SPI1CON1bits.PPRE = 1;   // 16:1 primary prescaler
    SPI1CON1bits.SPRE = 7;   // 1:1 secondary prescaler 

    //SPI1STATbits.SPIROV = 0;  // clear overflow
    SPI1STATbits.SPIEN = 1;   // enable SPI
}



/*
  Send one byte over SPI and return the received byte.
  SPI is full-duplex: every write produces a read data Byte to send
  return Byte received during transmission
 */

uint8_t spi_write(uint8_t data) {
    while (SPI1STATbits.SPITBF == 1);  // Wait until TX buffer is free
    SPI1BUF = data;                     // Send byte
    while (SPI1STATbits.SPIRBF == 0);  // Wait until RX buffer has data
    return SPI1BUF;                     // Return received byte
}


/*
  Read one register from a SPI sensor 
  Sets MSB=1 to signal read operation in the address byte, then clocks out 0x00 to receive data
  addr -> Register address
  return -> Register value
 */

uint8_t spi_read_register(uint8_t addr)
{
    spi_write(addr | 0x80); // MSB=1 ? read operation
    return spi_write(0x00); // Clock out zeros to receive register value
}

/*
  Write one register to a SPI sensor (send address byte followed by value byte)
  Clears MSB to signal write operation
  addr -> Register address
  value -> Value to write
 */

void spi_write_register(uint8_t addr, uint8_t value)
{
    spi_write(addr & 0x7F); // MSB=0 ? write operation
    spi_write(value);       // Send value
}

