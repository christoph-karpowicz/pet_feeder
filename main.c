#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define KEY_PRESSED !(PIND & (1 << PD3))
#define PERIOD 20 // 28800

volatile uint8_t timer_seconds;
volatile uint8_t timer_ticks;

void yellow_led_on() {
    PORTC |= (1 << PC0);
}

void yellow_led_off() {
    PORTC &= !(1 << PC0);
}

ISR(INT1_vect) {
    yellow_led_off();
    OCR1A = 0;
}

ISR(TIMER1_COMPA_vect) {
    timer_ticks++;
    if (timer_ticks >= 50) {
        timer_seconds++;
        timer_ticks = 0;
    }
    if (timer_seconds == PERIOD) {
        if (OCR1A == 0) {
            yellow_led_on();
            OCR1A = 800;
        }
        timer_seconds = 0;
    }
}

int main(void) {
    // Button
    DDRD &= !(1 << PD3);
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

    // Interrupts
    // Turn on the Timer/Counter1 Output Compare A match interrupt
    TIMSK |= (1 << OCIE1A);
    // Turn on interrupts on pin INT1
    GICR |= (1 << INT1);
    // Generate INT1 interrupt on falling egde
    MCUCR |= (1 << ISC11);

    sei();
    _delay_ms(1000);
    while (1) {
    }
    return 0;
}