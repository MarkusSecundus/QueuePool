#ifndef QUEUE_POOL__guard___fds4g89dfv46ds51d6a4d9as4d6sagr
#define QUEUE_POOL__guard___fds4g89dfv46ds51d6a4d9as4d6sagr

#include<algorithm>

#include "basic_definitions.h"
#include "utils/linked_list.h"
#include "utils/math_utils.h"
#include "memory_policy.h"



namespace markussecundus::queue_pooling{
    using namespace markussecundus::utils;

/// <summary>
/// Builds a collection of FIFO queues on top of a provided bytearray. Provides functionality to create/destroy a queue 
/// and to enqueue/dequeue bytes into/from a specific queue.
/// 
/// Queues are implemented as linked lists of fixed-size blocks with user specified block size. 
/// Each block has an overhead of a header - by default 4 bytes, but different encoding can be provided as template parameter.
/// Manual tweaking of block size and header is advised to achieve the best memory efficiency in specific usecase.
/// 
/// 
/// Optional optimization: 
///     - when queue depletes space in its current block and there is a free block directly on its right, 
///       it grows into it, saving header overhead, instead of allocating the next block that's in line in free list.
///     - in practice doesn't seem to always perform better than not doing it - tweaking required
///     - switchable by the use_multiblock_segments constructor argument.
///  
/// Performance analysis...
///   Enqueue/dequeue and create_queue are guaranteed to finish in O(1) time. 
///   Destroy queue must iterate through all the segments to mark them as free, thus O(n) time, albeit with quite nice constants (dependent on block size). 
///   
///   For future consideration: destroy_queue could be made run in O(1) time 
///    - the linked list operation to add a queue to free_list is O(1) by itself, bottleneck is the marking of segments as free.
///       Getting rid of it might make sense, but it would prevent us from doing the multiblock_segment optimization 
///       (there would be no O(1) way of telling if a block directly to another block's right is a free_list block? (maybe could be done efficiently with union-find??)).
/// </summary>
/// <typeparam name="TMemoryPolicy">Object specifying details about how memory shall be handled (block size, header encoding etc.) by a queue pool.</typeparam>
template<memory_policies::memory_policy TMemoryPolicy= memory_policies::standard_memory_policy>
class queue_pool_t : private TMemoryPolicy{
public:
    
    using segment_id_t = TMemoryPolicy::segment_id_t;
    using packed_segment_id_t = TMemoryPolicy::packed_segment_id_t;

    struct queue_handle_t {

        queue_handle_t() : queue_handle_t(uninitialized().get_segment_id()) {}
        queue_handle_t(segment_id_t segment_id_) :segment_id(segment_id_) {}

        static constexpr queue_handle_t from_header(typename queue_pool_t::header_view_t h) {
            return h.is_valid() ? queue_handle_t(h.get_segment_id()) : queue_handle_t::error();
        }

        segment_id_t get_segment_id()const { return segment_id; }

        static constexpr queue_handle_t uninitialized() { return queue_handle_t(~0); }
        static constexpr queue_handle_t error() { return empty(); }
        static constexpr queue_handle_t empty() { return queue_handle_t((segment_id_t)(std::size_t)(~0) - 1); }

        bool is_uninitialized() { return segment_id == uninitialized().segment_id; }
        bool is_empty() { return segment_id == empty().segment_id; }
        bool is_valid() { return !(is_empty() || is_uninitialized()); }
        static constexpr segment_id_t SPECIAL_VALUES_COUNT = 2;
    private:
        packed_segment_id_t segment_id;
    };



    template<typename ...Args>
    queue_pool_t(byte_t* buffer_, buffersize_t buffer_size_, bool use_multiblock_segments_, Args ...args) 
        : TMemoryPolicy(args...)
        , buffer(reinterpret_cast<buffer_view_t*>(buffer_))
        , buffer_size(buffer_size_ - sizeof(buffer_view_t::header))
        , use_multiblock_segments(use_multiblock_segments_) 
        {}

    /// <summary>
    /// Initializes the pool. Should be called before it's used for the first time.
    /// </summary>
    void init(){
        buffer->header.free_list = init_free_list();
    }


    /// <summary>
    /// Creates a new queue.
    /// Runs in O(1) time.
    /// </summary>
    /// <returns>Handle to a new queue</returns>
    queue_handle_t make_queue() {
        return queue_handle_t::empty();
    }
    /// <summary>
    /// Tries to enqueue a byte into a queue. 
    /// Can fail e.g. because running out of memory.
    /// 
    /// Runs in O(1) time.
    /// </summary>
    /// <param name="handle_ptr">Pointer to the queue handle. Value pointed to might get updated in the process of this function.</param>
    /// <param name="to_enqueue">Byte to enqueue.</param>
    /// <returns>Whether the operation was successfull (didn't fail due to out-of-memory etc.)</returns>
    bool try_enqueue_byte(queue_handle_t* handle_ptr, byte_t to_enqueue) {
        auto head = get_header(handle_ptr->get_segment_id());
        byte_t* new_byte;
        if (try_grow_queue_by_1(&head) && try_peak_front(head, &new_byte)) {
            *new_byte = to_enqueue;
            *handle_ptr = queue_handle_t::from_header(head);
            return true;
        }
        return false;
    }
    /// <summary>
    /// Tries to dequeue a byte from a queue.
    /// Will fail if there is nothing left in the provided queue.
    /// 
    /// Runs in O(1) time.
    /// </summary>
    /// <param name="handle_ptr">Pointer to the queue handle. Value pointed to might get updated in the process of this function.</param>
    /// <param name="out_byte">Byte that was dequeued</param>
    /// <returns>Whether the operation was successfull (there was still something to dequeue)</returns>
    bool try_dequeue_byte(queue_handle_t* handle_ptr, byte_t* out_byte) {
        if (!handle_ptr->is_valid())
            return false;

        auto head = get_header(handle_ptr->get_segment_id());

        byte_t* back_ref;
        if (!try_peak_back(head, &back_ref)) return false;
        *out_byte = *back_ref;
        if (try_shrink_queue_by_1(&head)) {
            *handle_ptr = queue_handle_t::from_header(head);
            return true;
        }
        return false;
    }
    /// <summary>
    /// Destroys the queue and releases its resources to be used by other queues.
    /// Queue handle gets invalidated in the process.
    /// </summary>
    /// <param name="handle_ptr">Queue to be used. Gets reset by this function to `uninitialized`.</param>
    void destroy_queue(queue_handle_t* handle_ptr)
    {
        if (!handle_ptr->is_valid())return;
        release_queue_to_freelist(get_header(handle_ptr->get_segment_id()));
        *handle_ptr = queue_handle_t::uninitialized();
    }


private:
#pragma region BufferManipulationPrimitives

    struct buffer_view_t {
        struct header_t {
            packed_segment_id_t free_list;
        } header;
        byte_t data[];
    };
    using header_view_t = typename TMemoryPolicy::segment_header_view_t;

    buffer_view_t* buffer;
    buffersize_t buffer_size;
    bool use_multiblock_segments;

    constexpr buffersize_t get_block_size_bytes() { return TMemoryPolicy::get_block_size_bytes(); }
    buffersize_t get_header_size_bytes(){return TMemoryPolicy::get_header_size_bytes();}
    buffersize_t get_allocatable_buffer_size_bytes() { return get_total_blocks_count() * get_block_size_bytes(); }

    byte_t* get_segment_start(segment_id_t segment_index) { return &(buffer->data[segment_index * get_block_size_bytes()]); }
    segment_id_t get_total_blocks_count() { return std::min<segment_id_t>(
        TMemoryPolicy::get_addressable_blocks_count() - queue_handle_t::SPECIAL_VALUES_COUNT, 
        (segment_id_t)(buffer_size) / get_block_size_bytes()); 
    }
    header_view_t get_header(segment_id_t segment_index) {
        if (segment_index < 0 || segment_index >= get_total_blocks_count() || !queue_handle_t(segment_index).is_valid() ) return header_view_t::invalid();
        return TMemoryPolicy::make_header_view(get_segment_start(segment_index), segment_index);
    }

    buffersize_t get_blocks_count_of_segment(header_view_t h, buffersize_t additional_bytes) {
        if (!h.is_valid()) return 0;
        return math::divide_round_up(h.get_segment_begin() + h.get_segment_length() + get_header_size_bytes() + additional_bytes, get_block_size_bytes());
    }
    buffersize_t get_blocks_count_of_segment(header_view_t h) { return get_blocks_count_of_segment(h, 0); }


    struct header_linked_list_access_policy {
        header_linked_list_access_policy(queue_pool_t* pool_) :pool(pool_) {}

        header_view_t get_next(header_view_t a) { return pool->get_header(a.get_next_segment_id()); }
        header_view_t get_last(header_view_t a) { return pool->get_header(a.get_last_segment_id()); }
        void set_next(header_view_t node, header_view_t to_set) { node.set_next_segment_id(to_set.get_segment_id()); }
        void set_last(header_view_t node, header_view_t to_set) { node.set_last_segment_id(to_set.get_segment_id()); }
        bool is_same_node(header_view_t a, header_view_t b) { return a == b; }
        bool is_null(header_view_t a) { return !a.is_valid(); }
    private:
        queue_pool_t* pool;
    };
    auto ll() { return linked_lists::linked_list_manipulator_t<header_view_t, header_linked_list_access_policy>(this); };

#pragma endregion

#pragma region FreeListManagement
    segment_id_t init_free_list() {
        header_view_t free_list = get_header(0);
        ll().init_node(free_list);
        free_list.set_is_free_segment(true);
        free_list.set_segment_begin(0);
        free_list.set_segment_length(get_allocatable_buffer_size_bytes() - get_header_size_bytes());
        return free_list.get_segment_id();
    }
    header_view_t get_free_list(){
        header_view_t ret = get_header(buffer->header.free_list);
        if (ret.is_valid() && ret.get_is_free_segment()) return ret;
        else return header_view_t::invalid();
    }
    void set_free_list(header_view_t h) {
        if (h.is_valid() && h.get_is_free_segment())
            buffer->header.free_list = h.get_segment_id();
        else
            buffer->header.free_list = 0;
    }

    header_view_t alloc_segment_from_free_list(header_view_t free_list) {
        if (!free_list.is_valid() || !free_list.get_is_free_segment())
            return header_view_t::invalid();

        auto allocated = free_list;
        auto free_list_remaining_blocks = get_blocks_count_of_segment(free_list);
        if (free_list_remaining_blocks < 1)
            return header_view_t::invalid();

        if (free_list_remaining_blocks == 1) {
            if (ll().is_single_node(free_list)) {
                set_free_list(header_view_t::invalid());
            } 
            else {
                auto next_free_segment = ll().next(free_list);
                ll().disconnect_node(free_list);
                set_free_list(next_free_segment);
            }
        }
        else { //`free_list_remaining_blocks > 1` 
            // we want to steal just the 1st block of this big freelist segment
            // subtract 1 block's worth of data from the total freelist length 
            // and make it so that the segment's data actually starts at the beginning of the next block
            free_list.set_segment_begin(get_block_size_bytes());
            free_list.set_segment_length(free_list.get_segment_length() - get_block_size_bytes());
            set_free_list(trim_segment_from_left(free_list));
        }
        ll().init_node(allocated);
        allocated.set_segment_begin(0);
        allocated.set_segment_length(1);
        allocated.set_is_free_segment(false);
        return allocated;
    }
    void release_queue_to_freelist(header_view_t queue_head) {
        if (!queue_head.is_valid()) return;

        //if freelist is invalid, it might be pointing to one of the blocks in this queue 
        // -> we must fetch it before we set its `is_free_list` flag to true
        auto og_free_list = get_free_list(); 
        ll().for_each(queue_head, [&](header_view_t node) {
            init_free_list_segment(node);
            });
        set_free_list(ll().prepend_list(og_free_list, queue_head));
    }

    void init_free_list_segment(header_view_t h) {
        if (!h.is_valid()) return;
        auto blocks_count = get_blocks_count_of_segment(h);
        h.set_segment_begin(0);
        h.set_segment_length(blocks_count * get_block_size_bytes() - get_header_size_bytes());
        h.set_is_free_segment(true);
    }

#pragma endregion

#pragma region SegmentManagement

    header_view_t trim_segment_from_left(header_view_t segment) {
        if (!segment.is_valid()) return segment;

        auto original_begin = segment.get_segment_begin();
        int unused_segments_count = original_begin / get_block_size_bytes();
        if (unused_segments_count <= 0)
            return segment;
        int bytes_to_trim = unused_segments_count * get_block_size_bytes();

        auto first_used_block = get_header(segment.get_segment_id() + unused_segments_count);
        if (!first_used_block.is_valid()) //this condition should never ever fail
            throw std::runtime_error("this should not happen");
        else {
            first_used_block.set_segment_begin(original_begin - bytes_to_trim);
            first_used_block.set_segment_length(segment.get_segment_length()); //length is an offset from begin -> it doesn't change
            first_used_block.set_is_free_segment(segment.get_is_free_segment());

            ll().init_node(first_used_block);
            if (!ll().is_single_node(segment)) {
                auto rest = ll().disconnect_node(segment); //returns handle to `segment.next`
                ll().prepend_list(rest, first_used_block); //inserts `first_used_block` just before `segment.next` -> same position as `segment` was originally
            }
        }
        //the freed part of the segment now starts at 0 and ends at the end of its last block
        ll().init_node(segment);
        segment.set_segment_begin(0);
        segment.set_segment_length(unused_segments_count * get_block_size_bytes() - get_header_size_bytes());
        //the `is_free_list` flag shall be set by the caller based on what he wants to do with the freed segment
        return first_used_block;
    }


    bool try_grow_queue_by_1(header_view_t* queue_head) {
        if (!queue_head) return false;

        if (!queue_head->is_valid()) { //queue is empty - we must allocate its 1st block
            auto allocated = alloc_segment_from_free_list(get_free_list());
            if (!allocated.is_valid()) return false;
            allocated.set_segment_length(1);
            *queue_head = allocated;
            return true;
        }

        auto queue_tail = ll().last(*queue_head);

        if (get_blocks_count_of_segment(queue_tail) == get_blocks_count_of_segment(queue_tail, +1)) {
            //there is still free space left in the current block
            queue_tail.set_segment_length(queue_tail.get_segment_length() + 1);
            return true;
        }

        if (use_multiblock_segments) { //try if the next block to the right is free to use
            auto next_block_to_right = get_header(queue_tail.get_segment_id() + get_blocks_count_of_segment(queue_tail));

            if (next_block_to_right.is_valid() && next_block_to_right.get_is_free_segment()) {
                auto new_block = alloc_segment_from_free_list(next_block_to_right);
                if (!new_block.is_valid()) return false; //this really should not happen, but whatever
               
                queue_tail.set_segment_length(queue_tail.get_segment_length() + 1);
                return true;
            }
        }
        //get some random free block from the free_list
        auto new_block = alloc_segment_from_free_list(get_free_list());
        if (!new_block.is_valid()) return false;
        new_block.set_segment_begin(0);
        new_block.set_segment_length(1);
        ll().insert_list(queue_tail, new_block);
        
        return true;
    }

    bool try_shrink_queue_by_1(header_view_t* out_queue_head) {
        
        if (!out_queue_head || !out_queue_head->is_valid()) return false;
        if (out_queue_head->get_segment_length() <= 0) return false;

        header_view_t queue_head = *out_queue_head;

        queue_head.set_segment_begin(queue_head.get_segment_begin() + 1);
        queue_head.set_segment_length(queue_head.get_segment_length() - 1);

        if (queue_head.get_segment_length() <= 0) { 
            //current segment is empty -> let's destroy it
            if (ll().is_single_node(queue_head)) 
                *out_queue_head = header_view_t::invalid();
            else 
                *out_queue_head = ll().next(queue_head);
            
            ll().disconnect_node(queue_head);
            init_free_list_segment(queue_head);
            set_free_list(ll().prepend_list(get_free_list(), queue_head));
        }
        else{
            //let's see if any blocks from the left side can be safely freed
            auto shrinked = trim_segment_from_left(queue_head);
            *out_queue_head = shrinked;
            if (shrinked != queue_head) { //if some blocks were freed
                ll().init_node(queue_head);
                queue_head.set_is_free_segment(true);
                set_free_list(ll().prepend_list(get_free_list(), queue_head));
            }
        }

        return true;
    }

    /// <summary>
    /// Gets pointer to the entry that was enqueued into the queue
    /// </summary>
    /// <param name="queue_head">First segment of the queue list</param>
    /// <param name="out_byte_ptr">Out value - pointer to the last enqueued entry</param>
    /// <returns>`true` IFF the queue is non-empty</returns>
    bool try_peak_front(header_view_t queue_head, byte_t** out_byte_ptr) {
        if (!queue_head.is_valid()) return false;

        auto queue_tail = ll().last(queue_head);
        if (out_byte_ptr) *out_byte_ptr = &queue_tail.get_segment_data()[queue_tail.get_segment_length() - 1 + queue_tail.get_segment_begin()];

        return true;
    }
    /// <summary>
    /// Gets pointer to the next entry that will be dequeued 
    /// </summary>
    /// <param name="queue_head">First segment of the queue list</param>
    /// <param name="out_byte_ptr">Out value - pointer to the next entry to be dequeued</param>
    /// <returns>`true` IFF the queue is non-empty</returns>
    bool try_peak_back(header_view_t queue_head, byte_t** out_byte_ptr) {
        if (!queue_head.is_valid()) return false;

        if (out_byte_ptr) *out_byte_ptr = &queue_head.get_segment_data()[queue_head.get_segment_begin()];
        return true;
    }

#pragma endregion


    //so that testing infrastructure can mess around with the internals of this class
#ifdef QUEUE_TEST_CLASS
    friend QUEUE_TEST_CLASS;
#endif
};

}

#endif