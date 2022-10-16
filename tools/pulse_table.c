#include <stdio.h>

int main(void) {
    printf("double pulse_lookup_table[31] = { ");
    for (int i=0; i<31; ++i) {
        double num = 95.52 / (8128.0 / i + 100);
        printf("%.17g", num);
        if (i == 30)
            printf(" };");
        else
            printf(", ");
    }
    printf("\n");
    return 0;
}
