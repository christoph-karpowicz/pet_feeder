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
    DDRD |= (1 << PD5);
}

void servo_on() {
    OCR1A = SERVO_ON_VALUE;
}

void servo_off() {
    OCR1A = SERVO_OFF_VALUE;
}

bool is_servo_on() {
    return OCR1A != SERVO_OFF_VALUE;
}