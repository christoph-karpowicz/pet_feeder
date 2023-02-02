#include <avr/io.h>
#include <stdbool.h>
#include "i2c.h"

#define DS1307_W 0xD0
#define I2C_START 0x08
#define I2C_WRITE_ADDR 0x18
#define I2C_WRITE_BYTE 0x28
#define I2C_STATUS_REG (TWSR & 0xF8)

void I2C_init() {
    // 1000000/(16+2*12*4) = 100kHz
    TWSR = (1 << TWPS0);
    TWBR = 12;
}

void I2C_start(bool *error_occurred) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    if (I2C_STATUS_REG != I2C_START)
        *error_occurred = true;
}

void I2C_stop() {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

void I2C_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_send(uint8_t addr, uint8_t data, bool *error_occurred) {
    I2C_start(error_occurred);
    I2C_write(DS1307_W);
    if (I2C_STATUS_REG != I2C_WRITE_ADDR)
        *error_occurred = true;
    I2C_write(addr);
    if (I2C_STATUS_REG != I2C_WRITE_BYTE)
        *error_occurred = true;
    I2C_write(data);
    if (I2C_STATUS_REG != I2C_WRITE_BYTE)
        *error_occurred = true;
    I2C_stop();
}