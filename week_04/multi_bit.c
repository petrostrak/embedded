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

int main(void)
{
  volatile uint32_t *ctrl = (volatile uint32_t *)TIMER_CTRL_ADDR;
  uint32_t reg = *ctrl;

  /* read a field */
  uint32_t prescaler = (reg >> 4) & 0xFF0u;

  /* or */
  uint32_t prescalerdefined = FIELD_READ(reg, 4, 8);
  assert(prescalerdefined == prescaler);

  return 0;
}
