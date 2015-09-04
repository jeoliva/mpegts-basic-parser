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
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "bitreader.h"
#include "tsparser.h"


// Main method
int main(int argc, char *argv[])
{
	int fd, bytes_read;
	uint8_t packet_buffer[TS_PACKET_SIZE];
	int n_packets = 0;

	TSParser tsParser;
	memset(&tsParser, 0, sizeof(TSParser));

	fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		printf("Error opening the stream\nSyntax: tsunpacket FileToParse.ts\n");
		return -1;
	}

	// Parse file while we can read full TS packets
	while(1)
	{
		bytes_read = read(fd, packet_buffer, TS_PACKET_SIZE);

		if(packet_buffer[0] == TS_DISCONTINUITY)
		{
			printf("Discontinuity detected!\n");
			signalDiscontinuity(&tsParser, 0);
		}
		else if(bytes_read < TS_PACKET_SIZE)
		{
			printf("End of file!\n");
			break;
		}
		else if(packet_buffer[0] == TS_SYNC)
		{
			ABitReader bitReader;
			initABitReader(&bitReader, packet_buffer, bytes_read);

			parseTSPacket(&tsParser, &bitReader);

			n_packets++;
		}
	}

	printf("Number of packets found: %d\n", n_packets);

	// Freeing resources
	close(fd);
	freeParserResources(&tsParser);

	return 0;
}
