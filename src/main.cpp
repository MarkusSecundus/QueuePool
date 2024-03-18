#include <ctime>

#include "tests/tests.h"

extern void main2();

int main(){

    main2();


    std::srand(std::time(nullptr));

    //tests::ll_test();
    //tests::ll_node_swap_test();
    //tests::QueuePoolTest{}.test_header_correctness();
    //tests::QueuePoolTest{}.test_allocation_only();
    //tests::QueuePoolTest{}.test_enqueue1();
    //tests::QueuePoolTest{}.test_enqueue2();
    //tests::QueuePoolTest{}.test_enqueue_dequeue1();
    //tests::QueuePoolTest{}.test_enqueue2with_destroy();
    tests::QueuePoolTest{}.test_queue_randomized();
    tests::QueuePoolTest{}.test_queue_randomized_with_destroy();
    //tests::ll_randomized_test();


    return 0;
}