#ifndef DISPLAY_H
#define DISPLAY_H

void handle_display_interrupt();
void init_display();
void disable_display();
void display_time(uint16_t period, uint16_t timer_seconds);

#endif