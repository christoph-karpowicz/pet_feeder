#ifndef LED_H
#define LED_H

void init_led();
void led_on();
void led_off();
void handle_led_blinking(bool error_occurred, uint16_t timer_seconds);

#endif