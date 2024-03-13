

#include "tests/tests.h"
using namespace tests;


#include "queue_pool.h"
using namespace queue_pooling;
#include "linked_list.h"
using namespace linked_lists;
#include "memory_policy.h"
using namespace memory_policies;

int main(){

    //tests::ll_test();
    //tests::QueuePoolTest{}.test_header_correctness();
    tests::QueuePoolTest{}.test_allocation_only();
    tests::QueuePoolTest{}.test_enqueue2();


    return 0;
}