#include "common.h"

void printError( char *buffer, int size)
{
	tftp_error_hdr	error_message;
	memcpy( &error_message, buffer, size);
	printf( "ERROR %d:\n%s\n", ntohs( error_message.message));

	return;
}


int recieveACK( unsigned short index, struct sockaddr_in *destination_addr, int *sock, char verbose_text[LOG_INFO_SUBJECT_SIZE])
{
	char buffer[516];
	int	recv_size = 0;
	socklen_t	destination_addr_len = sizeof( struct sockaddr_in);
	int	rexmt = retransmission_time;

	int result = 0;
	struct timeval	t_retx;
	fd_set	fds;

	FD_ZERO( &fds);
	FD_SET( *sock, &fds);
	t_retx.tv_sec = rexmt;
	t_retx.tv_userc = 0;

	result = select( (*sock) + 1, &fds, NULL, NULL, &t_retx);
	/*printf("recieveACK: RESULT from SELECT = %x\n", result); [> debug<] */

	if( resutl == 1)
	{
		recv_size = recvfrom( *sock, buffer, 516, 0, (struct sockaddr*)destination_addr, &destination_addr_len);
		if( verbose == 1)
		{
			log_info( &buffer, recv_size, verbose_text, VERBOSE_ACTION_RECIEVE);
		}
		if( OPCODE(buffer) == RFC1350_OP_ERROR)
		{
			printError( buffer, recv_size);
			return -1;
		}
		else if( OPCODE(buffer) == RFC1350_OP_ACK)
		{
			tftp_ack_hdr message_ack;
			memcpy( &message_ack, buffer, recv_size);
			if( ntohs( message_ack.num_block) == index)
			{
				/* Expected ACK recieved. Send next data package*/
				return 1;
			}
		}
	}

	return 0;
}


void sendDataPackage( char data[RFC1350_BLOCKSIZE], int size, unsigned short index, struct sockaddr_in destination_addr, int *sock, char verbose_text[LOG_INFO_SUBJECT_SIZE])
{
}

int sendData( FILE *file, struct sockaddr_in *reciever, int *sender_socket, char verbose_text[LOG_INFO_SUBJECT_SIZE])
{
	int bytes = RFC1350_BLOCKSIZE;
	char	data[RFC1350_BLOCKSIZE];
	int	next_package = 1;
	usingned short index = 0;
	int timeout = 5;

	/* While this package is not the last one */
	while( bytes == RFC1350_BLOCKSIZE)
	{
		if(timeout < 0)
		{
			return -1;
		}
		if( next_package == 1)
		{
			timeout = 5;
			bytes = fread( data, sizeof(char), RFC1350_BLOCKSIZE, file);
			index++;
			sendDataPackage( data, bytes, index, *reciever, sender_socket, verbose_text);
		}
		else if( next_package == 0)
		{
			sendDataPackage( data, bytes, index, *reciever, sender_socket, verbose_text);
			timeout -= 1;
		}
		else
		{
			return -1;
		}

		next_package = recieveACK( index, reciever, sender_socket, verbose_text);
	}

	return 0;
}


void sendDataPackage( char data[RFC1350_BLOCKSIZE], int size, unsigned short index, struct sockaddr_in destination_addr, int *sock, char verbose_text[LOG_INFO_SUBJECT_SIZE])
{
	tftp_data_hdr message;
	socklen_t destination_addr_len = sizeof( struct sockaddr_in);
	
	memcpy( message.data, data, size);
	message.opcode = htons(RFC1350_OP_DATA);
	size += sizeof( message.opcode);

	message.num_block = htons( index);
	size += sizeof( message.num_block);

	sendto( *sock, (char *)&message, size, 0, (struct sockaddr*)&destination_addr, destination_addr_len);
	if( verbose == 1)
	{
		log_info( &message, size, verbose_text, VERBOSE_ACTION_SEND);
	}

	return;

}




