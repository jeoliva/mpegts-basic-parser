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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "bitreader.h"
#include "tsparser.h"

// Parse a TS packet
void parseTSPacket(TSParser *parser, ABitReader *bitReader)
{
	uint32_t sync_byte;
	uint32_t transport_error_indicator;
	uint32_t payload_unit_start_indicator;
	uint32_t transport_priority;
	uint32_t pid;
	uint32_t transport_scrambling_control;
	uint32_t adaptation_field_control;
	uint32_t continuity_counter;

	sync_byte = getBits(bitReader, 8);

	transport_error_indicator = getBits(bitReader, 1);
	if (transport_error_indicator != 0)
	{
		printf("Packet with Error. Transport Error indicator: %u\n", transport_error_indicator);
	}

	payload_unit_start_indicator = getBits(bitReader, 1);
	printf("Payload unit start indicator: %u\n", payload_unit_start_indicator);

	transport_priority = getBits(bitReader, 1);
	printf("Transport Priority: %u\n", transport_priority);

	pid = getBits(bitReader, 13);
	printf("PID: 0x%04x\n", pid);

	transport_scrambling_control = getBits(bitReader, 2);
	printf("Transport Scrambling Control: %u\n", transport_scrambling_control);

	adaptation_field_control = getBits(bitReader, 2);
	printf("Adaptation field control: %u\n", adaptation_field_control);

	continuity_counter = getBits(bitReader, 4);
	printf("Continuity Counter: %u\n", continuity_counter);

	if (adaptation_field_control == 2 || adaptation_field_control == 3)
	{
		parseAdaptationField(parser, bitReader);
	}

	if (adaptation_field_control == 1 || adaptation_field_control == 3)
	{
		parseProgramId(parser, bitReader, pid, payload_unit_start_indicator);
	}
}

// Parse adaptation field
void parseAdaptationField(TSParser *parser, ABitReader *bitReader)
{
	uint32_t adaptation_field_length = getBits(bitReader, 8);
	printf("Adaptation field length: %u\n", adaptation_field_length);
	if (adaptation_field_length > 0)
	{
		skipBits(bitReader, adaptation_field_length * 8);
	}
}

// Parse program Id
void parseProgramId(TSParser *parser, ABitReader *bitReader, uint32_t pid, uint32_t payload_unit_start_indicator)
{
	int i, handled = 0;
	TSPointersListItem *listItem;
	TSProgram *pProgram;

	if (pid == 0)
	{
		if (payload_unit_start_indicator)
		{
			uint32_t skip = getBits(bitReader, 8);
			skipBits(bitReader, skip * 8);
		}

		parseProgramAssociationTable(parser, bitReader);
		return;
	}

	for (listItem = parser->mPrograms.mHead; listItem != NULL; listItem = listItem->mNext)
	{
		pProgram = (TSProgram *)listItem->mData;
		if (pid == pProgram->mProgramMapPID)
		{
			if (payload_unit_start_indicator)
			{
				uint32_t skip = getBits(bitReader, 8);
				skipBits(bitReader, skip * 8);
			}

			parseProgramMap(parser, pProgram, bitReader);
			handled = 1;
			break;
		}
		else
		{
			TSStream *pStream = getStreamByPID(pProgram, pid);
			if (pStream != NULL)
			{
				parseStream(pStream, payload_unit_start_indicator, bitReader);

				handled = 1;
				break;
			}
		}
	}

	if (!handled)
	{
		printf("PID 0x%04x not handled.\n", pid);
	}
}

// Parse Program Association table
void parseProgramAssociationTable(TSParser *parser, ABitReader *bitReader)
{
	size_t i;
	uint32_t table_id = getBits(bitReader, 8);
	printf("  table_id = %u\n", table_id);

	uint32_t section_syntax_indicator = getBits(bitReader, 1);
	printf("  section_syntax_indicator = %u\n", section_syntax_indicator);

	getBits(bitReader, 1);
	printf("  reserved = %u\n", getBits(bitReader, 2));

	uint32_t section_length = getBits(bitReader, 12);
	printf("  section_length = %u\n", section_length);

	printf("  transport_stream_id = %u\n", getBits(bitReader, 16));
	printf("  reserved = %u\n", getBits(bitReader, 2));
	printf("  version_number = %u\n", getBits(bitReader, 5));
	printf("  current_next_indicator = %u\n", getBits(bitReader, 1));
	printf("  section_number = %u\n", getBits(bitReader, 8));
	printf("  last_section_number = %u\n", getBits(bitReader, 8));

	size_t numProgramBytes = (section_length - 5 /* header */ - 4 /* crc */);
	printf("  numProgramBytes = %ld\n", numProgramBytes);

	for (i = 0; i < numProgramBytes / 4; ++i)
	{
		uint32_t program_number = getBits(bitReader, 16);
		printf("    program_number = %u\n", program_number);

		printf("    reserved = %u\n", getBits(bitReader, 3));

		if (program_number == 0)
		{
			printf("    network_PID = 0x%04x\n", getBits(bitReader, 13));
		}
		else
		{
			unsigned programMapPID = getBits(bitReader, 13);

			printf("    program_map_PID = 0x%04x\n", programMapPID);
			addProgram(parser, programMapPID);
		}
	}
	printf("  CRC = 0x%08x\n", getBits(bitReader, 32));
}

// Parse Stream
void parseStream(TSStream *stream, uint32_t payload_unit_start_indicator, ABitReader *bitReader)
{
	size_t payloadSizeBits;

	if (payload_unit_start_indicator)
	{
		if (stream->mPayloadStarted)
		{
			flushStreamData(stream);
		}
		stream->mPayloadStarted = 1;
	}

	if (!stream->mPayloadStarted)
	{
		return;
	}

	payloadSizeBits = numBitsLeft(bitReader);

	memcpy(stream->mBuffer + stream->mBufferSize, getBitReaderData(bitReader), payloadSizeBits / 8);
	stream->mBufferSize += (payloadSizeBits / 8);
}

// Flush stream data
void flushStreamData(TSStream *stream)
{
	ABitReader bitReader;
	initABitReader(&bitReader, (uint8_t *)stream->mBuffer, stream->mBufferSize);

	parsePES(stream, &bitReader);

	stream->mBufferSize = 0;
}

// Parse a PES packet
void parsePES(TSStream *stream, ABitReader *bitReader)
{
	uint32_t packet_startcode_prefix = getBits(bitReader, 24);
	uint32_t stream_id = getBits(bitReader, 8);
	uint32_t PES_packet_length = getBits(bitReader, 16);

	if (stream_id != 0xbc	  // program_stream_map
		&& stream_id != 0xbe  // padding_stream
		&& stream_id != 0xbf  // private_stream_2
		&& stream_id != 0xf0  // ECM
		&& stream_id != 0xf1  // EMM
		&& stream_id != 0xff  // program_stream_directory
		&& stream_id != 0xf2  // DSMCC
		&& stream_id != 0xf8) // H.222.1 type E
	{
		uint32_t PTS_DTS_flags;
		uint32_t ESCR_flag;
		uint32_t ES_rate_flag;
		uint32_t DSM_trick_mode_flag;
		uint32_t additional_copy_info_flag;
		uint32_t PES_header_data_length;
		uint32_t optional_bytes_remaining;
		uint64_t PTS = 0, DTS = 0;

		skipBits(bitReader, 8);

		PTS_DTS_flags = getBits(bitReader, 2);
		ESCR_flag = getBits(bitReader, 1);
		ES_rate_flag = getBits(bitReader, 1);
		DSM_trick_mode_flag = getBits(bitReader, 1);
		additional_copy_info_flag = getBits(bitReader, 1);

		skipBits(bitReader, 2);

		PES_header_data_length = getBits(bitReader, 8);
		optional_bytes_remaining = PES_header_data_length;

		if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3)
		{
			skipBits(bitReader, 4);
			PTS = parseTSTimestamp(bitReader);
			skipBits(bitReader, 1);

			optional_bytes_remaining -= 5;

			if (PTS_DTS_flags == 3)
			{
				skipBits(bitReader, 4);

				DTS = parseTSTimestamp(bitReader);
				skipBits(bitReader, 1);

				optional_bytes_remaining -= 5;
			}
		}

		if (ESCR_flag)
		{
			skipBits(bitReader, 2);

			uint64_t ESCR = parseTSTimestamp(bitReader);

			skipBits(bitReader, 11);

			optional_bytes_remaining -= 6;
		}

		if (ES_rate_flag)
		{
			skipBits(bitReader, 24);
			optional_bytes_remaining -= 3;
		}

		skipBits(bitReader, optional_bytes_remaining * 8);

		// ES data follows.
		if (PES_packet_length != 0)
		{
			uint32_t dataLength = PES_packet_length - 3 - PES_header_data_length;

			// Signaling we have payload data
			onPayloadData(stream, PTS_DTS_flags, PTS, DTS, getBitReaderData(bitReader), dataLength);

			skipBits(bitReader, dataLength * 8);
		}
		else
		{
			size_t payloadSizeBits;
			// Signaling we have payload data
			onPayloadData(stream, PTS_DTS_flags, PTS, DTS, getBitReaderData(bitReader), numBitsLeft(bitReader) / 8);

			payloadSizeBits = numBitsLeft(bitReader);
		}
	}
	else if (stream_id == 0xbe)
	{ // padding_stream
		skipBits(bitReader, PES_packet_length * 8);
	}
	else
	{
		skipBits(bitReader, PES_packet_length * 8);
	}
}

int64_t parseTSTimestamp(ABitReader *bitReader)
{
	int64_t result = ((uint64_t)getBits(bitReader, 3)) << 30;
	skipBits(bitReader, 1);
	result |= ((uint64_t)getBits(bitReader, 15)) << 15;
	skipBits(bitReader, 1);
	result |= getBits(bitReader, 15);

	return result;
}

// convert PTS to timestamp
int64_t convertPTSToTimestamp(TSStream *stream, uint64_t PTS)
{
	if (!stream->mProgram->mFirstPTSValid)
	{
		stream->mProgram->mFirstPTSValid = 1;
		stream->mProgram->mFirstPTS = PTS;
		PTS = 0;
	}
	else if (PTS < stream->mProgram->mFirstPTS)
	{
		PTS = 0;
	}
	else
	{
		PTS -= stream->mProgram->mFirstPTS;
	}

	return (PTS * 100) / 9;
}

// Function called when we have payload content
void onPayloadData(TSStream *stream, uint32_t PTS_DTS_flag, uint64_t PTS, uint64_t DTS, uint8_t *data, size_t size)
{
	int64_t timeUs = convertPTSToTimestamp(stream, PTS);
	if (stream->mStreamType == TS_STREAM_VIDEO)
	{
		printf("Payload Data!!!! Video (%02x), PTS: %lld, DTS:%lld, Size: %ld\n", stream->mStreamType, PTS, DTS, size);
	}
	else if (stream->mStreamType == TS_STREAM_AUDIO)
	{
		printf("Payload Data!!!! Audio (%02x), PTS: %lld, DTS:%lld, Size: %ld\n", stream->mStreamType, PTS, DTS, size);
	}
}

// Add a new program to the list of programs
void addProgram(TSParser *parser, uint32_t programMapPID)
{
	TSProgram *program = (TSProgram *)malloc(sizeof(TSProgram));
	memset(program, 0, sizeof(TSProgram));

	program->mProgramMapPID = programMapPID;

	addItemToList(&parser->mPrograms, program);
}

// Add a new stream to the specified program
void addStream(TSProgram *program, uint32_t elementaryPID, uint32_t streamType)
{
	TSStream *stream = (TSStream *)malloc(sizeof(TSStream));
	memset(stream, 0, sizeof(TSStream));

	stream->mProgram = program;
	stream->mElementaryPID = elementaryPID;
	stream->mStreamType = streamType;

	printf("Add Stream. PID: %d, Stream Type: %d (%X)\n", elementaryPID, streamType, streamType);

	addItemToList(&program->mStreams, stream);
}

// Add an item to a linked list
void addItemToList(TSPointersList *list, void *data)
{
	TSPointersListItem *item = (TSPointersListItem *)malloc(sizeof(TSPointersListItem));
	item->mData = data;
	item->mNext = NULL;

	if (list->mTail != NULL)
	{
		list->mTail->mNext = item;
		list->mTail = item;
	}
	else
	{
		list->mHead = list->mTail = item;
	}
}

// Get Stream object given its id
TSStream *getStreamByPID(TSProgram *program, uint32_t pid)
{
	TSPointersListItem *listItem;
	TSStream *stream;
	for (listItem = program->mStreams.mHead; listItem != NULL; listItem = listItem->mNext)
	{
		stream = (TSStream *)listItem->mData;
		if (stream != NULL && stream->mElementaryPID == pid)
		{
			return stream;
		}
	}
	return NULL;
}

// Get a Program object given its id
TSProgram *getProgramByPID(TSParser *parser, uint32_t pid)
{
	TSPointersListItem *listItem;
	TSProgram *program;
	for (listItem = parser->mPrograms.mHead; listItem != NULL; listItem = listItem->mNext)
	{
		program = (TSProgram *)listItem->mData;
		if (program != NULL && program->mProgramMapPID == pid)
		{
			return program;
		}
	}
	return NULL;
}

// Parse program map
void parseProgramMap(TSParser *parser, TSProgram *program, ABitReader *bitReader)
{
	uint32_t section_syntax_indicator;
	uint32_t table_id;
	uint32_t section_length;
	uint32_t program_info_length;
	size_t infoBytesRemaining;
	uint32_t streamType;
	uint32_t elementaryPID;
	uint32_t ES_info_length;
	uint32_t info_bytes_remaining;

	table_id = getBits(bitReader, 8);

	printf("****** PROGRAM MAP *****\n");
	printf("	table_id = %u\n", table_id);

	section_syntax_indicator = getBits(bitReader, 1);
	printf("	section_syntax_indicator = %u\n", section_syntax_indicator);

	// Reserved
	skipBits(bitReader, 3);

	section_length = getBits(bitReader, 12);
	printf("  section_length = %u\n", section_length);
	printf("  program_number = %u\n", getBits(bitReader, 16));
	printf("  reserved = %u\n", getBits(bitReader, 2));
	printf("  version_number = %u\n", getBits(bitReader, 5));
	printf("  current_next_indicator = %u\n", getBits(bitReader, 1));
	printf("  section_number = %u\n", getBits(bitReader, 8));
	printf("  last_section_number = %u\n", getBits(bitReader, 8));
	printf("  reserved = %u\n", getBits(bitReader, 3));
	printf("  PCR_PID = 0x%04x\n", getBits(bitReader, 13));
	printf("  reserved = %u\n", getBits(bitReader, 4));

	program_info_length = getBits(bitReader, 12);
	printf("  program_info_length = %u\n", program_info_length);

	skipBits(bitReader, program_info_length * 8); // skip descriptors

	// infoBytesRemaining is the number of bytes that make up the
	// variable length section of ES_infos. It does not include the
	// final CRC.
	infoBytesRemaining = section_length - 9 - program_info_length - 4;

	while (infoBytesRemaining > 0)
	{
		streamType = getBits(bitReader, 8);
		printf("    stream_type = 0x%02x\n", streamType);

		printf("    reserved = %u\n", getBits(bitReader, 3));

		elementaryPID = getBits(bitReader, 13);
		printf("    elementary_PID = 0x%04x\n", elementaryPID);

		printf("    reserved = %u\n", getBits(bitReader, 4));

		ES_info_length = getBits(bitReader, 12);
		printf("    ES_info_length = %u\n", ES_info_length);

		info_bytes_remaining = ES_info_length;
		while (info_bytes_remaining >= 2)
		{
			uint32_t descLength;
			printf("      tag = 0x%02x\n", getBits(bitReader, 8));

			descLength = getBits(bitReader, 8);
			printf("      len = %u\n", descLength);

			skipBits(bitReader, descLength * 8);

			info_bytes_remaining -= descLength + 2;
		}

		if (getStreamByPID(program, elementaryPID) == NULL)
			addStream(program, elementaryPID, streamType);

		infoBytesRemaining -= 5 + ES_info_length;
	}

	printf("  CRC = 0x%08x\n", getBits(bitReader, 32));
	printf("****** PROGRAM MAP *****\n");
}

// Free program resources
void freeProgramResources(TSProgram *program)
{
	// Free Streams
	TSPointersListItem *item;
	TSStream *pStream;
	while (program->mStreams.mHead != NULL)
	{
		item = program->mStreams.mHead;
		program->mStreams.mHead = item->mNext;

		if (item->mData != NULL)
		{
			pStream = (TSStream *)item->mData;
			free(pStream);
		}

		free(item);
	}
}

// Free parser resources
void freeParserResources(TSParser *parser)
{
	// Free Programs
	TSPointersListItem *item;
	TSProgram *pProgram;
	while (parser->mPrograms.mHead != NULL)
	{
		item = parser->mPrograms.mHead;
		parser->mPrograms.mHead = item->mNext;

		if (item->mData != NULL)
		{
			pProgram = (TSProgram *)item->mData;
			freeProgramResources(pProgram);
			free(pProgram);
		}

		free(item);
	}
}

// Signal a discontinuity
void signalDiscontinuity(TSParser *parser, int isSeek)
{
	TSPointersListItem *item;
	TSProgram *pProgram;
	while (parser->mPrograms.mHead != NULL)
	{
		item = parser->mPrograms.mHead;
		parser->mPrograms.mHead = item->mNext;

		if (item->mData != NULL)
		{
			pProgram = (TSProgram *)item->mData;
			signalDiscontinuityToProgram(pProgram, isSeek);
		}
	}
}

// Signal a discontinuity to a program
void signalDiscontinuityToProgram(TSProgram *program, int isSeek)
{
	TSPointersListItem *item;
	TSStream *pStream;
	while (program->mStreams.mHead != NULL)
	{
		item = program->mStreams.mHead;
		program->mStreams.mHead = item->mNext;

		if (item->mData != NULL)
		{
			pStream = (TSStream *)item->mData;
			signalDiscontinuityToStream(pStream, isSeek);
		}
	}
}

// Signal a discontinuity to a stream
void signalDiscontinuityToStream(TSStream *stream, int isSeek)
{
	stream->mPayloadStarted = 0;
	stream->mBufferSize = 0;
}
