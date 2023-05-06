#ifndef DISPLAY_H
#define DISPLAY_H

struct activeDisplay {
    void (*display_func)(uint8_t);
    uint8_t cycles;
    uint8_t seconds_in_cycle;
};

struct displayTimeLeft {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

void handle_display_interrupt();
void init_display();
void init_display_time(uint16_t timer_top, uint16_t timer_seconds);
void init_display_greeting();
void init_display_error();
void disable_display();

#endif