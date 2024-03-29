#ifndef MEMORY_POLICY__guard____ASfd1456ADShfgjffdsdf654g98g4d6f5fd
#define MEMORY_POLICY__guard____ASfd1456ADShfgjffdsdf654g98g4d6f5fd

#include "basic_definitions.h"

namespace markussecundus::queue_pooling::memory_policies{
        
    /// <summary>
    /// Contract for a view that facilitates access to fields of a segment header of a queue_pool.
    /// </summary>
    template<typename THeaderView>
    concept header_view = requires(THeaderView pol, segment_id_t segment_id, buffersize_t buffersize, bool flag, byte_t*bytebuffer, THeaderView view_a, THeaderView view_b)
    {
        //id of the next segment in the queue linked list
        {pol.get_next_segment_id()} -> std::convertible_to<segment_id_t>;
        {pol.set_next_segment_id(segment_id)} -> std::convertible_to<void>;

        //id of the last segment in the queue linked list
        {pol.get_last_segment_id()} -> std::convertible_to<segment_id_t>;
        {pol.set_last_segment_id(segment_id)} -> std::convertible_to<void>;

        //position of the byte in segment_data where actual data begins (gets incremented by dequeue)
        {pol.get_segment_begin()} -> std::convertible_to<buffersize_t>;
        {pol.set_segment_begin(buffersize)} -> std::convertible_to<void>;

        //length of the data (gets incremented by enqueue, decremented by dequeue)
        {pol.get_segment_length()} -> std::convertible_to<buffersize_t>;
        {pol.set_segment_length(buffersize)} -> std::convertible_to<void>;

        //whether the segment is marked as a part of the free list
        {pol.get_is_free_segment()} -> std::convertible_to<bool>;
        {pol.set_is_free_segment(flag)} -> std::convertible_to<void>;

        //pointer to the beginning of segment's data (after the header ends)
        {pol.get_segment_data()} -> std::convertible_to<byte_t*>;
        //id of the segment for debugging purposes - carried in the view itself, not in the header
        {pol.get_segment_id()} -> std::convertible_to<segment_id_t>;
        //if the view points to a valid segment header
        //if `false`, any read operation on the header is undefined behaviour!
        {pol.is_valid()} -> std::convertible_to<bool>;

        //if two views point to the same segment header
        {view_a == view_b} -> std::convertible_to<bool>;

        //gets an invalid header_view instance
        {THeaderView::invalid()} -> std::convertible_to<THeaderView>;
    };
    /// <summary>
    /// Object specifying details about how memory shall be handled (block size, header encoding etc.) by a queue pool.
    /// </summary>
    template<typename THeaderPolicy>
    concept memory_policy = 
        header_view<typename THeaderPolicy::segment_header_view_t> &&
        requires (THeaderPolicy pol, byte_t *byteptr, typename THeaderPolicy::segment_id_t segment_id){
        //fixed ammount of bytes that needs to be reserved as header overhead
        {THeaderPolicy::get_header_size_bytes()} -> std::convertible_to<buffersize_t>;
        //fixed ammount of bytes in a single queue block
        {pol.get_block_size_bytes()} -> std::convertible_to<buffersize_t>;
        //how many blocks can be addressed in total considering the data types used by the header
        {pol.get_addressable_blocks_count()} -> std::convertible_to<segment_id_t>;
        //create a header view, starting in a specified segment, displaying specified segment id
        {pol.make_header_view(byteptr, segment_id)} -> std::convertible_to<typename THeaderPolicy::segment_header_view_t>;
    }
    //type big enough for storing segment_ids in memory (important to save as much space as possible in adapter.cpp's handle pool)
    && std::convertible_to<typename THeaderPolicy::packed_segment_id_t, typename THeaderPolicy::segment_id_t>
    //type safe for performing arithmetics on segment_ids
    && std::convertible_to<typename THeaderPolicy::segment_id_t, segment_id_t>;

    /// <summary>
    /// Standard memory policy whose headers have 4 byte overhead and its block size is specified as a runtime argument.
    /// </summary>
    struct standard_memory_policy {
    public:
        using packed_segment_id_t = std::uint8_t;
        using segment_id_t = std::uint16_t;

        struct segment_header_view_t {
        private:
            friend standard_memory_policy;
            segment_header_view_t(byte_t* segment_start, segment_id_t segment_index) : header_ptr_raw(segment_start), segment_id(segment_index){}
        public:
            bool operator==(segment_header_view_t other)const { return this->header_ptr_raw == other.header_ptr_raw; }

            segment_id_t get_next_segment_id() { return get_header()->next_segment; }
            void set_next_segment_id(segment_id_t value) { get_header()->next_segment = (byte_t)(value); }
            segment_id_t get_last_segment_id(){ return get_header()->last_segment; }
            void set_last_segment_id(segment_id_t value) { get_header()->last_segment = (byte_t)(value);  }

            buffersize_t get_segment_begin() {
                //If data begins on position 0, we only needs 1 bit to carry that info.
                //If it doesn't, then the byte on position 0 is free to carry 1 byte of info about where the segment begins.
                //If 1 byte is not enough, then the byte on position 2 must also be free, so we can safely misuse it 
                // to carry even more bits of the segment_begin number.
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

            byte_t* get_segment_data() { return reinterpret_cast<byte_t*>(header_ptr_raw) + get_header_size_bytes(); }

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
            //for some reason GCC doesn't accept the expressions in these two asserts as constexpr (even though MSVC does)
            //static_assert((std::intptr_t)(&(((long_segment_begin_info_t*)0)->first_byte)) == 0, "For some reason the compiler doesn't put the first_byte part of long_segment_begin_info_t to its beginning even though it should according to C spec");
            //static_assert((std::intptr_t)(&(((long_segment_begin_info_t*)0)->bits_8_to_15)) == 1, "For some reason the compiler doesn't put the bits_8_to_15 part of long_segment_begin_info_t into its 2nd byte");
            static_assert(sizeof(typename long_segment_begin_info_t::first_byte_t) == 1, "Long segment begin info must have an exactly 1 byte header");
            static_assert(sizeof(long_segment_begin_info_t) == 2, "Long segment begin info must be exactly 2 bytes");

            struct packed_header_t {
                byte_t next_segment;
                byte_t last_segment;
                byte_t segment_length_lower;
                byte_t segment_length_upper : 4;
                byte_t is_free_segment : 1;
                byte_t is_full_from_begin : 1;
            };//fields do not really need to be packed in memory exactly in the order they are written, just being 4 bytes long is enough
            static_assert(sizeof(packed_header_t) == 4, "Segment header is supposed to take exactly 5 bytes");

            void* header_ptr_raw;
            segment_id_t segment_id;
            packed_header_t* get_header() { return reinterpret_cast<packed_header_t*>(header_ptr_raw); }
            long_segment_begin_info_t::first_byte_t* get_begin_info_extension_header_first_byte_only() { return reinterpret_cast<long_segment_begin_info_t::first_byte_t*>(reinterpret_cast<byte_t*>(header_ptr_raw) + get_header_size_bytes()); }
            long_segment_begin_info_t* get_begin_info_extension_header() { return reinterpret_cast<long_segment_begin_info_t*>(get_begin_info_extension_header_first_byte_only()); }
        };


        standard_memory_policy(buffersize_t block_size_) : block_size(block_size_) {}

        static constexpr buffersize_t get_header_size_bytes() { return sizeof(typename segment_header_view_t::packed_header_t); }
        buffersize_t get_block_size_bytes() { return block_size; }
        static constexpr segment_id_t get_addressable_blocks_count() { return 1<<8; }
        segment_header_view_t make_header_view(byte_t* segment_start, segment_id_t segment_index) { return segment_header_view_t(segment_start, segment_index); }

    private:
        buffersize_t block_size;
    };

}

#endif