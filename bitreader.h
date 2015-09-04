/*
 * MpegtTS Basic Parser
 * Copyright (c) jeoliva, All rights reserved.

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 *License along with this library.
 */

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
