#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <avr/sleep.h>
#include "i2c.h"

#define DISPLAY_OUTPUT_PORT PORTA
#define DISPLAY_CONTROL_PORT PORTC

#define BUTTON_STANDBY_TIMER_TOP 50
#define BUTTON_PRESS_MANUAL_STOP 1
#define BUTTON_PRESS_RESET_TIMER 2
#define BUTTON_PRESS_TEST_MODE 3
#define SERVO_ON_MAX_SECONDS 5
#define PERIOD_TEST_MODE 10
#define PERIOD 28800

uint8_t display_coms[3] = {PC7, PC6, PC5};
uint8_t display_digits[10] = {0x03, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x01, 0x09};

volatile uint16_t timer_seconds;
volatile uint8_t servo_on_seconds;

volatile bool button_active;
volatile uint8_t button_press_counter;
volatile uint8_t button_wait_timer;

volatile bool is_sleeping;

volatile bool display_enabled;
volatile uint8_t display_timer;

bool test_mode;
bool error_occurred;

void enable7segment() {
    display_enabled = true;
    display_timer = 0;
    // set a 64 divider for Timer0 and enable it
    TCCR0 |= (1 << CS01) | (1 << CS00);
    // enable Timer0 interrupt
    TIMSK |= (1 << OCIE0);
}

void disable7segment() {
    TCCR0 &= !((1 << CS01) | (1 << CS00));
    TIMSK &= !(1 << OCIE0);
    DISPLAY_OUTPUT_PORT = 0xFF;
    display_enabled = false;
}

void displayTime() {
    wake_up();
    enable7segment();
}

static inline void led_on() {
    PORTD |= (1 << PD7);
}

static inline void led_off() {
    PORTD &= !(1 << PD7);
}

static inline void servo_on() {
    wake_up();
    disable7segment();
    OCR1A = 1400;
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
            displayTime();
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
            timer_seconds = 0;
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
    if (!button_active && button_press_counter == 0 && !is_servo_on() && !display_enabled) {
        is_sleeping = true;
    }
}

void wake_up() {
    is_sleeping = false;
}

// 7 segment control
ISR(TIMER0_COMP_vect) {
    static uint8_t active_com = 0;
    DISPLAY_CONTROL_PORT &= 0x1F;
    DISPLAY_CONTROL_PORT |= (1 << display_coms[active_com]);
    DISPLAY_OUTPUT_PORT = display_digits[1];
    active_com++;
    if (active_com > 2) {
        active_com = 0;
    }
}

// External interrupt caused by a limit switch
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

    if (display_enabled) {
        display_timer++;
        if (display_timer > 5) {
            disable7segment();
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

void initServoControl() {
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

// 7 segment display LED output
void init7segment() {
    // LED output pins
    DDRA = 0xFF;
    // driver pins
    DDRC |= (1 << PC5) | (1 << PC6) | (1 << PC7);
    // set CTC mode for Timer0
    TCCR0 |= (1 << WGM01);
    // set Output Compare Register
    OCR0 = 125;
    // turn all segments off
    DISPLAY_OUTPUT_PORT = 0xFF;
}

void init() {
    disable_JTAG();
    disable_analog_comp();

    // Button
    DDRD &= !(1 << PD3);
    // Yellow LED
    DDRD |= (1 << PD7);

    initServoControl();

    // Interrupts
    // Turn on interrupts on pins INT0, INT1 and INT2
    GICR |= (1 << INT0) | (1 << INT1) | (1 << INT2);
    // Generate INT0 interrupt on falling egde
    MCUCR |= (1 << ISC01);
    // Generate INT1 interrupt on low level
    // MCUCR &= !((1 << ISC10) | (1 << ISC11));

    I2C_init();
    // Enable SQW/OUT with frequency of 1Hz
    I2C_send(0x07, 0x10, &error_occurred);
    _delay_ms(100);
    I2C_send(0x00, 0, &error_occurred);

    init7segment();

    _delay_ms(1000);
    sei();
}

int main(void) {
    init();
    set_sleep_mode(SLEEP_MODE_IDLE);
    while (1) {
        if (is_sleeping) {
            sleep_enable();
            sleep_cpu();
            sleep_disable();
        }
    }
    return 0;
}