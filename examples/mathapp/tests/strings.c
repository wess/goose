#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stringlib.h>

int main(void) {
    /* length */
    assert(str_len("hello") == 5);
    assert(str_len("") == 0);

    /* equality */
    assert(str_equal("abc", "abc") == 1);
    assert(str_equal("abc", "def") == 0);

    /* starts_with / ends_with */
    assert(str_starts_with("hello world", "hello") == 1);
    assert(str_starts_with("hello world", "world") == 0);
    assert(str_ends_with("hello world", "world") == 1);
    assert(str_ends_with("hello world", "hello") == 0);

    /* upper / lower */
    char buf[32];
    strcpy(buf, "Hello");
    str_upper(buf);
    assert(str_equal(buf, "HELLO"));

    str_lower(buf);
    assert(str_equal(buf, "hello"));

    /* trim */
    strcpy(buf, "  hello  ");
    str_trim(buf);
    assert(str_equal(buf, "hello"));

    printf("all string tests passed\n");
    return 0;
}
