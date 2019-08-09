#ifdef BUILD_UTEST_INCLUDED


#define STACK_IMPL
#include <drive/mem/stack.h>

#include <utest.h>
#include <stdlib.h>


/* --------------------------------------------------------------- fixture -- */


struct mem_stack_fixture {
        int i;
};


UTEST_F_SETUP(mem_stack_fixture) {
        ASSERT_EQ(1, 1);

//        utest_fixture->ctx = ctx;
}


UTEST_F_TEARDOWN(mem_stack_fixture) {
        ASSERT_EQ(1, 1);
}


/* ------------------------------------------------------------ test cases -- */


UTEST_F(mem_stack_fixture, push_pop) {
        ASSERT_EQ(1, 1);
}


UTEST_F(mem_stack_fixture, push_pop_to) {
        ASSERT_EQ(1, 1);
}


UTEST_F(mem_stack_fixture, push_clear_push) {
        ASSERT_EQ(1, 1);
}


#endif
