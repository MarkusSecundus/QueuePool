#include<iostream>
#include<cstddef>

using offset_t = std::size_t;
using byte_t = unsigned char;

using buffer_intptr_t = offset_t;


struct SegmentHeaderUncompressed {
    buffer_intptr_t next_segment;
    buffer_intptr_t last_segment;
    offset_t segment_length;
    bool is_free_segment;
};

struct BasicHeaderPolicy {
    constexpr int get_header_size_bytes() { return 5; }
    constexpr offset_t get_header_size_bits() { return get_header_size_bytes() * 8; }


    SegmentHeaderUncompressed unpack_header(byte_t* header_ptr) const {
        SegmentHeaderUncompressed ret;
        ret.next_segment = header_ptr[0] | (header_ptr[1] & 0xF) << 8;
        ret.last_segment = header_ptr[2] | (header_ptr[1] & 0xF0) << 4;
        ret.segment_length = header_ptr[3] | (header_ptr[4] & 0xF) << 8;
        ret.is_free_segment = !!(header_ptr[4] & 0x10);
        return ret;
    }
    void pack_header(byte_t* header_ptr, SegmentHeaderUncompressed header_data) {
        PackedHeader* hdr = reinterpret_cast<PackedHeader*>(header_ptr);
        header_ptr[0] = header_data.next_segment & 0xFF;
        header_ptr[1] = ((header_data.next_segment & 0xF00) >> 8) | ((header_data.last_segment & 0xF00) >> 4);
        header_ptr[2] = header_data.last_segment & 0xFF;
        header_ptr[3] = (byte_t)(header_data.segment_length & 0xFF);
        header_ptr[4] = (byte_t)((header_data.segment_length & 0xF00) >> 8) | (((byte_t)!!header_data.is_free_segment) << 4);
        printf("0: %x, 1: %x, 2: %x, 3: %x, 4: %x\n", header_ptr[0], header_ptr[1], header_ptr[2], header_ptr[3], header_ptr[4]);
    }

    struct PackedHeader{
        byte_t next_lower;
        byte_t next_upper : 4;
        byte_t last_upper : 4;
        byte_t last_lower;
        byte_t segment_length_lower;
        byte_t segment_length_upper : 4;
        byte_t is_free_segment : 1;
    };
    static_assert(sizeof(PackedHeader) == 5, "dsads");
};


template<typename THeaderPolicy>
struct QueuePool : private THeaderPolicy {

};



int main(){
    byte_t buffer[10];
    
    BasicHeaderPolicy pol;
    SegmentHeaderUncompressed h;
    h.next_segment = 122;
    h.last_segment = 543;
    h.segment_length = 174;
    h.is_free_segment = true;
    
    printf("%x %d\n", 0xF00 >> 8 | 0xA00 >> 4, sizeof(BasicHeaderPolicy::PackedHeader));

    printf("next: %llx, last: %llx, length: %llx, is_free: %d\n", h.next_segment, h.last_segment, h.segment_length, h.is_free_segment);
    {
        auto h = pol.unpack_header(buffer);
        printf("next: %llx, last: %llx, length: %llx, is_free: %d\n", h.next_segment, h.last_segment, h.segment_length, h.is_free_segment);
    }

    pol.pack_header(buffer, h);

    {
        auto h = pol.unpack_header(buffer);
        printf("next: %llx, last: %llx, length: %llx, is_free: %d\n", h.next_segment, h.last_segment, h.segment_length, h.is_free_segment);
    }

    


    printf("Hello world!\n");
    return 0;
}