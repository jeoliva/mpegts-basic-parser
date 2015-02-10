#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "bitreader.h"
#include "tsparser.h"

int openStream(char *url);
int readData(int fd, uint8_t *buffer, int buffer_size);
int closeStream(int fd);


int main(int argc, char *argv[])
{
	int fd, bytes_read;
	uint8_t packet_buffer[TS_PACKET_SIZE];
	int n_packets = 0;
	
	TSParser tsParser;
	memset(&tsParser, 0, sizeof(TSParser));
	
	fd = openStream(argv[1]);
	if(fd<0)
	{
		printf("Error opening the stream\nSyntax: tsunpacket FileToParse.ts\n");
		return -1;
	}
	
	while(1)
	{
		bytes_read = readData(fd, packet_buffer, TS_PACKET_SIZE);
			
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
	
	printf("Number of packets: %d\n", n_packets);
	printf("Finishing application\n");
	close(fd);
	
	freeParserResources(&tsParser);
	
	return 0;
}





int openStream(char *url)
{
	int fd = open(url, O_RDONLY);
	return fd;
}

int closeStream(int fd)
{
	return close(fd);
}

int readData(int fd, uint8_t *buffer, int buffer_size)
{
	int res = 0;
	
	res = read(fd, buffer, buffer_size);
	
	return res;
}