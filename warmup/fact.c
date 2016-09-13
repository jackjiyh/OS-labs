#include "common.h"
#include <stdio.h>
#include <unistd.h>
int fact(int base);
int isFloat(float num);

int
main(int argc, char *argv[])
{
	//TBD();
        if (argc == 2) {
            float num = atof(argv[1]);
            if (num <= 0 || isFloat(num) == 1) {
                printf("Huh?\n");
                return 0;
            } else if (num > 12) {
                printf("Overflow\n");
                return 0;
            }
            int res = fact(num);
            printf("%d", res);
            printf("\n");
        } else {
            printf("Huh?\n");
        }

	return 0;
}

int fact(int base) {
    if (base == 1) return base;
    else {
        return base*fact(base-1);
    }
}

int isFloat(float num) {
    int temp = num;
    if ( (num-temp) != 0 ) return 1;
    else return 0;
}
