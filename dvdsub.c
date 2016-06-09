#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#define BUFFER_LENGTH 64

void printbuffer(unsigned char *buffer, int len)
{
	int i;
	for( i = 0; i < len; ++i)
		printf("%02x ", buffer[i]);
	printf("\n");
}

void read_buff(unsigned char *buffer, FILE *vidfile, int bytes, int *buffpos )
{
	size_t readlen;
	if(bytes > BUFFER_LENGTH)
	{
		printf("ERROR buffer too short\n");
		return;
	}

	readlen = fread(buffer, sizeof(char), bytes, vidfile);
	if(readlen != bytes)
	{
		printf("ERROR File not read properly!\n");
		exit(1);
	}
	*buffpos = bytes;
}

void read_buff_append(unsigned char *buffer, FILE *vidfile, int bytes, int *buffpos )
{
	if(bytes + *buffpos > BUFFER_LENGTH)
	{
		printf("ERROR buffer too short\n");
		return;	
	}

	unsigned char tempbuff[BUFFER_LENGTH - *buffpos + 1];
	size_t readlen;
	int i = 0;

	readlen = fread(tempbuff, sizeof(char), bytes, vidfile);
	if(readlen != bytes)
	{
		printf("ERROR File not read properly!\n");
		exit(1);
	}

	while(i < bytes)
	{
		buffer[*buffpos] = tempbuff[i];
		*buffpos += 1;
		i++;
	}
}

void skipbytes(FILE *vidfile, int bytes)
{
	fseek(vidfile, bytes, SEEK_CUR);
}

void readdata(FILE *vidfile, unsigned char *buffer)
{
	int i;
	uint16_t packetlen;
	int buffpos = 0; //first empty position in buffer

	for(i = 0; i < 100000; ++i) //arbitrarily chosen value 
	{
		read_buff(buffer, vidfile, 4, &buffpos);

		if(buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01 && buffer[3] == 0xba)
		{
			printf("PACK Header\n");
			continue;
		}
		//PES Header
		else if(buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01)
		{
			// printf("PES Header\n");
			/*
			0xbd - Private stream 1 (non MPEG audio, subpictures)
			0xbe - Padding stream
			0xbf - Private stream 2 (navigation data)
			0xc0 - 0xdf - MPEG-1 or MPEG-2 audio stream
			0xe0 - 0xef - MPEG-1 or MPEG-2 video stream
 			*/
			if(buffer[3] == 0xbd)
			{
				printf("Private stream 1\n");
				read_buff_append(buffer, vidfile, 2, &buffpos);
				packetlen = (buffer[4] << 8) | buffer[5];
				printf("Packet length: %" PRIu16 "\n", packetlen);
				skipbytes(vidfile, 2);
				continue;
			}
			else if(buffer[3] == 0xbe)
			{
				printf("Padding stream\n");
				continue;
			}
			else if(buffer[3] == 0xbf)
			{
				printf("Private stream 2\n");
				continue;
			}
			else if(buffer[3] >= 0xc0 && buffer[3] <= 0xdf)
			{
				printf("Audio stream\n");
				continue;
			}
			else if(buffer[3] >= 0xe0 && buffer[3] <=0xef)
			{
				printf("Video Stream\n");
				continue;
			}
			else
			{
				printf("Unknown stream id :%02x\n", buffer[3]);
				continue;
			}
		} 

		fseek(vidfile, -3, SEEK_CUR);
	}
}

int main(int argc, char* argv[])
{
	FILE *vidfile;
	unsigned char buffer[10];
	int i;

	vidfile = fopen(argv[1], "r");
	if(!vidfile)
	{
		printf("ERROR opening file\n");
		return 0;
	}
	readdata(vidfile, buffer);

	fclose (vidfile);
	return 0;
}