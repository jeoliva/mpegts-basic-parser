#ifndef __BIT_READER_H__
#define __BIT_READER_H__

#include <sys/types.h>
#include <stdint.h>




typedef struct ABitReader 
{
    uint8_t *mData;
    size_t mSize;

    uint32_t mReservoir;  // left-aligned bits
    size_t mNumBitsLeft;
}ABitReader;


void initABitReader(ABitReader *bitReader, uint8_t *data, size_t size);
uint32_t getBits(ABitReader *bitReader, size_t n);
void skipBits(ABitReader *bitReader, size_t n);
size_t numBitsLeft(ABitReader *bitReader);
uint8_t *getBitReaderData(ABitReader *bitReader);


#endif  // A_BIT_READER_H_

