#include<iostream>
#include<cstddef>



#pragma region LinkedListManipulation


template<typename TAccessPolicy, typename TNode>
concept linked_list_manipulator_access_policy = requires(TAccessPolicy pol, TNode a, TNode b)
{
    {pol.get_next(a)} -> std::convertible_to<TNode>;
    {pol.get_last(a)} -> std::convertible_to<TNode>;
    {pol.set_next(a, b)} -> std::convertible_to<void>;
    {pol.set_last(a, b)} -> std::convertible_to<void>;
    {pol.is_same_node(a, b)} ->std::convertible_to<bool>;
};

template<typename TNode, linked_list_manipulator_access_policy<TNode> TNodeAccessPolicy>
struct linked_list_manipulator_t : private TNodeAccessPolicy {

    template<typename ...Args>
    linked_list_manipulator_t(Args ...args) : TNodeAccessPolicy(args...) {}

    TNode init_single_node(TNode a) {
        TNodeAccessPolicy& p(accessor_policy());
        p.set_next(a, a);
        p.set_last(a, a);
        return a;
    }


    void append_list(TNode n, TNode to_append) {
        TNodeAccessPolicy& p(accessor_policy());
        if (p.is_same_node(n, to_append))
            return;

        TNode n_next = p.get_next(n);
        TNode n_last = p.get_last(n);
        TNode to_append_next = p.get_next(to_append);
        TNode to_append_last = p.get_last(to_append);

        p.set_next(n, to_append);
        p.set_last(to_append, n);
        p.set_next(to_append_last, n_next);
        p.set_last(n_next, to_append_last);
    }

    void concat_lists(TNode a, TNode b) {
        TNodeAccessPolicy& p(accessor_policy());
        append_list(p.get_next(a), b);
    }

    TNode disconnect_node(TNode a) {
        TNodeAccessPolicy& p(accessor_policy());
        TNode last = p.get_last(a);
        TNode next = p.get_next(a);

        p.set_next(last, next);
        p.set_last(next, last);

        init_single_node(a);

        return next;
    }

    bool is_single_node(TNode a) {
        TNodeAccessPolicy& p(accessor_policy());
        return p.is_same_node(a, p.get_next(a));
    }

    bool is_valid_list(TNode a) {
        TNodeAccessPolicy& p(accessor_policy());
        bool ret = true;
        foreach(a, [&](TNode a) {
            if (!p.is_same_node(a, p.get_last(p.get_next(a)))) ret = false;
            if (!p.is_same_node(a, p.get_next(p.get_last(a)))) ret = false;
            });
        return ret;
    }


    template<typename TFunc>
    void foreach(TNode begin, TFunc iteration) {
        TNodeAccessPolicy& p(accessor_policy());
        TNode a = begin;
        do {
            iteration(a);
            a = p.get_next(a);
        } while (!p.is_same_node(a, begin));
    }

private:
    TNodeAccessPolicy& accessor_policy() { return *static_cast<TNodeAccessPolicy*>(this); }
};


#pragma region LinkedListManipulationTest

namespace linked_list_manipulation_tests {

    struct LinkedListNode {
        LinkedListNode* next, * last;
        int value;

        using n = LinkedListNode*;

        struct policy {
            n get_next(n a) { return a->next; }
            n get_last(n a) { return a->last; }
            void set_next(n node, n to_set) { node->next = to_set; }
            void set_last(n node, n to_set) { node->last = to_set; }
            bool is_same_node(n a, n b) { return a == b; }
        };

        struct policy_reversed {
            n get_next(n a) { return a->last; }
            n get_last(n a) { return a->next; }
            void set_next(n node, n to_set) { node->last = to_set; }
            void set_last(n node, n to_set) { node->next = to_set; }
            bool is_same_node(n a, n b) { return a == b; }
        };
    };

    void ll_test() {
        LinkedListNode a, b, c, d;
        linked_list_manipulator_t<LinkedListNode*, LinkedListNode::policy> h;
        linked_list_manipulator_t<LinkedListNode*, LinkedListNode::policy_reversed> h2;

        a.value = 10; h.init_single_node(&a);
        b.value = 11; h.init_single_node(&b);
        c.value = 12; h.init_single_node(&c);
        d.value = 13; h.init_single_node(&d);

#define TEST_PRINT(n) printf("it %d(" #n "): ", h.is_valid_list(&n)); h.foreach(&n, [](LinkedListNode* n) {printf("%d, ", n->value); }); printf(" | "); h2.foreach(&n, [](LinkedListNode* n) {printf("%d, ", n->value); }); printf("\n")

        TEST_PRINT(a);
        h.disconnect_node(&a);
        TEST_PRINT(a);
        h.concat_lists(&a, &b);
        TEST_PRINT(a);
        h.concat_lists(&c, &d);
        TEST_PRINT(c);
        h.concat_lists(&a, &c);
        TEST_PRINT(a);

        h.disconnect_node(&a);
        TEST_PRINT(a);
        TEST_PRINT(b);
        h.disconnect_node(&c);
        TEST_PRINT(c);
        TEST_PRINT(b);
        h.disconnect_node(&b);
        TEST_PRINT(d);
        TEST_PRINT(b);

#undef TEST_PRINT
    }
}

#pragma endregion


#pragma endregion



using byte_t = unsigned char;
using buffersize_t = std::size_t;
using segment_id_t = std::uint32_t;






struct standard_header_policy_t {
    static constexpr int get_header_size_bytes() { return sizeof(segment_header_view_t::packed_header_t); }
    static constexpr std::size_t get_header_size_bits() { return get_header_size_bytes() * 8; }


    struct segment_header_view_t {
    public:
        friend standard_header_policy_t;
        segment_header_view_t(byte_t* segment_start, segment_id_t segment_index) : header_ptr_raw(segment_start), segment_id(segment_index){}

        segment_id_t get_next_segment_id() { return get_header()->next_segment; }
        void set_next_segment_id(segment_id_t value) { get_header()->next_segment = (byte_t)(value); }
        segment_id_t get_last_segment_id(){ return get_header()->last_segment; }
        void set_last_segment_id(segment_id_t value) { get_header()->last_segment = (byte_t)(value); }

        buffersize_t get_segment_begin() {
            if (get_header()->is_full_from_begin) return 0;
            auto begin_info_extension_header_1st_byte = get_begin_info_extension_header_first_byte_only();
            if (!begin_info_extension_header_1st_byte->is_2_byte_number)
                return begin_info_extension_header_1st_byte->lower_7_bits;
            else {
                auto begin_info_extension_header = get_begin_info_extension_header();
                return begin_info_extension_header->first_byte.lower_7_bits | (begin_info_extension_header->bits_8_to_15 << 7);
            }
        }
        void set_segment_begin(buffersize_t value) {
            bool is_full_from_begin = (value == 0);
            get_header()->is_full_from_begin = is_full_from_begin;
            if (!is_full_from_begin) {
                bool size_fits_into_1_byte = (value < 128);
                if (size_fits_into_1_byte) {
                    auto extension_header = get_begin_info_extension_header_first_byte_only();
                    extension_header->is_2_byte_number = false;
                    extension_header->lower_7_bits = (byte_t)(value);
                }
                else {
                    auto extension_header = get_begin_info_extension_header();
                    extension_header->first_byte.is_2_byte_number = true;
                    extension_header->first_byte.lower_7_bits = (byte_t)(value & 0x7F);
                    extension_header->bits_8_to_15 = (byte_t)((value >> 7) & 0xFF);
                }
            }
        }

        buffersize_t get_segment_length(){
            auto packed = get_header();
            return packed->segment_length_lower | (packed->segment_length_upper << 8);
        }
        void set_segment_length(buffersize_t value) {
            auto packed = get_header();
            packed->segment_length_lower = (byte_t)(value & 0xFF);
            packed->segment_length_upper = (byte_t)((value & 0xF00) >> 8);
        }
        bool get_is_free_segment() { return get_header()->is_free_segment; }
        void set_is_free_segment(bool value) { get_header()->is_free_segment = value; }

        byte_t* get_segment_data() { return header_ptr_raw + get_header_size_bytes(); }

        segment_id_t get_segment_id() { return segment_id; }

        bool is_valid() { return (bool)header_ptr_raw; }
        static segment_header_view_t invalid() { return segment_header_view_t(NULL, 0); }
    private:
        
        struct long_segment_begin_info_t {
            struct first_byte_t {
                byte_t lower_7_bits : 7;
                byte_t is_2_byte_number : 1;
            };
            first_byte_t first_byte; //according to C spec, first field must always be put at the beginning of the struct
            byte_t bits_8_to_15;
        };
        static_assert((std::intptr_t)(&(((long_segment_begin_info_t*)0)->first_byte)) == 0, "For some reason the compiler doesn't put the first_byte part of long_segment_begin_info_t to its beginning even though it should according to C spec");
        static_assert((std::intptr_t)(&(((long_segment_begin_info_t*)0)->bits_8_to_15)) == 1, "For some reason the compiler doesn't put the bits_8_to_15 part of long_segment_begin_info_t into its 2nd byte");
        static_assert(sizeof(long_segment_begin_info_t::first_byte_t) == 1, "Long segment begin info must have an exactly 1 byte header");
        static_assert(sizeof(long_segment_begin_info_t) == 2, "Long segment begin info must be exactly 2 bytes");

        struct packed_header_t {
            byte_t next_segment : 7;
            byte_t is_free_segment : 1;
            byte_t last_segment : 7;
            byte_t is_full_from_begin : 1;
            byte_t segment_length_lower;
            byte_t segment_length_upper : 4;
        };//fields do not really need to be packed in memory exactly in the order they are written, just being 4 bytes long is enough
        static_assert(sizeof(packed_header_t) == 4, "Segment header is supposed to take exactly 5 bytes");

        segment_id_t segment_id;
        byte_t* header_ptr_raw;
        packed_header_t* get_header() { return reinterpret_cast<packed_header_t*>(header_ptr_raw); }
        long_segment_begin_info_t::first_byte_t* get_begin_info_extension_header_first_byte_only() { return reinterpret_cast<long_segment_begin_info_t::first_byte_t*>(header_ptr_raw + get_header_size_bytes()); }
        long_segment_begin_info_t* get_begin_info_extension_header() { return reinterpret_cast<long_segment_begin_info_t*>(get_begin_info_extension_header_first_byte_only()); }
    };
};


struct queue_handle_t {
    queue_handle_t(buffersize_t segment_id_): segment_id(segment_id_){}

    segment_id_t segment_id;

    static queue_handle_t empty() { return queue_handle_t(~0 + 1); }
    static queue_handle_t uninitialized(){ return queue_handle_t(~0); }
    bool is_empty() { return segment_id == (~0 + 1); }
    bool is_uninitialized() { return segment_id == ~0; }

};
static_assert(sizeof(queue_handle_t) == sizeof(segment_id_t), "");



using THeaderPolicy = standard_header_policy_t;
template<buffersize_t segment_alignment=20>
struct queue_pool_t{
public:
    queue_pool_t(byte_t* buffer, buffersize_t buffer_size) : buffer_raw(buffer), buffer_size_raw(buffer_size), ll_helper(this) {
        auto free_list = get_free_list();;
        ll_helper.init_single_node(free_list);
        free_list.set_is_free_segment(true);
        free_list.set_segment_begin(0);
        free_list.set_segment_length(get_buffer_size());
    }

    queue_handle_t make_queue() {
        return queue_handle_t(0);
    }
    void enqueue_byte(queue_handle_t &handle, byte_t to_enqueue) {
        
    }
    byte_t dequeue_byte(queue_handle_t &handle) {
        return 0;
    }
    void destroy_queue(queue_handle_t &handle){}


private:
    using header_view_t = typename THeaderPolicy::segment_header_view_t;

    byte_t* buffer_raw;
    buffersize_t buffer_size_raw;
    segment_id_t free_list_id = 0;

    byte_t* get_buffer() { return buffer_raw; }
    buffersize_t get_buffer_size() { return buffer_size_raw; }

    byte_t* get_segment_start(segment_id_t segment_index) { return &(get_buffer()[segment_index * segment_alignment]); }
    header_view_t get_header(segment_id_t segment_index) { return header_view_t(get_segment_start(segment_index), segment_index); }
    header_view_t get_free_list() {
        header_view_t ret = get_header(free_list_id);
        return ret;
    }
    header_view_t alloc_segment_from_free_list() {
        auto free_list = get_free_list();
        if (!free_list.get_is_free_segment())
            return header_view_t::invalid();

        if (free_list.get_segment_length() < segment_alignment) {
            
        }
    }

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
    linked_list_manipulator_t<header_view_t, header_list_access_policy> ll_helper;


};







int main(){

    linked_list_manipulation_tests::ll_test();
    return 0;

    byte_t buffer[1024];

    //auto h = standard_header_policy_t::segment_header_view_t(buffer, 0);
    //
    //h.set_is_free_segment(false);
    //h.set_next_segment_id(0x5E);
    //h.set_last_segment_id(0x3B);
    //h.set_segment_length(0x122);
    //h.set_segment_begin(0x1122);
    //
    //printf("next: %llx, last: %llx, length: %llx, begin: %llx, is_free: %d\n", h.get_next_segment_id(), h.get_last_segment_id(), h.get_segment_length(), h.get_segment_begin(), h.get_is_free_segment());
    
    
    queue_pool_t<> pool(buffer, 1024);


    printf("Hello world!\n");
    return 0;
}