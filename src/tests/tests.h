#ifndef TESTS__guard___fd4s98v4s65d46h54h6hgf4j6b5fd4
#define TESTS__guard___fd4s98v4s65d46h54h6hgf4j6b5fd4


#include "../basic_definitions.h"





namespace tests{
    struct QueuePoolTest {
        void test_allocation_only();

        void test_enqueue1();
        void test_enqueue2();
        void test_enqueue2with_destroy();

        void test_enqueue_dequeue_only_full();
        void test_enqueue_dequeue1();

        void test_queue_randomized();
        void test_queue_randomized_with_destroy();

        void test_header_correctness();
    private:
        //to be able to write helper functions that use friendship with queue_pool_t inside tests_*.cpp files
        struct Helper;
    };

    void ll_test();
    void ll_node_swap_test();
    void ll_randomized_test();

}









#endif