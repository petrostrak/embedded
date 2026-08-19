#include <assert.h>
#include <inttypes.h>
#include <stdint.h>

#define FIELD_READ(reg, shift, width)                                          \
  (((uint32_t)(reg) >> (shift)) & ((1u << (width)) - 1u))

#define TIMER_CTRL_ADDR 0x40000000u

#define TIMER_MODE_MASK 0x00000003u      /* bits 0-1,   width 2  */
#define TIMER_ENABLE_MASK 0x00000004u    /* bit  2,     width 1  */
#define TIMER_IRQ_EN_MASK 0x00000008u    /* bit  3,     width 1  */
#define TIMER_PRESCALER_MASK 0x00000FF0u /* bits 4-11,  width 8  */
#define TIMER_CLKSRC_MASK 0x00003000u    /* bits 12-13, width 2  */
#define TIMER_RELOAD_MASK 0x0FFF0000u    /* bits 16-27, width 12 */

/* isolates the lowest set bit: 0x00000FF0 -> 0x00000010 */
#define LOWBIT(mask) ((uint32_t)(mask) & (~(uint32_t)(mask) + 1u))
/* multiply by the lowest bit == shift left; divide == shift right */
#define FIELD_GET(mask, reg)                                                   \
  (((uint32_t)(reg) & (uint32_t)(mask)) / LOWBIT(mask))
#define FIELD_PREP(mask, val)                                                  \
  (((uint32_t)(val) * LOWBIT(mask)) & (uint32_t)(mask))
#define FIELD_MODIFY(reg, mask, val)                                           \
  ((reg) = ((uint32_t)(reg) & ~(uint32_t)(mask)) | FIELD_PREP((mask), (val)))

int main(void)
{
  volatile uint32_t *ctrl = (volatile uint32_t *)TIMER_CTRL_ADDR;
  /*       ^         ^      ^
           |         |      +-- treat the number as a location
           |         +--------- 32 bits wide
           +------------------- may change outside my program
  */
  uint32_t reg = *ctrl;

  /* read a field */
  uint32_t prescaler = (reg >> 4) & 0xFF0u;

  /* or */
  uint32_t prescalerdefined = FIELD_READ(reg, 4, 8);
  assert(prescalerdefined == prescaler);

  /* write a field
   * remember, a field is part of a word, so you
   * must write the whole word. */
  uint32_t new_value = 0x55u;   /* the new value I want to write */
  uint32_t tmp = reg;           /* read the word */
  tmp &= ~TIMER_PRESCALER_MASK; /* clear the old field */
  tmp |= (new_value << 4) &
         TIMER_PRESCALER_MASK; /* set the new field with & mask at the end */
  reg = tmp;                   /* write */

  /* write all fields in one operation */
  uint32_t cfg = FIELD_PREP(TIMER_MODE_MASK, 2u) |
                 FIELD_PREP(TIMER_PRESCALER_MASK, 42u) |
                 FIELD_PREP(TIMER_CLKSRC_MASK, 1u) |
                 FIELD_PREP(TIMER_RELOAD_MASK, 1000u) | TIMER_IRQ_EN_MASK;

  *ctrl = cfg;

  return 0;
}
