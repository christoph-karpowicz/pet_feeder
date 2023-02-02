#ifndef I2C_H
#define I2C_H

void I2C_init();
void I2C_start(bool *error_occurred);
void I2C_stop();
void I2C_write(uint8_t data);
void I2C_send(uint8_t addr, uint8_t data, bool *error_occurred);

#endif