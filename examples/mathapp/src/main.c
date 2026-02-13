#include <stdio.h>
#include <mathlib.h>
#include <stringlib.h>

int main(void) {
    /* math examples */
    printf("=== Math ===\n");
    printf("2 + 3 = %d\n", math_add(2, 3));
    printf("10 - 4 = %d\n", math_sub(10, 4));
    printf("6 * 7 = %d\n", math_mul(6, 7));
    printf("10 / 3 = %.2f\n", math_div(10, 3));
    printf("5! = %d\n", math_factorial(5));
    printf("clamp(15, 0, 10) = %d\n", math_clamp(15, 0, 10));

    /* string examples */
    printf("\n=== Strings ===\n");
    char buf[64];
    str_copy(buf, "  Hello, Goose!  ");
    str_trim(buf);
    printf("trimmed: '%s'\n", buf);

    str_upper(buf);
    printf("upper: '%s'\n", buf);

    str_lower(buf);
    printf("lower: '%s'\n", buf);

    printf("starts with 'hello': %s\n",
           str_starts_with(buf, "hello") ? "yes" : "no");
    printf("ends with 'goose!': %s\n",
           str_ends_with(buf, "goose!") ? "yes" : "no");

    return 0;
}
