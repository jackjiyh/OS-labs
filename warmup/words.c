#include "common.h"
#include <stdio.h>

int
main(int argc, char *argv[])
{
	//TBD();
	if (argc > 1) {
	    int i;
	    for (i=1;i<argc;i++) {
	        printf(argv[i]);
	        printf("\n");
	    }
	}
	return 0;
}
