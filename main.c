#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/sleep.h>
#include "lib/init.h"
#include "lib/sleep.h"
#include <util/delay.h>
#include "lib/led.h"
#include "lib/servo.h"
#include "lib/button.h"
#include "lib/i2c.h"
#include "lib/display.h"

#define SERVO_ON_MAX_SECONDS 5
#define TIMER_TOP_TEST_MODE 10
#define TIMER_TOP 28800

volatile uint16_t timer_seconds;
volatile uint8_t servo_on_seconds;

extern bool button_active;
extern uint8_t button_press_counter;

extern bool is_sleeping;

extern bool display_enabled;
extern struct activeDisplay active_display;
extern uint8_t display_cycle;
extern uint8_t display_timer;

bool test_mode;
bool error_occurred;

void prepare_servo_on() {
    wake_up();
    if (display_enabled) {
        disable_display();
    }
    servo_on_seconds = 0;
}

void put_to_sleep() {
    if (!button_active && button_press_counter == 0 && !is_servo_on() && !display_enabled) {
        sleep();
    }
}

uint16_t get_timer_top() {
    return test_mode ? TIMER_TOP_TEST_MODE : TIMER_TOP;
}

// External interrupt caused by a limit switch
ISR(INT0_vect) {
    servo_off();
}

// External interrupt caused by a button
ISR(INT1_vect) {
    wake_up();
    if (display_enabled) {
        disable_display();
    }
    handle_button_press_interrupt();
}

// Internal interrupt caused by Timer/Counter1
ISR(TIMER1_COMPA_vect) {
    handle_button_timer_interrupt(get_timer_top(), &timer_seconds, &error_occurred, &test_mode);
}

// 7 segment control interrupt
ISR(TIMER0_COMP_vect) {
    handle_display_interrupt();
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

    bool past_counter_max = timer_seconds >= get_timer_top();
    if (past_counter_max && !error_occurred) {
        if (!is_servo_on()) {
            prepare_servo_on();
            servo_on();
            timer_seconds = 0;
        }
    }

    if (display_enabled) {
        display_timer++;
        if (display_timer % active_display.seconds_in_cycle == 0) {
            display_cycle++;
        }
        if (display_cycle > active_display.cycles) {
            disable_display();
        }
    }
    
    put_to_sleep();
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

    init_display_greeting();
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