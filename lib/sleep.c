#include <stdbool.h>

volatile bool is_sleeping;

void sleep() {
    is_sleeping = true;
}

void wake_up() {
    is_sleeping = false;
}