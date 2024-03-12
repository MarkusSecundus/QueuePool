

#include "tests/tests.h"
using namespace tests;

int main(){

    tests::ll_test();
    tests::QueuePoolTest{}.test_allocation_only();
    tests::QueuePoolTest{}.tst();

    return 0;
}