#include <avr/io.h>
#include <stdbool.h>
#include <stdlib.h>
#include "display.h"
#include "sleep.h"

#define SECONDS_IN_AN_HOUR 3600
#define SECONDS_IN_A_MINUTE 60
#define DISPLAY_OUTPUT_PORT PORTA
#define DISPLAY_CONTROL_PORT PORTC
#define DISPLAY_LETTER_H 0x8B
#define DISPLAY_DOT 0x7F

uint8_t display_coms[3] = {PC7, PC6, PC5};
// digit codes for DPgfedcba LED layout
uint8_t display_digits[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
// letter codes for H E Y
uint8_t display_greeting_letters[3] = {0x89, 0x86, 0x91};
// letter codes for E R R
uint8_t display_error_letters[3] = {0x86, 0x88, 0x88};

volatile bool display_enabled;
volatile uint8_t display_cycle;
volatile uint8_t display_timer;

struct activeDisplay active_display = { .display_func = NULL, .cycles = 0, .seconds_in_cycle = 0 };
struct displayTimeLeft display_time_left = { .hours = 0, .minutes = 0, .seconds = 0 };
uint8_t* allocated_digits = NULL;
uint8_t number_of_allocated_digits = 0;

static void enable_display() {
    display_enabled = true;
    display_timer = 0;
    display_cycle = 1;
    // set a 64 divider for Timer0 and enable it
    TCCR0 |= (1 << CS01) | (1 << CS00);
    // enable Timer0 interrupt
    TIMSK |= (1 << OCIE0);
}

static void display_hours(uint8_t active_com) {
    switch (active_com) {
        case 0:
            DISPLAY_OUTPUT_PORT = display_digits[display_time_left.hours];
            break;
        case 1:
            DISPLAY_OUTPUT_PORT = DISPLAY_LETTER_H;
            break;
        case 2:
            DISPLAY_OUTPUT_PORT = 0xFF;
            break;
    }
}

static inline void reset_allocated_digits() {
    free(allocated_digits);
    number_of_allocated_digits = 0;
    allocated_digits = NULL;
}

static uint8_t* alloc_digits(uint8_t time_left) {
    uint8_t* digits = (uint8_t*) calloc(1, sizeof(uint8_t));
    while (time_left > 0 || number_of_allocated_digits == 0) {
        if (number_of_allocated_digits > 0) {
            digits = realloc(digits, (number_of_allocated_digits + 1) * sizeof(uint8_t));
        }
        uint8_t digit = time_left % 10;
        time_left /= 10;
        digits[number_of_allocated_digits] = digit;
        number_of_allocated_digits++;
    }
    return digits;
}

static void display_minutes_and_seconds(uint8_t time_left, uint8_t active_com) {
    static uint8_t last_display_cycle = 3;

    if (last_display_cycle != display_cycle) {
        if (allocated_digits != NULL) {
            reset_allocated_digits();
        }
        allocated_digits = alloc_digits(time_left);
        last_display_cycle = display_cycle;
    }
    
    switch (active_com) {
        case 0:
            if (number_of_allocated_digits > 1) {
                DISPLAY_OUTPUT_PORT = display_digits[allocated_digits[1]];
            } else {
                DISPLAY_OUTPUT_PORT = display_digits[0];
            }
            break;
        case 1:
            DISPLAY_OUTPUT_PORT = display_digits[allocated_digits[0]];
            break;
        case 2:
            DISPLAY_OUTPUT_PORT = 0xFF;
            break;
    }
}

static void display_time(uint8_t active_com) {
    switch (display_cycle) {
        case 1:
            display_hours(active_com);
            break;
        case 2:
            display_minutes_and_seconds(display_time_left.minutes, active_com);
            break;
        case 3:
            display_minutes_and_seconds(display_time_left.seconds, active_com);
            break;
        default:
            DISPLAY_OUTPUT_PORT = 0xFF;
    }
}

static void display_greeting(uint8_t active_com) {
    DISPLAY_OUTPUT_PORT = display_greeting_letters[active_com];
}

static void display_error(uint8_t active_com) {
    DISPLAY_OUTPUT_PORT = display_error_letters[active_com];
}

void handle_display_interrupt() {
    static uint8_t active_com = 0;
    // reset active com
    DISPLAY_CONTROL_PORT &= 0x1F;
    // activate current com
    DISPLAY_CONTROL_PORT |= (1 << display_coms[active_com]);

    (*active_display.display_func)(active_com);

    // increment active com so that 3 digits/letters can be displayed
    active_com++;
    if (active_com > 2) {
        active_com = 0;
    }
}

// 7 segment display LED output
void init_display() {
    // LED output pins
    DDRA = 0xFF;
    // driver pins
    DDRC |= (1 << PC5) | (1 << PC6) | (1 << PC7);
    // set CTC mode for Timer0
    TCCR0 |= (1 << WGM01);
    // set Output Compare Register
    OCR0 = 25;
    // turn all segments off
    DISPLAY_OUTPUT_PORT = 0xFF;
}

void init_display_time(uint16_t timer_top, uint16_t timer_seconds) {
    if (display_enabled) {
        return;
    }
    wake_up();

    uint16_t time_left = timer_top - timer_seconds;
    display_time_left.hours = time_left / SECONDS_IN_AN_HOUR;
    display_time_left.minutes = 
        (time_left - (display_time_left.hours * SECONDS_IN_AN_HOUR)) / SECONDS_IN_A_MINUTE;
    display_time_left.seconds = 
        time_left - ((display_time_left.hours * SECONDS_IN_AN_HOUR) + (display_time_left.minutes * SECONDS_IN_A_MINUTE));

    active_display.display_func = &display_time;
    active_display.cycles = 3;
    active_display.seconds_in_cycle = 2;

    enable_display();
}

void init_display_greeting() {
    if (display_enabled) {
        return;
    }
    wake_up();

    active_display.display_func = &display_greeting;
    active_display.cycles = 1;
    active_display.seconds_in_cycle = 6;

    enable_display();
}

void init_display_error() {
    if (display_enabled) {
        return;
    }
    wake_up();

    active_display.display_func = &display_error;
    active_display.cycles = 1;
    active_display.seconds_in_cycle = 6;

    enable_display();
}

void disable_display() {
    TCCR0 &= !((1 << CS01) | (1 << CS00));
    TIMSK &= !(1 << OCIE0);
    DISPLAY_OUTPUT_PORT = 0xFF;
    display_enabled = false;
    if (allocated_digits != NULL){
        reset_allocated_digits();
    }
}