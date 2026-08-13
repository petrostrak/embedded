#ifndef FLAG_H
#define FLAG_H

/* Deliberately NOT volatile, NOT _Atomic. That is the point of the exercise. */
extern volatile int ready;

void set_ready(void);

#endif /* FLAG_H */
