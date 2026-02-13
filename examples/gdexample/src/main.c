#include <stdio.h>
#include "gd.h"

int main(void) {
    gdImagePtr img = gdImageCreate(200, 200);
    if (!img) {
        fprintf(stderr, "failed to create image\n");
        return 1;
    }

    int white = gdImageColorAllocate(img, 255, 255, 255);
    int blue  = gdImageColorAllocate(img, 0, 0, 255);
    (void)white;

    gdImageLine(img, 0, 0, 199, 199, blue);
    gdImageRectangle(img, 10, 10, 190, 190, blue);

    printf("created 200x200 image with line and rectangle\n");
    printf("white=%d blue=%d\n", white, blue);

    gdImageDestroy(img);
    printf("image destroyed, done.\n");
    return 0;
}
