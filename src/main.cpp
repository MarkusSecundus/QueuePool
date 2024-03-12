#include<iostream>
#include<cstddef>


//see https://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif
#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif




using byte_t = unsigned char;
using buffersize_t = std::size_t;


struct SegmentHeaderUncompressed {
    buffersize_t next_segment;
    buffersize_t last_segment;
    buffersize_t segment_length;
    bool is_free_segment;
};

struct BasicHeaderPolicy {
    static constexpr int get_header_size_bytes() { return sizeof(packed_header_t); }
    static constexpr std::size_t get_header_size_bits() { return get_header_size_bytes() * 8; }


    static SegmentHeaderUncompressed unpack_header(byte_t* header_ptr) {
        packed_header_t* packed = reinterpret_cast<packed_header_t*>(header_ptr);
        SegmentHeaderUncompressed ret;
        ret.next_segment = packed->next_lower | (packed->next_upper << 8);
        ret.last_segment = packed->last_lower | (packed->last_upper << 8);
        ret.segment_length = packed->segment_length_lower | (packed->segment_length_upper << 8);
        ret.is_free_segment = packed->is_free_segment;
        return ret;
    }
    static void pack_header(byte_t* header_ptr, SegmentHeaderUncompressed header_data) {
        packed_header_t* packed = reinterpret_cast<packed_header_t*>(header_ptr);
        packed->next_lower = (byte_t)(header_data.next_segment & 0xFF);
        packed->next_upper = (byte_t)((header_data.next_segment & 0xF00) >> 8);
        packed->last_lower = (byte_t)(header_data.last_segment & 0xFF);
        packed->last_upper = (byte_t)((header_data.last_segment & 0xF00) >> 8);
        packed->segment_length_lower = (byte_t)(header_data.segment_length & 0xFF);
        packed->segment_length_upper = (byte_t)((header_data.segment_length & 0xF00) >> 8);
        packed->is_free_segment = header_data.is_free_segment;
    }

private: 
    PACK(struct packed_header_t {
        byte_t next_lower;
        byte_t next_upper : 4;
        byte_t last_upper : 4;
        byte_t last_lower;
        byte_t segment_length_lower;
        byte_t segment_length_upper : 4;
        byte_t is_free_segment : 1;
    });
    static_assert(sizeof(packed_header_t) == 5, "Segment header is supposed to take exactly 5 bytes");
};



struct queue_handle_t {
    queue_handle_t(buffersize_t queue_start_offset_): queue_start_offset(queue_start_offset_){}

    buffersize_t queue_start_offset;

    static queue_handle_t empty() { return queue_handle_t(~0); }
    bool is_empty() { return queue_start_offset == ~0; }

};
static_assert(sizeof(queue_handle_t) == sizeof(buffersize_t), "");



template<typename THeaderPolicy=BasicHeaderPolicy, buffersize_t segment_granularity=32>
struct QueuePool : private THeaderPolicy{
public:
    queue_handle_t make_queue() {
        return queue_handle_t();
    }
    void enqueue_byte(queue_handle_t &handle, byte_t to_enqueue) {
        
    }
    byte_t dequeue_byte(queue_handle_t &handle) {
        return 0;
    }
    void destroy_queue(queue_handle_t &handle){}


private:
    byte_t* buffer_raw;
    buffersize_t buffer_size_raw;

    queue_handle_t get_buffer_header() { return *reinterpret_cast<buffer_header_t*>(buffer_raw); }
    byte_t* get_buffer() { return buffer_raw + sizeof(buffer_header_t); }
    buffersize_t get_buffer_size() { return buffer_size_raw - sizeof(buffer_header_t); }

    struct buffer_header_t {
        queue_handle_t free_list;
    };
};



int main(){

    byte_t buffer[10];

    SegmentHeaderUncompressed h;
    h.next_segment = 8029;
    h.last_segment = 543;
    h.segment_length = 374;
    h.is_free_segment = true;

    printf("next: %llx, last: %llx, length: %llx, is_free: %d\n", h.next_segment, h.last_segment, h.segment_length, h.is_free_segment);

    BasicHeaderPolicy::pack_header(buffer, h);

    {
        auto h = BasicHeaderPolicy::unpack_header(buffer);
        printf("next: %llx, last: %llx, length: %llx, is_free: %d\n", h.next_segment, h.last_segment, h.segment_length, h.is_free_segment);
    }


    printf("Hello world!\n");
    return 0;
}