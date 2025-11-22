#include <stdint.h>
#include <stdio.h>

#define TIE_5_TYPES(T0, T1, T2, T3, T4)                                        \
  (((int64_t)T0) + ((int64_t)(T1) << 8) + ((int64_t)(T2) << 16) +              \
   ((int64_t)(T3) << 24) + ((int64_t)(T4) << 32))

#define TIE_5_TYPES_LITERAL(T0, T1, T2, T3, T4) TIE_5_TYPES(T0, T1, T2, T3, T4)

typedef enum type_t {
  a = TIE_5_TYPES_LITERAL(0x33, 0xF4, 0x33, 0xF4, 0xF5),
} type_t;

int main() {
  printf("%lx\n", a);
  return 0;
}
