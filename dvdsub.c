#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#define BUFFER_LENGTH 50000
#define SPU_BUFFER_LENGTH 50000

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
	int check = fseek(vidfile, bytes, SEEK_CUR);
	if( check != 0)
		printf("ERROR failed skipbytes\n");
}

void process_spu(FILE *vidfile)
{
	uint16_t size_spu, size_data;
	unsigned char spu_buffer[SPU_BUFFER_LENGTH];
	unsigned char *spu_data;
	unsigned char spu_ctrlbuff[SPU_BUFFER_LENGTH];
	int spu_buffpos = 0, spu_datapos = 0, spu_ctrlpos = 0;
	int data_start, control_start;

	//First 2 bytes - size of total subpicture packet
	read_buff(spu_buffer, vidfile, 2, &spu_buffpos);
	size_spu = (spu_buffer[0] << 8) | spu_buffer[1];
	printf("SPU size: %" PRIu16 " \n", size_spu);
	if(!(size_spu > 0))
	{
		printf("ERROR spu size incorrect\n");
		return;
	}

	//Next 2 bytes - Size of data packet (offset to control sequence)
	read_buff_append(spu_buffer, vidfile, 2, &spu_buffpos);
	size_data = (spu_buffer[2] << 8) | spu_buffer[3];
	printf("SPU data size: %" PRIu16 " \n", size_data);

	if(!(size_data >= 0))
	{
		printf("ERROR spu data size incorrect\n");
		return;
	}

	//read packet data to spu_data
	data_start = ftell(vidfile);
	spu_data = (unsigned char *) malloc( size_data ); 
	read_buff(spu_data, vidfile, size_data - 4, &spu_datapos);
	printbuffer(spu_data, spu_datapos);
	
	//process control packet
	uint16_t date, offset, prevoff = -1, parasize;
	uint8_t command;
	uint8_t color[4];
	uint8_t contrast[4];//alpha channel
	uint16_t coord[4];
	uint16_t pixoffset[2];
	int ctrlpacket_end = 0, ctrlseq_end;

	control_start = ftell(vidfile);
	while( ctrlpacket_end == 0 )
	{
		read_buff(spu_ctrlbuff, vidfile, 4, &spu_ctrlpos);
		date = (spu_ctrlbuff[0] << 8) | spu_ctrlbuff[1]; 
		offset = (spu_ctrlbuff[2] << 8) | spu_ctrlbuff[3];
		if( offset == prevoff) // if same offset as previous -> last control sequence
			ctrlpacket_end = 1;
		ctrlseq_end = 0;
		while( ctrlseq_end == 0)
		{
			read_buff_append(spu_ctrlbuff, vidfile, 1, &spu_ctrlpos);
			command = spu_ctrlbuff[spu_ctrlpos - 1];
			switch(command)
			{
				case 0x01:	//Display sequence
				case 0x02:	//Stop Display - Use not clear
							break;
				case 0x03:	//SET_COLOR
							read_buff_append(spu_ctrlbuff, vidfile, 2, &spu_ctrlpos);
							color[0] = (spu_ctrlbuff[spu_ctrlpos - 2] & 0xf0) >> 4;
							color[1] = spu_ctrlbuff[spu_ctrlpos - 2] & 0x0f;
							color[2] = (spu_ctrlbuff[spu_ctrlpos - 1] & 0xf0) >> 4;
							color[3] = spu_ctrlbuff[spu_ctrlpos - 1] & 0x0f;
							break;
				case 0x04:	//SET_CONTR
							read_buff_append(spu_ctrlbuff, vidfile, 2, &spu_ctrlpos);
							contrast[0] = (spu_ctrlbuff[spu_ctrlpos - 2] & 0xf0) >> 4;
							contrast[1] = spu_ctrlbuff[spu_ctrlpos - 2] & 0x0f;
							contrast[2] = (spu_ctrlbuff[spu_ctrlpos - 1] & 0xf0) >> 4;
							contrast[3] = spu_ctrlbuff[spu_ctrlpos - 1] & 0x0f;
							break;
				case 0x05:	//SET_DAREA
							read_buff_append(spu_ctrlbuff, vidfile, 6, &spu_ctrlpos);
							coord[0] = ((spu_ctrlbuff[spu_ctrlpos - 6] << 8) | (spu_ctrlbuff[spu_ctrlpos - 5] & 0xf0)) >> 4 ; //starting x coordinate
							coord[1] = ((spu_ctrlbuff[spu_ctrlpos - 5] & 0x0f) << 8 ) | spu_ctrlbuff[spu_ctrlpos - 4] ; //ending x coordinate
							coord[2] = ((spu_ctrlbuff[spu_ctrlpos - 3] << 8) | (spu_ctrlbuff[spu_ctrlpos - 2] & 0xf0)) >> 4 ; //starting y coordinate
							coord[3] = ((spu_ctrlbuff[spu_ctrlpos - 2] & 0x0f) << 8 ) | spu_ctrlbuff[spu_ctrlpos - 1] ; //ending y coordinate
							//(x1-x2+1)*(y1-y2+1)
							break;
				case 0x06:	//SET_DSPXA - Pixel address
							read_buff_append(spu_ctrlbuff, vidfile, 4, &spu_ctrlpos);
							pixoffset[0] = (spu_ctrlbuff[spu_ctrlpos - 4] << 8) | spu_ctrlbuff[spu_ctrlpos -3];
							pixoffset[1] = (spu_ctrlbuff[spu_ctrlpos - 2] << 8) | spu_ctrlbuff[spu_ctrlpos -1];
							break;
				case 0x07:	//SET_COLCON
							printf("Command 07 not implemented\n");
							read_buff_append(spu_ctrlbuff, vidfile, 2, &spu_ctrlpos);
							parasize = (spu_ctrlbuff[spu_ctrlpos -2 ] << 8) | spu_ctrlbuff[spu_ctrlpos -1];
							skipbytes(vidfile, parasize);
							break;
				case 0xff:	ctrlseq_end = 1;
							break;
				default	 : 	printf("Unknown command in spu control packet\n");	 
			}
		}
	}
	printbuffer(spu_ctrlbuff, spu_ctrlpos);

	free(spu_data);
}

void readdata(FILE *vidfile, unsigned char *buffer)
{
	int i;
	uint16_t packetlen;
	uint8_t headerlen, substream_id;
	int buffpos = 0; //first empty position in buffer

	for(i = 0; i < 100000000; ++i) //arbitrarily chosen value 
	{
		read_buff(buffer, vidfile, 4, &buffpos);

		if(buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01 && buffer[3] == 0xba)
		{
			// printf("PACK Header\n");
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
				printf("Private stream 1");
				read_buff_append(buffer, vidfile, 5, &buffpos);
				packetlen = (buffer[4] << 8) | buffer[5];
				printf(" Packet length: %" PRIu16 "\n", packetlen);
				// skipbytes(vidfile, 2);
				// read_buff_append(buffer, vidfile, 3, &buffpos);
				
				headerlen = buffer[8];
				printf("Header length:%" PRIu8 "\n", headerlen);
				
				skipbytes(vidfile, headerlen);
				read_buff_append(buffer, vidfile, 1, &buffpos);
				substream_id = buffer[9] & 0xff;
				printf("substream_id: %02x \n", substream_id);
				// printbuffer(buffer, buffpos);

				//32 possible subtitle streams
				if( substream_id >= 0x20 && substream_id < 0x40)
				{
					printf("Subpictures found!\n");
					process_spu(vidfile);
				}
				
				continue;
			}
			else if(buffer[3] == 0xbe)
			{
				// printf("Padding stream\n");
				continue;
			}
			else if(buffer[3] == 0xbf)
			{
				// printf("Private stream 2\n");
				continue;
			}
			else if(buffer[3] >= 0xc0 && buffer[3] <= 0xdf)
			{
				// printf("Audio stream\n");
				continue;
			}
			else if(buffer[3] >= 0xe0 && buffer[3] <=0xef)
			{
				// printf("Video Stream\n");
				continue;
			}
			else
			{
				// printf("Unknown stream id :%02x\n", buffer[3]);
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