#include <mathlib.h>

int math_add(int a, int b) {
    return a + b;
}

int math_sub(int a, int b) {
    return a - b;
}

int math_mul(int a, int b) {
    return a * b;
}

double math_div(int a, int b) {
    if (b == 0) return 0.0;
    return (double)a / (double)b;
}

int math_factorial(int n) {
    if (n <= 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; i++)
        result *= i;
    return result;
}

int math_clamp(int val, int lo, int hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}
