#ifndef DISPLAY_H
#define DISPLAY_H

void init_7segment();
void disable_7segment();
void display_time(uint16_t period, uint16_t timer_seconds);

#endif