#include <avr/io.h>
#include <stdbool.h>

#define SERVO_ON_VALUE 1400
#define SERVO_OFF_VALUE 0

void init_servo_control() {
    // Fast PWM Waveform Generation Mode
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM12) | (1 << WGM13);
    // Timer divider 1 value
    TCCR1B |= (1 << CS10);
    // Set output to OCR1A
    TCCR1A |= (1 << COM1A1);
    // TOP value
    ICR1 = 19999;
    OCR1A = 0;
    // PD5 is the PWM output
    // and PD6 is used to apply voltage to a MOSFET's gate to power the servo
    DDRD |= (1 << PD5) | (1 << PD6);
}

void servo_on() {
    PORTD |= (1 << PD6);
    OCR1A = SERVO_ON_VALUE;
}

void servo_off() {
    OCR1A = SERVO_OFF_VALUE;
    PORTD &= ~(1 << PD6);
}

bool is_servo_off() {
    return OCR1A == SERVO_OFF_VALUE;
}