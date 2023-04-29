#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/sleep.h>
#include "lib/init.h"
#include <util/delay.h>
#include "lib/led.h"
#include "lib/servo.h"
#include "lib/button.h"
#include "lib/i2c.h"
#include "lib/display.h"

#define DISPLAY_CYCLE_SECONDS 2
#define SERVO_ON_MAX_SECONDS 5
#define PERIOD_TEST_MODE 10
#define PERIOD 28800

volatile uint16_t timer_seconds;
volatile uint8_t servo_on_seconds;

extern bool button_active;
extern uint8_t button_press_counter;

volatile bool is_sleeping;

extern bool display_enabled;
extern uint8_t display_cycle;
extern uint8_t display_timer;

bool test_mode;
bool error_occurred;

void prepare_servo_on() {
    wake_up();
    disable_display();
    servo_on_seconds = 0;
}

void put_to_sleep() {
    if (!button_active && button_press_counter == 0 && !is_servo_on() && !display_enabled) {
        is_sleeping = true;
    }
}

void wake_up() {
    is_sleeping = false;
}

// External interrupt caused by a limit switch
ISR(INT0_vect) {
    servo_off();
}

// External interrupt caused by a button
ISR(INT1_vect) {
    wake_up();
    handle_button_press_interrupt();
}

// Internal interrupt caused by Timer/Counter1
ISR(TIMER1_COMPA_vect) {
    handle_button_timer_interrupt(PERIOD, timer_seconds, &error_occurred, &test_mode);
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
        handle_led_blinking(error_occurred, timer_seconds);
    }
    bool past_period = !test_mode && timer_seconds >= PERIOD;
    bool past_period_test_mode = test_mode && timer_seconds >= PERIOD_TEST_MODE;
    if ((past_period || past_period_test_mode) && !error_occurred) {
        if (!is_servo_on()) {
            prepare_servo_on();
            servo_on();
            timer_seconds = 0;
        }
    }

    if (display_enabled) {
        display_timer++;
        if (display_timer % DISPLAY_CYCLE_SECONDS == 0) {
            display_cycle++;
        }
        if (display_cycle > 3) {
            disable_display();
        }
    }
    
    put_to_sleep();
}

// 7 segment control interrupt
ISR(TIMER0_COMP_vect) {
    handle_display_interrupt();
}

void init() {
    disable_JTAG();
    disable_analog_comp();

    init_button();
    init_led();
    init_servo_control();
    init_interrupts();
    init_RTC_clock(&error_occurred);
    init_display();

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