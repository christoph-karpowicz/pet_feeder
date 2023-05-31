#include <avr/io.h>
#include <stdbool.h>
#include "led.h"

#define BUTTON_DATA_DIRECTION_REG DDRD
#define BUTTON_PIN PD3
#define BUTTON_STANDBY_TIMER_TOP 40
#define BUTTON_PRESS_MANUAL_STOP 1
#define BUTTON_PRESS_RESET_TIMER 2
#define BUTTON_PRESS_TEST_MODE 3

volatile bool button_active;
volatile uint8_t button_press_counter;
volatile uint8_t button_wait_timer;

// Turns on the Timer/Counter1 Output Compare A match interrupt
static void enable_PWM_button_interrupt() {
    TIMSK |= (1 << OCIE1A);
}

static void disable_PWM_button_interrupt() {
    TIMSK &= ~(1 << OCIE1A);
}

static void resolve_error(uint16_t *timer_seconds, bool *error_occurred) {
    *timer_seconds = 0;
    *error_occurred = false;
}

static void handle_button_press_sequence(uint16_t timer_top, uint16_t *timer_seconds, 
                                         bool *error_occurred, bool *test_mode) {
    disable_PWM_button_interrupt();

    switch (button_press_counter) {
        case BUTTON_PRESS_MANUAL_STOP:
            servo_off();
            if (*error_occurred) {
                init_display_error();
            } else {
                init_display_time(timer_top, *timer_seconds);
            }
            break;
        case BUTTON_PRESS_RESET_TIMER:
            resolve_error(timer_seconds, error_occurred);
            *test_mode = false;
            break;
        case BUTTON_PRESS_TEST_MODE:
            led_on();
            resolve_error(timer_seconds, error_occurred);
            *test_mode = true;
            break;
        default:
            break;
    }
    button_active = false;
    button_press_counter = 0;
}

void init_button() {
    BUTTON_DATA_DIRECTION_REG &= ~(1 << BUTTON_PIN);
}

void handle_button_press_interrupt() {
    enable_PWM_button_interrupt();
    if (button_wait_timer < BUTTON_STANDBY_TIMER_TOP - 5 || button_press_counter == 0) {
        button_press_counter++;
        button_wait_timer = BUTTON_STANDBY_TIMER_TOP;
    }
    button_active = true;
}

void handle_button_timer_interrupt(uint16_t timer_top, uint16_t *timer_seconds, 
                                   bool *error_occurred, bool *test_mode) {
    if (button_wait_timer > 0) {
        button_wait_timer--;
    } else if (button_press_counter > 0 && button_wait_timer == 0) {
        handle_button_press_sequence(timer_top, timer_seconds, error_occurred, test_mode);
    }
}
