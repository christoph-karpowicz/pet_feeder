#include <avr/io.h>

void disable_JTAG() {
    MCUCSR |= (1 << JTD);
    MCUCSR |= (1 << JTD);
}

void disable_analog_comp() {
    ACSR |= (1 << ACD);
}

void init_interrupts() {
    // Turn on interrupts on pins INT0, INT1 and INT2
    GICR |= (1 << INT0) | (1 << INT1) | (1 << INT2);
    // Generate INT0 interrupt on falling egde
    MCUCR |= (1 << ISC01);
    // Generate INT1 interrupt on low level
    // MCUCR &= !((1 << ISC10) | (1 << ISC11));
}