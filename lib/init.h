#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#ifndef INIT_H
#define INIT_H

void disable_JTAG();
void disable_analog_comp();
void init_interrupts();

#endif