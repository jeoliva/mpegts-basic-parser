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

#ifndef __TS_PARSER_H__
#define __TS_PARSER_H__


#define TS_PACKET_SIZE	  188
#define TS_SYNC			  0x47
#define TS_DISCONTINUITY  0x0

#define TS_STREAM_VIDEO	  0x1b
#define TS_STREAM_AUDIO   0x0f

// Linked lists structure
typedef struct TSPointersListItem
{
	void *mData;
	struct TSPointersListItem *mNext;
}TSPointersListItem;

typedef struct TSSPointersList
{
	TSPointersListItem *mHead;
	TSPointersListItem *mTail;
}TSPointersList;

// Programs (one TS file could have 1 or more programs)
typedef struct TSProgram
{
	uint32_t mProgramMapPID;
	uint64_t mFirstPTS;
	int mFirstPTSValid;

	TSPointersList mStreams;
}TSProgram;

// Streams (one program could have one or more streams)
typedef struct TSStream
{
	TSProgram *mProgram;

	uint32_t mElementaryPID;
	uint32_t mStreamType;
	uint32_t mPayloadStarted;

	char mBuffer[128*1024];
	int mBufferSize;
}TSStream;

// Parser. Keeps a reference to the list of programs
typedef struct TSParser
{
	TSPointersList mPrograms;
}TSParser;

void signalDiscontinuity(TSParser *parser, int isSeek);
void signalDiscontinuityToProgram(TSProgram *program, int isSeek);
void signalDiscontinuityToStream(TSStream *stream, int isSeek);

void parseTSPacket(TSParser *parser, ABitReader *bitReader);
void parseAdaptationField(TSParser *parser, ABitReader *bitReader);
void parseProgramId(TSParser *parser, ABitReader *bitReader, uint32_t pid, uint32_t payload_unit_start_indicator);
void parseProgramAssociationTable(TSParser *parser, ABitReader *bitReader);
void parseProgramMap(TSParser *parser, TSProgram *program, ABitReader *bitReader);
void parseStream(TSStream *stream, uint32_t payload_unit_start_indicator, ABitReader *bitReader);
void parsePES(TSStream *stream, ABitReader *bitReader);
int64_t parseTSTimestamp(ABitReader *bitReader);

void addProgram(TSParser *parser, uint32_t programMapPID);
void addStream(TSProgram *program, uint32_t elementaryPID, uint32_t streamType);

TSStream *getStreamByPID(TSProgram *program, uint32_t pid);
TSProgram *getProgramByPID(TSParser *parser, uint32_t pid);

void flushStreamData(TSStream *stream);
void onPayloadData(TSStream *stream, uint32_t PTS_DTS_flag, uint64_t PTS, uint64_t DTS, uint8_t *data, size_t size);

void freeParserResources(TSParser *parser);

void addItemToList(TSPointersList *list, void *data);

#endif
