#include "put_bits.h"
#include <cassert>
#include <cmath>

namespace nx {
PutBitsContext::PutBitsContext( uint8_t *buffer, int bytes ) {
    this->buffer   = buffer;
    this->capacity = bytes;
    this->bit_left = 8;
}

void PutBitsContext::put_bits( int n, uint32_t value ) {
    while ( n > 0 ) {
        if ( bit_left == 0 ) {
            this->index++;
            this->bit_left = 8;
        }
        assert( this->index < this->capacity );
        if ( this->index >= this->capacity ) return;
        // 获取当前写指针
        uint8_t *ptr = buffer + index;
        // 当前指针对应的 value
        uint8_t old_val = *ptr;

        if ( bit_left >= n ) {
            // 如果写入的 bits 小于当前字节剩余的 bits
            // 直接写入，更新 bit_left
            uint8_t bit_mask = pow( 2, n ) - 1;
            *ptr             = old_val << n | ( value & bit_mask );
            bit_left -= n;
            n = 0;
        }
        else {
            // 如果写入的 bits 大于当前字节剩余的 bits
            // 1. 先写入 bit_left 位
            // 2. 更新 bit_left
            // 3. 更新 n
            uint8_t bit_mask     = pow( 2, bit_left ) - 1;
            uint8_t val_to_write = value >> ( n - bit_left ) & bit_mask;
            *ptr                 = ( old_val << bit_left ) | val_to_write;
            n -= bit_left;
            bit_left = 0;
        }
    }
}

} // namespace nx
