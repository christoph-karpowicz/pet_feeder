#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#define KEY_PRESSED !(PIND & (1 << PD6))

void yellow_led_on() {
    PORTC |= (1 << PC0);
}

void yellow_led_off() {
    PORTC &= !(1 << PC0);
}

int main(void) {
    // Button
    DDRD &= !(1 << PD6);
    // Yellow LED
    DDRC |= (1 << PC0);

    // Servo control
    // Fast PWM Waveform Generation Mode
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);
    // 1 timer divider
    TCCR1B |= (1 << CS10);
    // Set output to OCR1A
    TCCR1A |= (1 << COM1A1);
    // TOP value
    ICR1 = 19999;
    OCR1A = 0;
    DDRD |= (1 << PD5);

    _delay_ms(2000);
    yellow_led_on();
    while (1) {
        if (KEY_PRESSED) {
            OCR1A = 800;
            yellow_led_off();
        } else if (OCR1A != 0) {
            OCR1A = 0;
            yellow_led_on();
        }
    }
    return 0;
}