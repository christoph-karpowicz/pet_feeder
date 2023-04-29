#ifndef I2C_H
#define I2C_H

void I2C_init();
void I2C_send(uint8_t addr, uint8_t data, bool *error_occurred);
void init_RTC_clock(bool *error_occurred);

#endif