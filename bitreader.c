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

#include "bitreader.h"

void fillReservoir(ABitReader *bitReader)
{
	bitReader->mReservoir = 0;
  size_t i;
  for (i = 0; bitReader->mSize > 0 && i < 4; ++i)
	{
    bitReader->mReservoir = (bitReader->mReservoir << 8) | *(bitReader->mData);

    ++bitReader->mData;
    --bitReader->mSize;
  }

  bitReader->mNumBitsLeft = 8 * i;
  bitReader->mReservoir <<= 32 - bitReader->mNumBitsLeft;
}

void putBits(ABitReader *bitReader, uint32_t x, size_t n)
{
	bitReader->mReservoir = (bitReader->mReservoir >> n) | (x << (32 - n));
  bitReader->mNumBitsLeft += n;

}

void initABitReader(ABitReader *bitReader, uint8_t *data, size_t size)
{
	bitReader->mData = data;
	bitReader->mSize = size;
	bitReader->mReservoir = 0;
	bitReader->mNumBitsLeft = 0;
}

uint32_t getBits(ABitReader *bitReader, size_t n)
{
	uint32_t result = 0;
  while (n > 0)
	{
    if (bitReader->mNumBitsLeft == 0)
		{
      fillReservoir(bitReader);
    }

    size_t m = n;
    if (m > bitReader->mNumBitsLeft)
		{
      m = bitReader->mNumBitsLeft;
    }

    result = (result << m) | (bitReader->mReservoir >> (32 - m));
    bitReader->mReservoir <<= m;
    bitReader->mNumBitsLeft -= m;
    n -= m;
  }

  return result;
}

void skipBits(ABitReader *bitReader, size_t n)
{
	while (n > 32)
	{
    getBits(bitReader, 32);
    n -= 32;
  }

  if (n > 0)
	{
    getBits(bitReader, n);
  }
}

size_t numBitsLeft(ABitReader *bitReader)
{
	return bitReader->mSize * 8 + bitReader->mNumBitsLeft;
}

uint8_t *getBitReaderData(ABitReader *bitReader)
{
	return bitReader->mData - bitReader->mNumBitsLeft / 8;
}
