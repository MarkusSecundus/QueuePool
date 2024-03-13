#include "../basic_definitions.h"

#include "tests.h"

#define QUEUE_TEST_CLASS tests::QueuePoolTest

#include "../queue_pool.h"

using namespace queue_pooling;


namespace tests{
    void QueuePoolTest::test_allocation_only() {
        std::cout << "\n----------------------------------------\nALLOCATION...\n";
        constexpr int BUFFER_SIZE = 512;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<20>>;

        pool_t pool(buffer, BUFFER_SIZE);

        while (true) {
            auto allocated = pool.alloc_segment_from_free_list();
            if (!allocated.is_valid())
                break;
            std::cout << "allocated... is_free:"<<allocated.get_is_free_segment() <<", id: "<< (int)allocated.get_segment_id()<<", next: "<< (int)allocated.get_next_segment_id() <<", last: "<< (int)allocated.get_last_segment_id()<<", begin: "<<allocated.get_segment_begin()<<", length: "<<allocated.get_segment_length()<<",  data: "<< (void*)allocated.get_segment_data()<<"\n" ;
            allocated = pool.get_header(*pool.get_free_list_id_ptr());
            std::cout << "free_list... is_free:"<<allocated.get_is_free_segment() <<", id: "<< (int)allocated.get_segment_id()<<", next: "<< (int)allocated.get_next_segment_id() <<", last: "<< (int)allocated.get_last_segment_id()<<", begin: "<<allocated.get_segment_begin()<<", length: "<<allocated.get_segment_length()<<",  data: "<< (void*)allocated.get_segment_data()<<"\n" ;
        }


        std::cout << "handles... empty: "<< (int)pool_t::queue_handle_t::empty().get_segment_id() <<", invalid: "<< (int)pool_t::queue_handle_t::uninitialized().get_segment_id()<<"\n";

        auto q = pool.make_queue();
        byte_t byte;
        std::cout << pool.try_peek_byte(&q, &byte) <<"\n";
        auto header = pool.get_header(q.get_segment_id());
        std::cout << pool.try_grow_queue_by_1(&header) <<"\n";
    }



    void QueuePoolTest::test_enqueue1() {
        std::cout << "\n---------------------------------\n";

        constexpr int BUFFER_SIZE = 70;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<10>>;
        pool_t pool(buffer, BUFFER_SIZE);

        auto q = pool.make_queue();
        std::size_t i = 0;
        for (byte_t b = 0; pool.try_enqueue_byte(&q, b); ++b, ++i) {
            byte_t *bb;
            std::cout << (int)b << " -> " << (int)(pool.try_peak_front(pool.get_header(q.get_segment_id()), &bb), *bb) << " -> " << (int)buffer[i] << "\n";
        }

        std::cout << "\n********\n";
        for (std::size_t i=0; i < BUFFER_SIZE; ++i)
            std::cout << (int)buffer[i] << "\n";
    }
    void QueuePoolTest::test_enqueue2() {
        std::cout << "\n---------------------------------\n";

        constexpr int BUFFER_SIZE = 70, BLOCK_SIZE=15;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;
        pool_t pool(buffer, BUFFER_SIZE);

        auto q = pool.make_queue();
        auto q2 = pool.make_queue();
        std::size_t i = 0;
        bool choice=false;
        for (byte_t b = 0; (choice = (std::rand() & 1)), (pool.try_enqueue_byte(choice?&q:&q2, b + (choice?0:0)) || (choice = !choice , pool.try_enqueue_byte(choice ? &q : &q2, b + (choice ? 0 : 0)))); ++b, ++i) {
            byte_t *bb;
            std::cout << (int)b << " into[" << choice << "] -> " << (int)(pool.try_peak_front(pool.get_header(q.get_segment_id()), &bb), *bb) << " -> " << (int)buffer[i] << "\n";
        }

        std::cout << "\n********\n";
        for (std::size_t i = 0; i < BUFFER_SIZE; ++i) {
            if (!(i % BLOCK_SIZE)) std::cout << "_\n";
            std::cout << (int)buffer[i] << "\n";
        }
    }



    void QueuePoolTest::test_header_correctness(){
        std::cout << "\n----------------------------------------\nHEADER CORRECTNESS...\n";

        byte_t buffer[40];
        using header_t = standard_memory_policy<20>::segment_header_view_t;
        header_t h(buffer, 0);


#define TEST(setter, getter) \
        std::cout << "Testing " #setter "\n";\
        h.setter(0);\
        for (;; ) {\
            auto current = h.getter();\
            h.setter(current + 1);\
            auto next = h.getter();\
            if (next != current + 1) {\
                if (next == 0) { std::cout << "limit is: " << (std::size_t)current << "\n"; break; }\
                else std::cout << "!skip " << (std::size_t)current << " -> " << (std::size_t)next << "\n";\
            }\
        }

        TEST(set_segment_begin, get_segment_begin);
        TEST(set_segment_length, get_segment_length);
        TEST(set_last_segment_id, get_last_segment_id);
        TEST(set_next_segment_id, get_next_segment_id);
#undef TEST
    }
}

