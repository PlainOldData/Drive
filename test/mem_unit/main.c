
#ifdef BUILD_UTEST_INCLUDED
/*
 * Execution of Unit tests.
 */


#include <utest.h>


UTEST_MAIN();


#else
/*
 * Utest not present, maybe it hasn't been cloned into thirdparty.
 */


#include <stdio.h>
#include <stdlib.h>


int
main() {
        fprintf(stderr, "ERROR: Utest is not present so not tests built\n");
        return EXIT_FAILURE;
}


#endif
