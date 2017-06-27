#include "mbed.h"
#include "adns9800.h"
#include "firmware.h"

ADNS9800::ADNS9800(PinName MOSI, PinName MISO, PinName SCK, PinName NCS):mySPI(MOSI, MISO, SCK), ncs(NCS)
{
    // initialize
    initComplete = 0;
    mySPI.format(8, 3);
    mySPI.frequency(1000000);
}

void ADNS9800::initialize()
{
    comm_end();
    comm_begin();
    comm_end();
        
        
    write_reg(REG_Configuration_I,0x09);
    write_reg(REG_Power_Up_Reset, 0x5a); // force reset
    wait_ms(50);

    // read registers and discard data
    read_reg(REG_Motion);
    read_reg(REG_Delta_X_L);
    read_reg(REG_Delta_X_H);
    read_reg(REG_Delta_Y_L);
    read_reg(REG_Delta_Y_H);
    
    // upload firmware
    printf("Uploading firmware... \r\n");
    upload_firmware();
    wait_ms(10);
    printf("Done... \r\n");
    
    // enable laser(bit 0 = 0b), in normal mode (bits 3,2,1 = 000b)
    // reading the actual value of the register is important because the real
    // default value is different from what is said in the datasheet, and if you
    // change the reserved bytes (like by writing 0x00...) it would not work.
    uint8_t laser_ctrl0 = read_reg(REG_LASER_CTRL0);
    write_reg(REG_LASER_CTRL0, laser_ctrl0 & 0xf0);

    wait_ms(1);

    // initialized!
    initComplete = 1;
}


void ADNS9800::comm_begin()
{
    ncs = 0;
}

void ADNS9800::comm_end()
{
    ncs = 1;
}

void ADNS9800::reset_movePin()
{
    // writes movement register to reset pin
    write_reg(REG_Motion, 1);
}

uint8_t ADNS9800::read_reg(uint8_t reg_addr)
{
    uint8_t val;
    comm_begin();
    mySPI.write(reg_addr);      // send register address to read
    wait_us(100);
    val = mySPI.write(0x00);    // read value
    comm_end();
    return val;
}

void ADNS9800::write_reg(uint8_t reg_addr, uint8_t val)
{
    comm_begin();
    mySPI.write(reg_addr | 0x80);
    mySPI.write(val);
    wait_us(20);
    comm_end();
    wait_us(100);
}

void ADNS9800::upload_firmware()
{
    write_reg(REG_Configuration_IV, 0x02);
    write_reg(REG_SROM_Enable, 0x1d);
    wait_ms(10);
    
    write_reg(REG_SROM_Enable, 0x18);
    comm_begin();

    mySPI.write(REG_SROM_Load_Burst | 0x80);
    wait_us(15);

    uint8_t c;
    for (int i=0; i<FIRMWARE_LENGTH; i++) {
        c = firmware_data[i];
        mySPI.write(c);
        wait_us(15);
    }
    comm_end();
}

void ADNS9800::getMovement(uint8_t *mot, int* x, int* y)
{
    uint8_t dx_lo, dx_hi, dy_lo, dy_hi;
    
    if(initComplete == 1)
    {
        comm_begin();
        *mot = read_reg(REG_Motion);
        dx_lo = read_reg(REG_Delta_X_L);
        dx_hi = read_reg(REG_Delta_X_H);
        dy_lo = read_reg(REG_Delta_Y_L);
        dy_hi = read_reg(REG_Delta_Y_H);
        comm_end();
        
        *x = (dx_hi << 8 | dx_lo);
        *y = (dy_hi << 8 | dy_lo);        
    }
}

void ADNS9800::read_burst(int* movement, int* observe, int* delta_x, int* delta_y)
{
    uint8_t dx_lo, dx_hi, dy_lo, dy_hi;
    
    // reads multiple registers in a burst
    comm_begin();
    mySPI.write(REG_Motion_Burst);
    wait_us(500);
    
    // first reg is movement, followed by obervation, dx_lo, dx_hi, dy_lo, dy_hi, 
    // squal, pixel_sum, max_px, min_px, shutter_upper, shutter_lower, frame_period_upper, frame_period_lower
    // we only read the first 6 bytes
    
    *movement = mySPI.write(0);
    *observe = mySPI.write(0);
    dx_lo = mySPI.write(0);
    dx_hi = mySPI.write(0);
    dy_lo = mySPI.write(0);
    dy_hi = mySPI.write(0);
    
    comm_end();
    
    *delta_x = (dx_hi << 8 | dx_lo);
    *delta_y = (dy_hi << 8 | dy_lo);    
}