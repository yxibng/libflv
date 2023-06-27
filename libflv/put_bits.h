#ifndef __PUT_BITS_H__
#define __PUT_BITS_H__
#include <cstdint>

namespace nx {

struct PutBitsContext {

private:
    struct
    {
        uint8_t *buffer   = nullptr;
        int      capacity = 0; // bytes
    };

    struct
    {
        // current write byte index
        int index = 0;
        // current byte, number of bits left
        int bit_left = 0;
    };

public:
    PutBitsContext( uint8_t *buffer, int bytes );
    ~PutBitsContext() = default;
    /**
     * @brief put n bits from value in bits context
     *
     * @param n number of bits to write
     * @param value  uint32_t value contains the source bits
     */
    void put_bits( int n, uint32_t value );
};

};     // namespace nx

#endif // __PUT_BITS_H__
