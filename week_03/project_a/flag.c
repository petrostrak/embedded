#include "flag.h"
#include <stdbool.h>

int volatile ready = 0;

void set_ready(void) { ready = 1; }
