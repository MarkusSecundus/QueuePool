#ifndef QUEUE_POOL__guard___fds4g89dfv46ds51d6a4d9as4d6sagr
#define QUEUE_POOL__guard___fds4g89dfv46ds51d6a4d9as4d6sagr


#include "basic_definitions.h"
#include "linked_list.h"
using namespace linked_lists;
#include "memory_policy.h"
using namespace memory_policies;



namespace queue_pooling{




template<memory_policy TMemoryPolicy=standard_memory_policy<20>>
struct queue_pool_t {

public:
    using segment_id_t = TMemoryPolicy::segment_id_t;
    struct queue_handle_t {
        queue_handle_t(segment_id_t segment_id_) :segment_id(segment_id_) {}
        static constexpr queue_handle_t from_header(typename queue_pool_t::header_view_t h) { return !h.is_valid() ? queue_handle_t::empty(): queue_handle_t(h.get_segment_id()); }

        segment_id_t get_segment_id()const { return segment_id; }

        static constexpr queue_handle_t uninitialized() { return queue_handle_t(~0); }
        static constexpr queue_handle_t error() { return queue_handle_t(~0); }
        static constexpr queue_handle_t empty() { return queue_handle_t((segment_id_t)(std::size_t)(~0) - 1); }


        bool is_uninitialized() { return segment_id == uninitialized().segment_id; }
        bool is_empty() { return segment_id == empty().segment_id; }
        bool is_valid() { return !(is_empty() || is_uninitialized()); }

    private:
        segment_id_t segment_id;
    };



    queue_pool_t(byte_t* buffer, buffersize_t buffer_size) : buffer_raw(buffer), buffer_size_raw(buffer_size) {
        if (buffer_size > TMemoryPolicy::get_max_addresable_memory_bytes())
            throw std::runtime_error("buffer is too big!");
        *get_free_list_id_ptr() = init_free_list();
    }

    queue_handle_t make_queue() {
        return queue_handle_t::empty();
    }
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
    bool try_peek_byte(queue_handle_t* handle_ptr, byte_t* out_byte) {
        if (!handle_ptr->is_valid())
            return false;

        return try_peak_back(get_header(handle_ptr->get_segment_id()), &out_byte);
    }
    bool try_dequeue_byte(queue_handle_t* handle_ptr, byte_t* out_byte) {
        if (!handle_ptr->is_valid())
            return false;

        auto head = get_header(handle_ptr->get_segment_id());
        if (try_shrink_queue_by_1(&head) && try_peak_back(head, &out_byte)) {
            *handle_ptr = queue_handle_t::from_header(head);
            return true;
        }
        return false;
    }
    void destroy_queue(queue_handle_t* handle_ptr)
    {
        if (!handle_ptr->is_valid())return;

        release_queue_to_freelist(get_header(handle_ptr->get_segment_id()));
        *handle_ptr = queue_handle_t::empty();
    }


private:
    using header_view_t = typename TMemoryPolicy::segment_header_view_t;
    static constexpr buffersize_t get_segment_alignment() { return TMemoryPolicy::get_segment_alignment(); }

    byte_t* buffer_raw;
    buffersize_t buffer_size_raw;


    byte_t* get_buffer() { return buffer_raw; }
    buffersize_t get_buffer_size() { return buffer_size_raw; }
    buffersize_t get_allocatable_buffer_size() { return get_max_segment_id() * get_segment_alignment(); }

    byte_t* get_segment_start(segment_id_t segment_index) { return &(get_buffer()[segment_index * get_segment_alignment()]); }
    segment_id_t get_max_segment_id() { return (segment_id_t)(get_buffer_size() / get_segment_alignment()); }

    header_view_t get_header(segment_id_t segment_index) {
        if (segment_index < 0 || segment_index >= get_max_segment_id()) return header_view_t::invalid();
        return header_view_t(get_segment_start(segment_index), segment_index);
    }



    segment_id_t free_list_id___ = 0;
    segment_id_t* get_free_list_id_ptr() { return &free_list_id___; }

    segment_id_t init_free_list() {
        header_view_t free_list = get_header(0);
        ll().init_node(free_list);
        free_list.set_is_free_segment(true);
        free_list.set_segment_begin(0);
        free_list.set_segment_length(get_allocatable_buffer_size());
        return free_list.get_segment_id();
    }

    header_view_t alloc_segment_from_free_list() {
        auto free_list = get_header(*get_free_list_id_ptr());
        if (!free_list.get_is_free_segment())
            return header_view_t::invalid();


        if (free_list.get_segment_begin() != 0) throw std::runtime_error("This should not happen!");

        auto ret = free_list;
        auto free_list_remaining = (std::ptrdiff_t)free_list.get_segment_length() - (std::ptrdiff_t)get_segment_alignment();
        if (free_list_remaining < 0)
            return header_view_t::invalid();

        if (free_list_remaining == 0) {
            if (ll().is_single_node(free_list)) {} //nothing special to do here - the segment will in any case get marked as non-free and that will be the end of free list
            else {
                auto next_free_segment = ll().next(free_list);
                ll().disconnect_node(free_list);
                *get_free_list_id_ptr() = next_free_segment.get_segment_id();
            }
        }
        else {
            free_list.set_segment_begin(get_segment_alignment());
            free_list.set_segment_length(free_list.get_segment_length() - get_segment_alignment());

            *get_free_list_id_ptr() = shrink_segment_from_left_according_to_its_begin(free_list).get_segment_id();
        }
        ret.set_is_free_segment(false);
        ret.set_segment_begin(0);
        ret.set_segment_length(0);
        ll().init_node(ret);
        return ret;
    }

    header_view_t shrink_segment_from_left_according_to_its_begin(header_view_t h) {
        auto begin = h.get_segment_begin();
        int segments_count = begin / get_segment_alignment();
        if (segments_count <= 0)
            return h;
        int bytes_count = segments_count * get_segment_alignment();

        auto next_part_of_this_continuous_segment = get_header(h.get_segment_id() + segments_count);
        if (next_part_of_this_continuous_segment.is_valid()) {
            next_part_of_this_continuous_segment.set_segment_begin(begin - bytes_count);
            next_part_of_this_continuous_segment.set_segment_length(h.get_segment_length());
            next_part_of_this_continuous_segment.set_is_free_segment(h.get_is_free_segment());

            ll().init_node(next_part_of_this_continuous_segment);
            ll().swap_nodes(h, next_part_of_this_continuous_segment);
        }
        else {
            ll().disconnect_node(h);
        }
        return next_part_of_this_continuous_segment;
    }


    bool try_grow_queue_by_1(header_view_t* queue_head) {
        if (!queue_head) return false;

        if (!queue_head->is_valid()) {
            auto ret = alloc_segment_from_free_list();
            if (!ret.is_valid()) return false;
            ret.set_segment_length(1);
            dbg_enqueue("initializing queue! " << (int)(ret.get_segment_begin() + ret.get_segment_length() + TMemoryPolicy::get_header_size_bytes()) << " / " << (int)get_segment_alignment() << "\n");
            *queue_head = ret;
            return true;
        }

        auto queue_tail = ll().last(*queue_head);

        auto segment_size = queue_tail.get_segment_begin() + queue_tail.get_segment_length() + TMemoryPolicy::get_header_size_bytes();
        //dbg_enqueue( << "testing to take next byte - segment_size: " << segment_size << " / " << get_segment_alignment() << "\n");
        if (math::divide_round_up(segment_size, get_segment_alignment()) == math::divide_round_up((segment_size + 1), get_segment_alignment())) {
            //dbg_enqueue( << "taking next byte - segment_size: " << segment_size << " / " << get_segment_alignment() << "\n");
            queue_tail.set_segment_length(queue_tail.get_segment_length() + 1);
            return true;
        }
        if (USE_LARGE_SEGMENTS()) {
            auto next_block = get_header(queue_tail.get_segment_id() + get_blocks_count_of_segment(queue_tail));
            dbg_enqueue("peeking next block(" << next_block.is_valid() << ")..." << " id: " << (int)queue_tail.get_segment_id() << " -> " << (int)next_block.get_segment_id() << " < " << (int)get_max_segment_id());
            if (next_block.is_valid()) dbg_enqueue(" free(" << next_block.get_is_free_segment() << ") length : " << next_block.get_segment_length() << "\n");
            else dbg_enqueue("\n");
            if (next_block.is_valid() && next_block.get_is_free_segment() && next_block.get_segment_length() >= get_segment_alignment()) {
                dbg_enqueue("extending block! ...");
                next_block.set_segment_begin(get_segment_alignment());
                next_block.set_segment_length(next_block.get_segment_length() - get_segment_alignment());
                auto new_free_block_begin = shrink_segment_from_left_according_to_its_begin(next_block);
                if (next_block.get_segment_id() == *get_free_list_id_ptr()) *get_free_list_id_ptr() = new_free_block_begin.get_segment_id();
                dbg_enqueue("new_free_block_id: " << (int)new_free_block_begin.get_segment_id() << "\n");

                queue_tail.set_segment_length(queue_tail.get_segment_length() + 1);
                return true;
            }
        }
        {
            auto new_block = alloc_segment_from_free_list();
            dbg_enqueue("allocating new block(" << new_block.is_valid() << ")!...");
            if (!new_block.is_valid()) return false;
            dbg_enqueue("id: " << (int)new_block.get_segment_id() << ", free(" << (int)new_block.get_is_free_segment() << "), length: " << (int)new_block.get_segment_length() << "\n");
            new_block.set_segment_begin(0);
            new_block.set_segment_length(1);
            ll().insert_list(queue_tail, new_block);
        }
        return true;
    }

    bool try_shrink_queue_by_1(header_view_t* out_queue_head) {
        
        if (!out_queue_head || !out_queue_head->is_valid()) return false;
        if (out_queue_head->get_segment_length() <= 0) return false;

        header_view_t queue_head = *out_queue_head;

        queue_head.set_segment_length(queue_head.get_segment_length() - 1);
        queue_head.set_segment_begin(queue_head.get_segment_begin() + 1);

        if (queue_head.get_segment_length() <= 0) {
            if (ll().is_single_node(queue_head)) 
                *out_queue_head = header_view_t::invalid();
            else 
                *out_queue_head = ll().next(queue_head);
            
            ll().disconnect_node(queue_head);
            init_free_list_segment(queue_head);
            auto free_list = get_header(*get_free_list_id_ptr());
            if (!free_list.get_is_free_segment())
                *get_free_list_id_ptr() = queue_head.get_segment_id();
            else
                ll().prepend_list(free_list, queue_head);
        }

        return true;
    }


    bool try_peak_front(header_view_t queue_head, byte_t** out_byte_ptr) {
        if (!queue_head.is_valid()) return false;

        auto queue_tail = ll().last(queue_head);
        if (out_byte_ptr) *out_byte_ptr = &queue_tail.get_segment_data()[queue_tail.get_segment_begin() + queue_tail.get_segment_length() - 1];

        return true;
    }
    bool try_peak_back(header_view_t queue_head, byte_t** out_byte_ptr) {
        if (!queue_head.is_valid()) return false;

        if (out_byte_ptr) *out_byte_ptr = &queue_head.get_segment_data()[queue_head.get_segment_begin()];
        return true;
    }

    void release_queue_to_freelist(header_view_t queue_head) {
        if (!queue_head.is_valid()) return;

        //dbg_destroy("<destroying the queue>\n");

        auto free_list = get_header(*get_free_list_id_ptr());
        bool free_list_exists = free_list.get_is_free_segment(); //edgecase when free_list is empty and points exactly at queue_head -> we need to ask now, before we set it's free_list flag to true
        ll().for_each(queue_head, [&](header_view_t node) {init_free_list_segment(node); });
        if (!free_list_exists) {
            *get_free_list_id_ptr() = queue_head.get_segment_id();
        }
        else {
            ll().prepend_list(free_list, queue_head);
        }
    }

    void init_free_list_segment(header_view_t h) {
        if (!h.is_valid()) return;
        auto blocks_count = get_blocks_count_of_segment(h);
        h.set_segment_begin(0);
        h.set_segment_length(blocks_count * get_segment_alignment());
        h.set_is_free_segment(true);
    }


    static constexpr bool USE_LARGE_SEGMENTS() { return false; }

    buffersize_t get_blocks_count_of_segment(header_view_t h) { return math::divide_round_up(h.get_segment_begin() + h.get_segment_length() + TMemoryPolicy::get_header_size_bytes(), get_segment_alignment()); }


    struct header_list_access_policy {
    public:
        header_list_access_policy(queue_pool_t *pool_):pool(pool_){}

        header_view_t get_next(header_view_t a) { return pool->get_header(a.get_next_segment_id()); }
        header_view_t get_last(header_view_t a) { return pool->get_header(a.get_last_segment_id()); }
        void set_next(header_view_t node, header_view_t to_set) { node.set_next_segment_id(to_set.get_segment_id()); }
        void set_last(header_view_t node, header_view_t to_set) { node.set_last_segment_id(to_set.get_segment_id()); }
        bool is_same_node(header_view_t a, header_view_t b) { return a.get_segment_id() == b.get_segment_id(); }
    private:
        queue_pool_t *pool;
    };
    auto ll() { return linked_list_manipulator_t<header_view_t, header_list_access_policy>(this); };


#ifdef QUEUE_TEST_CLASS
    friend QUEUE_TEST_CLASS;
#endif
};




















}

#endif