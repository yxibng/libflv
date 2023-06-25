#include "get_bits.h"

namespace nx {

GetBitContext::GetBitContext(const uint8_t *buffer, int bytes) {
    this->buffer = buffer;
    this->bitCount = bytes * 8;
    this->index = 0;
}

uint32_t GetBitContext::get_bit1() {
    assert(this->index < this->bitCount);
    int byteIndex = this->index >> 3;
    uint8_t tmp = *(buffer + byteIndex);
    int bitIndex = this->index % 8;
    tmp <<= bitIndex;
    tmp >>= 8 - 1;
    this->index++;
    return tmp;
}
uint32_t GetBitContext::get_bits(int n) {
    if (n == 0) return 0;
    assert(n >= 1 && n <= 32);
    uint32_t ret = 0;
    for (int i = n - 1; i >= 0; i--) {
        ret |= (get_bit1() << i);
    }
    return ret;
}
uint32_t GetBitContext::get_ue_golomb() {
    int count = 0;
    while (get_bit1() == 0 && count < 32) {
        count++;
    }
    uint32_t ret = get_bits(count) + (1 << count) - 1;
    return ret;
}

int32_t GetBitContext::get_se_golomb() {
    int r = get_ue_golomb();
    if (r & 0x01) {
        r = (r + 1) / 2;
    } else {
        r = -(r / 2);
    }
    return r;
}

void GetBitContext::skip_bits(int n) {
    assert(n > 0 && n < 32);
    if (this->index + n < bitCount) {
        this->index += n;
    }
}

}; // namespace nx
