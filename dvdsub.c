#include <stdio.h>
#include <stdint.h>

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
	if(bytes > BUFFER_LENGTH)
	{
		printf("ERROR buffer too short\n");
		return;
	}
	fread(buffer, bytes, 1, vidfile);
	*buffpos = bytes;
}

void readdata(FILE *vidfile, unsigned char *buffer)
{
	int i;
	uint16_t packetlen;
	int buffpos = 0;

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
	printbuffer(buffer, 4);


	fclose (vidfile);
	return 0;
}