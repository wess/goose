#include <stdio.h>
#include <assert.h>
#include <mathlib.h>

int main(void) {
    /* addition */
    assert(math_add(2, 3) == 5);
    assert(math_add(-1, 1) == 0);
    assert(math_add(0, 0) == 0);

    /* subtraction */
    assert(math_sub(10, 4) == 6);
    assert(math_sub(0, 5) == -5);

    /* multiplication */
    assert(math_mul(6, 7) == 42);
    assert(math_mul(-3, 3) == -9);

    /* factorial */
    assert(math_factorial(0) == 1);
    assert(math_factorial(1) == 1);
    assert(math_factorial(5) == 120);

    /* clamp */
    assert(math_clamp(15, 0, 10) == 10);
    assert(math_clamp(-5, 0, 10) == 0);
    assert(math_clamp(5, 0, 10) == 5);

    printf("all math tests passed\n");
    return 0;
}
