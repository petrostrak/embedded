#ifndef BIT_OPS_H
#define BIT_OPS_H

#include <stdint.h>

#ifndef BIT
#define BIT(n) ((uint32_t)1u << (n))
#endif

#define SET_BITS(reg, m) ((reg) |= (m))
#define CLR_BITS(reg, m) ((reg) &= ~(m))
#define TGL_BITS(reg, m) ((reg) ^= (m))
#define TST_BITS(reg, m) (((reg) & (m)) != 0u)

/* Isolates the lowest set bit of the mask = 2^(shift of the field). */
#define LOW_BIT(m) ((m) & (0u - (m)))

/* Build a field value in place, without touching a register. */
#define FIELD_PREP(m, v) ((((uint32_t)(v)) * LOW_BIT(m)) & (m))
#define FIELD_GET(reg, m) (((reg) & (m)) / LOW_BIT(m))
#define FIELD_SET(reg, m, v) ((reg) = ((reg) & ~(m)) | FIELD_PREP((m), (v)))

#endif /* BIT_OPS_H */
