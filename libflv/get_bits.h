#ifndef __GET_BITS_H__
#define __GET_BITS_H__

#endif // __GET_BITS_H__

#include <cassert>
#include <cstdint>
#include <cstring>

namespace nx {

class GetBitContext {
private:
    const uint8_t *buffer   = nullptr;
    int            index    = 0;
    int            bitCount = 0;

public:
    GetBitContext( const uint8_t *buffer, int bytes );
    ~GetBitContext() = default;
    uint32_t get_bit1();
    uint32_t get_bits( int n );
    uint32_t get_ue_golomb();
    int32_t  get_se_golomb();
    void     skip_bits( int n );
};

}; // namespace nx
