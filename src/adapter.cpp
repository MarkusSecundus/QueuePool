#include<array>

#include "queue_pool.h"

using namespace queue_pooling;

/// <summary>
/// Size in bytes of the buffer used by the queue pool.
/// </summary>
constexpr std::size_t GLOBAL_BUFFER_SIZE = 2048;
/// <summary>
/// Size of individual blocks of memory that are allocated by the queues. Each block has 4 bytes overhead for a header.
/// </summary>
constexpr buffersize_t GLOBAL_BLOCK_SIZE = 24;
/// <summary>
/// Defines whether a queue segment can expand into a free block to its right (thus saving overhead of a header),
///  or a new block must be allocated and connected as a linked list. 
/// Having it on seems to not always be a better option, but it's a nice thing to tweak along with block size.
/// </summary>
constexpr bool GLOBAL_USE_LARGE_SEGMENTS = false;
constexpr segment_id_t GLOBAL_MAX_QUEUES = 64;

extern void out_of_memory();
extern void on_illegal_operation();

void out_of_memory() { printf("out-of-memory!\n"); }
void on_illegal_operation() { printf("ILLEGAL!\n"); }


/// <summary>
/// I am convinced that the interface requested by the assignment is quite dumb, clunky to use and its usage imposes unnecessary overhead.
/// Thus, my implementation of queue_pool_t declares different interface which I think makes a bit more sense.
/// This class serves as an adapter that makes it fit the interface requested by the assignment.
/// Still doesn't assume global state, because global state is evil.
/// </summary>
/// <typeparam name="MAX_QUEUES">How many queues can max exist at any given point.</typeparam>
/// <typeparam name="TMemoryPolicy">Specifies encoding of segment headers - what memory overhead they have any how big buffer is adressable.</typeparam>
template<segment_id_t MAX_QUEUES = GLOBAL_MAX_QUEUES, memory_policy TMemoryPolicy = standard_memory_policy>
class queue_pool_adapter_t {
    using pool_t = queue_pool_t<TMemoryPolicy>;
public:
    using Q = pool_t::queue_handle_t;
    template<typename ...Args>
    queue_pool_adapter_t(byte_t* buffer_, buffersize_t buffer_size_, Args ...args)
        : buffer(reinterpret_cast<buffer_view_t*>(buffer_))
        , buffer_size(buffer_size_ - sizeof(buffer_view_t::header))
        , pool(buffer->data, buffer_size, args...)
        {}

    /// <summary>
    /// Initializes the pool. Should be called before it's used for the first time.
    /// </summary>
    void init() {
        for (buffersize_t t = 0; t < MAX_QUEUES; ++t)
            buffer->header.handles[t] = pool_t::queue_handle_t::uninitialized();
        pool.init();
    }

    Q* create_queue() {
        //MAX_QUEUES=64 -> this fits in a single cacheline if the compiler alligns the buffer alright -> probably doesn't make sense to optimize this further unless profiling says otherwise xD
        //still it's pretty dumb that I even have to waste space for these handles
        for (buffersize_t t = 0; t < MAX_QUEUES; ++t) {
            if (buffer->header.handles[t].is_uninitialized()) {
                return &(buffer->header.handles[t] = pool.make_queue());
            }
        }
        on_illegal_operation();
        return nullptr;
    }

    void destroy_queue(Q* q) {
        if (!is_valid_handle(q)){
            on_illegal_operation();
            return;
        }
        pool.destroy_queue(q);
        *q = Q::uninitialized();
    }

    void enqueue_byte(Q* q, byte_t b) {
        if (!is_valid_handle(q))
        {
            on_illegal_operation();
            return;
        }
        if (!pool.try_enqueue_byte(q, b)) {
            out_of_memory();
            return;
        }
    }
    byte_t dequeue_byte(Q* q) {
        if (!is_valid_handle(q)) {
            on_illegal_operation();
            return -1;
        }
        byte_t ret;
        if (!pool.try_dequeue_byte(q, &ret)) {
            on_illegal_operation();
            return -1;
        }
        return ret;
    }

private:
    bool is_valid_handle(Q* q) {
        return q && !q->is_uninitialized();
    }

    struct buffer_view_t {
        struct header_t {
            std::array<typename pool_t::queue_handle_t, MAX_QUEUES> handles;
        } header;
        byte_t data[];
    } *buffer;
    buffersize_t buffer_size;
    pool_t pool;
};



/// Now finally the actual interface that was requested by the assignment. I hope nobody ever uses this in production xD
byte_t global_buffer[GLOBAL_BUFFER_SIZE];

using pool_t = queue_pool_adapter_t<>;
using Q = pool_t::Q;

static pool_t get_global_pool_() {
    return pool_t(global_buffer, GLOBAL_BUFFER_SIZE, GLOBAL_USE_LARGE_SEGMENTS, GLOBAL_BLOCK_SIZE);
}
Q* create_queue() { return get_global_pool_().create_queue(); }
void destroy_queue(Q* q) { get_global_pool_().destroy_queue(q); }
void enqueue_byte(Q* q, byte_t b) { get_global_pool_().enqueue_byte(q, b); }
byte_t dequeue_byte(Q* q) { return get_global_pool_().dequeue_byte(q); }

/// <summary>
/// As quoted from the assignment:
/// """Your code is not allowed to call malloc() or other heap management routines.
///  Instead, all *storage*(other than local variables in your functions) must be within a provided array..."""
/// Even though this variable takes 1 byte of space, which GCC stubbornly refuses to optimize away, 
/// it is never used for any *storage*, so initializing the pool this way hopefully should be ok.
/// 
/// Although anyway, it's pretty dumb that the required interface doesn't permit an `init()` function explicitly called by the user.
/// </summary>
struct pool_initialization_helper_t___ {
    pool_initialization_helper_t___() { get_global_pool_().init(); }
} pool_initialization_helper___;


void main2() {
    Q* q0 = create_queue();
    enqueue_byte(q0, 0);
    enqueue_byte(q0, 1);
    Q* q1 = create_queue();
    enqueue_byte(q1, 3);
    enqueue_byte(q0, 2);
    enqueue_byte(q1, 4);
    printf("%d", dequeue_byte(q0));
    printf("%d\n", dequeue_byte(q0));
    enqueue_byte(q0, 5);
    enqueue_byte(q1, 6);
    printf("%d", dequeue_byte(q0));
    printf("%d\n", dequeue_byte(q0));
    destroy_queue(q0);
    printf("%d", dequeue_byte(q1));
    printf("%d", dequeue_byte(q1));
    printf("%d\n", dequeue_byte(q1));
    destroy_queue(q1);
}