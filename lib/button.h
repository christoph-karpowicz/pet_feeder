#ifndef BUTTON_H
#define BUTTON_H

void init_button();
void handle_button_press_interrupt();
void handle_button_timer_interrupt(uint16_t timer_top, uint16_t *timer_seconds, bool *error_occurred, bool *test_mode);

#endif