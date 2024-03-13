#include "../basic_definitions.h"

#include "tests.h"

#define QUEUE_TEST_CLASS tests::QueuePoolTest

#include "../queue_pool.h"

using namespace queue_pooling;


#include<array>
#include<deque>


namespace tests{

    static void printout_buffer(std::ostream& wrt, byte_t* buffer, buffersize_t buffer_size, buffersize_t block_size) {
        wrt << "\n********\n";
        for (std::size_t i = 0; i < buffer_size; ++i) {
            if (!(i % block_size)) std::cout << "_\n";
            wrt << i << "|    " << (int)buffer[i] << "\n";
        }
    }

    struct QueuePoolTest::Helper {
        template<memory_policy TMemoryPolicy>
        void printout_queue(std::ostream& wrt, queue_pool_t<TMemoryPolicy>& pool, typename queue_pool_t<TMemoryPolicy>::header_view_t h) { printout_queue(wrt, pool, h.is_valid()? typename queue_pool_t<TMemoryPolicy>::queue_handle_t(h.get_segment_id()) : queue_pool_t<TMemoryPolicy>::queue_handle_t::empty()); }
        template<memory_policy TMemoryPolicy>
        void printout_queue(std::ostream& wrt, queue_pool_t<TMemoryPolicy>& pool, buffersize_t handle) { printout_queue(wrt, pool, queue_pool_t<TMemoryPolicy>::queue_handle_t(handle)); }
        template<memory_policy TMemoryPolicy>
        void printout_queue(std::ostream& wrt, queue_pool_t<TMemoryPolicy>& pool, typename queue_pool_t<TMemoryPolicy>::queue_handle_t handle) {
            if (!handle.is_valid()) {
                wrt << "<empty>\n"; return;
            }
            pool.ll().for_each(pool.get_header(handle.get_segment_id()), [&](auto header) {
                wrt << (int) header.get_segment_id() << " -> ";
                });
            wrt << "\n";
        }

        struct Helper2;
    };



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
            allocated = pool.get_free_list();
            if (allocated.is_valid())std::cout << "free_list... is_free:" << allocated.get_is_free_segment() << ", id: " << (int)allocated.get_segment_id() << ", next: " << (int)allocated.get_next_segment_id() << ", last: " << (int)allocated.get_last_segment_id() << ", begin: " << allocated.get_segment_begin() << ", length: " << allocated.get_segment_length() << ",  data: " << (void*)allocated.get_segment_data() << "\n";
            else std::cout << "free_list... <empty>\n";
        }


        std::cout << "handles... empty: "<< (int)pool_t::queue_handle_t::empty().get_segment_id() <<", invalid: "<< (int)pool_t::queue_handle_t::uninitialized().get_segment_id()<<"\n";

        auto q = pool.make_queue();
        byte_t byte;
        std::cout << pool.try_peek_byte(&q, &byte) <<"\n";
        auto header = pool.get_header(q.get_segment_id());
        std::cout << pool.try_grow_queue_by_1(&header) <<"\n";
    }



    void QueuePoolTest::test_enqueue1() {
        std::cout << "\n---------------------------------\n TEST_ENQUEUE1\n";

        constexpr int BUFFER_SIZE = 70, BLOCK_SIZE = 10;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;
        pool_t pool(buffer, BUFFER_SIZE);

        auto q = pool.make_queue();
        std::size_t i = 0;
        for (byte_t b = 0; pool.try_enqueue_byte(&q, b); ++b, ++i) {
            byte_t *bb;
            std::cout << (int)b << " -> " << (int)(pool.try_peak_front(pool.get_header(q.get_segment_id()), &bb), *bb) << " -> " << (int)buffer[i] << "\n";
        }

        printout_buffer(std::cout, buffer, BUFFER_SIZE, BLOCK_SIZE);
    }
    void QueuePoolTest::test_enqueue2() {
        std::cout << "\n---------------------------------\n TEST_ENQUEUE2\n";

        constexpr int BUFFER_SIZE = 70, BLOCK_SIZE=5;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;
        pool_t pool(buffer, BUFFER_SIZE);

        auto q = pool.make_queue();
        auto q2 = pool.make_queue();
        std::size_t i = 0;
        bool choice=false;
        for (byte_t b = 0; (choice = (std::rand() & 1)), (pool.try_enqueue_byte(choice?&q:&q2, b + (choice?0:0)) || (choice = !choice , pool.try_enqueue_byte(choice ? &q : &q2, b + (choice ? 0 : 0)))); ++b, ++i) {
            byte_t *bb;
            std::cout << (int)b << " into[" << choice << "] -> " << (int)(pool.try_peak_front(pool.get_header((choice ? &q : &q2)->get_segment_id()), &bb), *bb) << " -> " << (int)buffer[i] << "\n";
        }

        printout_buffer(std::cout, buffer, BUFFER_SIZE, BLOCK_SIZE);
    }

    void QueuePoolTest::test_enqueue2with_destroy() {
        std::cout << "\n---------------------------------\n TEST_ENQUEUE2_WITH_DESTROY\n";

        constexpr int BUFFER_SIZE = 70, BLOCK_SIZE = 10, ITERATIONS=800;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;
        pool_t pool(buffer, BUFFER_SIZE);
        
        auto q = pool.make_queue();
        auto q2 = pool.make_queue();

        for (std::size_t b = 0; b < ITERATIONS; ++b) {
            bool choice = std::rand() & 1;
            if (!pool.try_enqueue_byte(choice ? &q2 : &q, (byte_t)b)) {
                std::cout << b << "... !full - destroying queue " << choice <<"\n";
                Helper{}.printout_queue(std::cout << "q   : ", pool, q);
                Helper{}.printout_queue(std::cout << "q2  : ", pool, q2);
                Helper{}.printout_queue(std::cout << "free: ", pool, pool.get_free_list());
                pool.destroy_queue(choice ? &q : &q2);
                std::cout << "after...\n";
                Helper{}.printout_queue(std::cout << "q   : ", pool, q);
                Helper{}.printout_queue(std::cout << "q2  : ", pool, q2);
                Helper{}.printout_queue(std::cout << "free: ", pool, pool.get_free_list());
                continue;
            }
            byte_t* bb=0;
            std::cout << b << "... " << (int)(byte_t)b << " into[" << choice << "] -> |" << (int)(pool.try_peak_front(pool.get_header(q.get_segment_id()), &bb), bb?*bb:0);
            std::cout << " ; " << (int)(pool.try_peak_front(pool.get_header(q2.get_segment_id()), &bb), bb ? *bb : 0) << "|\n";
        }
        printout_buffer(std::cout, buffer, BUFFER_SIZE, BLOCK_SIZE);
    }


    void QueuePoolTest::test_enqueue_dequeue_only_full() {
        std::cout << "\n---------------------------------\n  TEST_ENQUEUE_DEQUEUE_ONLY_FULL\n";

        constexpr int BUFFER_SIZE = 70, BLOCK_SIZE = 10, ITERATIONS=5;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;
        pool_t pool(buffer, BUFFER_SIZE);

        for (int it = 0; it < ITERATIONS; ++it) {
            auto q = pool.make_queue();
            for (int b = 0; pool.try_enqueue_byte(&q, (byte_t)b); ++b) {
                std::cout << (int)b << ")  " << (int)(char)b << "\n";
            }
            Helper{}.printout_queue(std::cout << "   q: ", pool, q);
            Helper{}.printout_queue(std::cout << "free: ", pool, pool.get_free_list());
            std::cout << "clearing.........\n";
            pool.destroy_queue(&q);
        }


        //printout_buffer(std::cout, buffer, BUFFER_SIZE, BLOCK_SIZE);
    }
    void QueuePoolTest::test_enqueue_dequeue1() {
        std::cout << "\n---------------------------------\n  TEST_ENQUEUE_DEQUEUE_1\n";

        constexpr int BUFFER_SIZE = 70, BLOCK_SIZE = 10;
        byte_t buffer[BUFFER_SIZE];

        using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;
        pool_t pool(buffer, BUFFER_SIZE);

        auto q = pool.make_queue();
        std::size_t i = 0;
        for (byte_t b = 0; pool.try_enqueue_byte(&q, b); ++b, ++i) {
            byte_t* bb;
            std::cout << (int)b << " -> " << (int)(pool.try_peak_front(pool.get_header(q.get_segment_id()), &bb), *bb) << " -> " << (int)buffer[i] << "\n";
        }
        //printout_buffer(std::cout, buffer, BUFFER_SIZE, BLOCK_SIZE);
        i = 0;
        for (byte_t b = 0; pool.try_dequeue_byte(&q, &b); ++i) {
            byte_t* bb;
            std::cout << i << "... "<< (int)b << "\n";
        }


        //printout_buffer(std::cout, buffer, BUFFER_SIZE, BLOCK_SIZE);
    }

    struct QueuePoolTest::Helper::Helper2 {

        template<std::size_t BUFFER_SIZE, std::size_t BLOCK_SIZE, std::size_t QUEUES_COUNT, std::size_t OPERATIONS_COUNT, std::size_t MAX_ELEMENTS_IN_QUEUE, int DEQUEUE_CHANCE>
        void test_queue_randomized_impl() {
            std::cout << "\n***********************\nRANDOMIZED_TEST(buffer_size=" << BUFFER_SIZE<<", block_size="<<BLOCK_SIZE<<", queues_count="<<QUEUES_COUNT << ", ops_count="<< OPERATIONS_COUNT<<", max_elems_in_queue="<<MAX_ELEMENTS_IN_QUEUE << ", dequeue=1/"<<DEQUEUE_CHANCE << ")\n";

            using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;

            byte_t buffer[BUFFER_SIZE];
            pool_t pool(buffer, BUFFER_SIZE);
            
            std::array<typename pool_t::queue_handle_t, QUEUES_COUNT> queues{};
            std::array<typename std::deque<byte_t>, QUEUES_COUNT> std_queues{};
            
            for (std::size_t t = 0; t < QUEUES_COUNT; ++t) 
                queues[t] = pool.make_queue();


            for (std::size_t op_ = 0; op_ < OPERATIONS_COUNT; ++op_) {
                int queue_index = std::rand() % QUEUES_COUNT;

                if (std::rand() % DEQUEUE_CHANCE) { //enqueue
                    if (std_queues[queue_index].size() >= MAX_ELEMENTS_IN_QUEUE) {
                        //std::cout << op_ << ")... too much stuff in queue " << queue_index << "\n";
                        continue;
                    }
                    byte_t to_enqueue = (byte_t)std::rand();
                    if (!pool.try_enqueue_byte(&(queues[queue_index]), to_enqueue)) {
                        std::cout << op_ << ")... " << "cannot enqueue value " << (int)to_enqueue << " to queue n." << queue_index << "\n";
                        continue;
                    }
                    std_queues[queue_index].push_back(to_enqueue);

                    //std::cout << op_ << ")... enqueue value "<<(int)to_enqueue<<" to n."<< queue_index << ", q.id = " << (int)queues[queue_index].get_segment_id() << "\n";
                }
                else { //dequeue
                    byte_t my_byte = 0, std_byte = 0;
                    bool std_empty = std_queues[queue_index].size() <= 0;
                    if (!std_empty) {
                        std_byte = std_queues[queue_index].front();
                        std_queues[queue_index].pop_front();
                    }
                    bool my_empty = !pool.try_dequeue_byte(&(queues[queue_index]), &my_byte);

                    if (std_empty != my_empty)
                        std::cout << op_ << ")... " << "emptiness difference: std(empty=" << std_empty << "), my(empty=" << my_empty << ")\n";
                    else if (std_byte != my_byte)
                        std::cout << op_ << ")... " << "value difference: std(" << (int)std_byte << "), my(" << (int)my_byte << ")\n";
                    //else std::cout << op_ << ")... alright\n";
                }
            }

            std::cout << "\n*TEST FINISHED!\n";
        }
        template<std::size_t BUFFER_SIZE, std::size_t BLOCK_SIZE, std::size_t QUEUES_COUNT, std::size_t OPERATIONS_COUNT, std::size_t MAX_ELEMENTS_IN_QUEUE, int DEQUEUE_CHANCE, int DESTROY_CHANCE>
        void test_queue_randomized_with_destroy_impl() {
            std::cout << "\n***********************\nRANDOMIZED_TEST(buffer_size=" << BUFFER_SIZE<<", block_size="<<BLOCK_SIZE<<", queues_count="<<QUEUES_COUNT << ", ops_count="<< OPERATIONS_COUNT<<", max_elems_in_queue="<<MAX_ELEMENTS_IN_QUEUE << ", dequeue=1/"<<DEQUEUE_CHANCE << ", destroy=1/"<<DESTROY_CHANCE<< ")\n";

            using pool_t = queue_pool_t<standard_memory_policy<BLOCK_SIZE>>;

            byte_t buffer[BUFFER_SIZE];
            pool_t pool(buffer, BUFFER_SIZE);
            
            std::array<typename pool_t::queue_handle_t, QUEUES_COUNT> queues{};
            std::array<typename std::deque<byte_t>, QUEUES_COUNT> std_queues{};
            
            for (std::size_t t = 0; t < QUEUES_COUNT; ++t) 
                queues[t] = pool.make_queue();


            for (std::size_t op_ = 0; op_ < OPERATIONS_COUNT; ++op_) {
                int queue_index = std::rand() % QUEUES_COUNT;

                int rand = std::rand();
                if (!(rand % DESTROY_CHANCE)) {
                    std_queues[queue_index].clear();
                    pool.destroy_queue(&queues[queue_index]);
                    //std::cout << op_ << ")... " << "destroying queue: " << queue_index << "\n";
                }
                else if (rand % DEQUEUE_CHANCE) { //enqueue
                    if (std_queues[queue_index].size() >= MAX_ELEMENTS_IN_QUEUE) {
                        //std::cout << op_ << ")... too much stuff in queue " << queue_index << "\n";
                        continue;
                    }
                    byte_t to_enqueue = (byte_t)std::rand();
                    if (!pool.try_enqueue_byte(&(queues[queue_index]), to_enqueue)) {
                        std::cout << op_ << ")... " << "cannot enqueue value " << (int)to_enqueue << " to queue n." << queue_index << "\n";
                        continue;
                    }
                    std_queues[queue_index].push_back(to_enqueue);

                    //std::cout << op_ << ")... enqueue value "<<(int)to_enqueue<<" to n."<< queue_index << ", q.id = " << (int)queues[queue_index].get_segment_id() << "\n";
                }
                else { //dequeue
                    byte_t my_byte = 0, std_byte = 0;
                    bool std_empty = std_queues[queue_index].size() <= 0;
                    if (!std_empty) {
                        std_byte = std_queues[queue_index].front();
                        std_queues[queue_index].pop_front();
                    }
                    bool my_empty = !pool.try_dequeue_byte(&(queues[queue_index]), &my_byte);

                    if (std_empty != my_empty)
                        std::cout << op_ << ")... " << "emptiness difference: std(empty=" << std_empty << "), my(empty=" << my_empty << ")\n";
                    else if (std_byte != my_byte)
                        std::cout << op_ << ")... " << "value difference: std(" << (int)std_byte << "), my(" << (int)my_byte << ")\n";
                    //else std::cout << op_ << ")... alright\n";
                }
            }

            std::cout << "\n*TEST FINISHED!\n";
        }

    };


    void QueuePoolTest::test_queue_randomized() {
        std::cout << "\n---------------------------------\nRANDOMIZED_TESTS...\n";

        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<2048, 24, 15, 50000, 120, 2>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<1920, 24, 15, 50000, 120, 2>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<1920, 24, 15, 50000, 80, 5>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<1920, 24, 7, 50000, 160, 5>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<1920, 64, 2, 50000, 800, 5>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<1920, 15, 64, 50000, 11, 5>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<1920, 15, 64, 50000, 16, 2>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_impl<4096, 44, 30, 50000, 80, 5>();
    }
    void QueuePoolTest::test_queue_randomized_with_destroy() {
        std::cout << "\n---------------------------------\nRANDOMIZED_TESTS...\n";

        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<2048, 24, 15, 50000, 120, 2, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 15, 50000, 120, 2, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 15, 50000, 80, 5, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 7, 50000, 160, 5, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 64, 2, 50000, 800, 5, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 15, 64, 50000, 11, 5, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 15, 64, 50000, 16, 2, 10>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<4096, 44, 30, 50000, 80, 5, 10>();


        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<2048, 24, 15, 50000, 120, 2, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 15, 50000, 120, 2, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 15, 50000, 80, 5, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 7, 50000, 160, 5, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 64, 2, 50000, 800, 5, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 15, 64, 50000, 11, 5, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 15, 64, 50000, 16, 2, 200>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<4096, 44, 30, 50000, 80, 5, 200>();

        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<2048, 24, 15, 50000, 120, 2, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 15, 50000, 120, 2, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 15, 50000, 80, 5, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 24, 7, 50000, 160, 5, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 64, 2, 50000, 800, 5, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 15, 64, 50000, 11, 5, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<1920, 15, 64, 50000, 16, 2, 2000>();
        QueuePoolTest::Helper::Helper2{}.test_queue_randomized_with_destroy_impl<4096, 44, 30, 50000, 80, 5, 2000>();
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

