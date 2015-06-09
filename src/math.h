/* Required math functions
 * isqrt_impl and isqrt provided by Siu Ching Pong on stackoverflow
 */
#include <pebble.h>

static uint32_t isqrt_impl(uint64_t const n, uint32_t const xk) {
    uint32_t const xk1 = (xk + n / xk) / 2;
    return (xk1 >= xk) ? xk : isqrt_impl(n, xk1);
}

/* An integer square root implementation. Not percise - but fast.
 * @n The number to root
 * @returns The square root
 */
uint32_t isqrt(uint64_t const n) {
    if (n == 0) return 0;
    if (n == 18446744073709551615ULL) return 4294967295U;
    return isqrt_impl(n, n);
}
