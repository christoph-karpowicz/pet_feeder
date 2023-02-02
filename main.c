#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/sleep.h>
#include "i2c.h"

#define DS1307_R 0xD1

#define BUTTON_STANDBY_TIMER_TOP 50
#define BUTTON_PRESS_MANUAL_STOP 1
#define BUTTON_PRESS_RESET_TIMER 2
#define BUTTON_PRESS_TEST_MODE 3
#define SERVO_ON_MAX_SECONDS 5
#define PERIOD_TEST_MODE 20
#define PERIOD 28800

volatile uint16_t timer_seconds;
volatile uint8_t servo_on_seconds;

volatile bool button_active;
volatile uint8_t button_press_counter;
volatile uint8_t button_wait_timer;

volatile bool is_sleeping;

bool test_mode;
bool error_occurred;

static inline void led_on() {
    PORTD |= (1 << PD7);
}

static inline void led_off() {
    PORTD &= !(1 << PD7);
}

static inline void servo_on() {
    wake_up();
    OCR1A = 500;
    servo_on_seconds = 0;
}

static inline void servo_off() {
    OCR1A = 0;
}

static inline bool is_servo_on() {
    return OCR1A != 0;
}

void handle_button_press_sequence() {
    disable_PWN_interrupt();
    button_active = false;

    switch (button_press_counter) {
        case BUTTON_PRESS_MANUAL_STOP:
            servo_off();
            break;
        case BUTTON_PRESS_RESET_TIMER:
            wake_up();
            timer_seconds = 0;
            error_occurred = false;
            test_mode = false;
            break;
        case BUTTON_PRESS_TEST_MODE:
            wake_up();
            led_on();
            test_mode = true;
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

// Turns on the Timer/Counter1 Output Compare A match interrupt
void enable_PWN_interrupt() {
    TIMSK |= (1 << OCIE1A);
}

void disable_PWN_interrupt() {
    TIMSK &= !(1 << OCIE1A);
}

void put_to_sleep() {
    if (!button_active && button_press_counter == 0 && !is_servo_on()) {
        is_sleeping = true;
    }
}

void wake_up() {
    is_sleeping = false;
}

// External interrupt caused by a contact
ISR(INT0_vect) {
    servo_off();
}

// External interrupt caused by a button
ISR(INT1_vect) {
    wake_up();
    enable_PWN_interrupt();
    if (button_wait_timer < BUTTON_STANDBY_TIMER_TOP - 10) {
        button_press_counter++;
        button_wait_timer = BUTTON_STANDBY_TIMER_TOP;
    }
    button_active = true;
}

// Internal interrupt caused by Timer/Counter1
ISR(TIMER1_COMPA_vect) {
    if (button_wait_timer > 0) {
        button_wait_timer--;
    } else if (button_press_counter > 0 && button_wait_timer == 0) {
        handle_button_press_sequence();
    }
}

// External interrupt caused by a DS1307 RTC clock
ISR(INT2_vect) {
    timer_seconds++;
    if (is_servo_on()) {
        servo_on_seconds++;
        if (servo_on_seconds > SERVO_ON_MAX_SECONDS) {
            servo_off();
            error_occurred = true;
            test_mode = false;
        }
    }

    if (!test_mode) {
        handle_led_blinking();
    }
    bool past_period = !test_mode && timer_seconds >= PERIOD;
    bool past_period_test_mode = test_mode && timer_seconds >= PERIOD_TEST_MODE;
    if ((past_period || past_period_test_mode) && !error_occurred) {
        if (!is_servo_on()) {
            servo_on();
            timer_seconds = 0;
        }
    }
    put_to_sleep();
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
    // Turn on interrupts on pins INT0, INT1 and INT2
    GICR |= (1 << INT0) | (1 << INT1) | (1 << INT2);
    // Generate INT0 interrupt on rising egde
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    // Generate INT1 interrupt on low level
    // MCUCR &= !((1 << ISC10) | (1 << ISC11));

    I2C_init();
    // Enable SQW/OUT with frequency of 1Hz
    I2C_send(0x07, 0x10, &error_occurred);
    _delay_ms(100);
    I2C_send(0x00, 0, &error_occurred);

    _delay_ms(1000);
    sei();
}

int main(void) {
    init();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    while (1) {
        if (is_sleeping) {
            sleep_enable();
            sleep_cpu();
            sleep_disable();
        }
    }
    return 0;
}