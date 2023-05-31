#include <avr/io.h>
#include <stdbool.h>

#define LED_PORT PORTD
#define LED_PIN PD7

static void led_off() {
    LED_PORT &= ~(1 << LED_PIN);
}

void led_on() {
    LED_PORT |= (1 << LED_PIN);
}

void init_led() {
    DDRD |= (1 << PD7);
}

void handle_led_blinking(bool error_occurred, uint16_t timer_seconds) {
    if (!error_occurred && timer_seconds > 4) {
        return;
    }
    
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