#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

#define BUTTON_STANDBY_TIMER_TOP 50
#define BUTTON_PRESS_MANUAL_STOP 1
#define BUTTON_PRESS_RESET_TIMER 2
#define BUTTON_PRESS_MANUAL_SPIN 3
#define SERVO_ON_MAX_SECONDS 5
#define PERIOD 20 // 28800

volatile uint16_t timer_seconds;
volatile uint8_t servo_on_seconds;
volatile uint8_t timer_ticks;

volatile uint8_t button_press_counter;
volatile uint8_t button_wait_timer;

bool error_occurred;

static inline void led_on() {
    PORTD |= (1 << PD7);
}

static inline void led_off() {
    PORTD &= !(1 << PD7);
}

static inline void servo_on() {
    OCR1A = 500;
    servo_on_seconds = 0;
}

static inline void servo_off() {
    OCR1A = 0;
}

void handle_button_press() {
    switch (button_press_counter) {
        case BUTTON_PRESS_MANUAL_STOP:
            servo_off();
            break;
        case BUTTON_PRESS_RESET_TIMER:
            timer_seconds = 0;
            error_occurred = false;
            break;
        case BUTTON_PRESS_MANUAL_SPIN:
            servo_on();
            break;
        default:
            break;
    }
    button_press_counter = 0;
}

void handle_led_blinking() {
    if (error_occurred) {
        if (timer_seconds % 2 != 0) {
            led_on();
        } else {
            led_off();
        }
        return;
    }
    
    switch (timer_seconds) {
        case 1:
            led_on();
            break;
        case 2:
            led_off();
            break;
        case 3:
            led_on();
            break;
        default:
            led_off();
            break;
    }
}

ISR(INT0_vect) {
    servo_off();
}

ISR(INT1_vect) {
    if (button_wait_timer < BUTTON_STANDBY_TIMER_TOP - 10) {
        button_press_counter++;
        button_wait_timer = BUTTON_STANDBY_TIMER_TOP;
    }
}

ISR(TIMER1_COMPA_vect) {
    timer_ticks++;
    if (timer_ticks > 50) {
        timer_seconds++;
        timer_ticks = 0;
        if (OCR1A != 0) {
            servo_on_seconds++;
            if (servo_on_seconds > SERVO_ON_MAX_SECONDS) {
                servo_off();
                error_occurred = true;
            }
        }

        handle_led_blinking();
        if (timer_seconds >= PERIOD && !error_occurred) {
            if (OCR1A == 0) {
                servo_on();
                timer_seconds = 0;
            }
        }
    }

    if (button_wait_timer > 0) {
        button_wait_timer--;
    } else if (button_press_counter > 0 && button_wait_timer == 0) {
        handle_button_press();
    }
}

void disable_JTAG() {
    MCUCSR |= (1 << JTD);
    MCUCSR |= (1 << JTD);
}

void disable_analog_comp() {
    ACSR |= (1 << ACD);
}

void init() {
    disable_JTAG();
    disable_analog_comp();
    
    // Button
    DDRD &= !(1 << PD3);
    // Yellow LED
    DDRD |= (1 << PD7);

    // Servo control
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

    // Interrupts
    // Turn on the Timer/Counter1 Output Compare A match interrupt
    TIMSK |= (1 << OCIE1A);
    // Turn on interrupts on pins INT0 and INT1
    GICR |= (1 << INT0) | (1 << INT1);
    // Generate INT0 interrupt on rising egde
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    // Generate INT1 interrupt on falling egde
    MCUCR |= (1 << ISC11);

    _delay_ms(1000);
    sei();
}

int main(void) {
    init();
    while (1) {
    }
    return 0;
}