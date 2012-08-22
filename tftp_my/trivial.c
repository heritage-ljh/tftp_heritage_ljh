/*----------------- TFTP client -------------------*/

#incldue "common.h"

int showHelp()
{
	printf("/*******************************************************\n\
	\n\
			trivial -f file -H host [-p port] {-r|-w} -v\n\
	\n\
			Compulsory params:\n\
			-f:	filename\n\
			-H:	server host\n\
			[-r, -w]: reads/writes to the server\n\
	\n\
		Optional params:\n\
			-p:	server port. If not set, port will be 60\n\
			-v:	use verbose mode\n\
			-t:	maximum time in seconds for retransmission\n\
	\n\
		Help:\n\
			-h:	shows this info\n\
	\n\
	**************************************************************/");
	fflush( stdout);
	return 1;
}

void sendQuery( tftp_rwq_hdr query, int *socket_origin, struct sockaddr_in server)
{
	/*------------ Variables --------------*/
	socklen_t server_length = sizeof( struct sockaddr_in);
	int	query_size = 0;
	char buffer[516];

	bzero( buffer, 516);

	st_u_int16( query.opcode, buffer);
	query_size += (int)strlen( query.filename) + 1;

	strcpy( buffer + query_size, query.mode);
	query_size += (int)strlen( query.mode) + 1;

	sendto( *socket_origin, buffer, query_size, 0, (struct sockaddr*)&server, server_length);
	if( verbose == 1)
	{
		log_info( buffer, query_size, VERBOSE_SUBJECT_CLIENT,VERBOSE_ACTION_SEND);
	}
}

int main( int argc, char *argc[])
{
	int param	  = 0;
	int show_help = 0;
	int mode	  = 0;
	int server_port = RFC1350_PORT;

	/*------- Sockets ----------*/
	struct sockaddr_in server;
	struct sockaddr_in client;
	struct hostent *server_host;
	int	client_socket;

	/*-------- TFTP ------------*/
	int final = 0;
	tftp_rwq_hdr query;
	FILE *file;

	retransmission_time = 5; /* Default seconds value for retransmission*/ 

	while( (param = getopt( argc, argv, "hvrwt:f:H:p:")) != -1)
	{
		switch( (char)param)
		{
			case 'f':
				target_file = optarg;
				break;
			case 'H':
				server_host_name = optarg;
				break;
			case 'p':
				server_port = atoi(optarg);
				break;
			case 'r':
				/* User wants to read data from the server.Flags -r and -w cannot be set
				 * at the same time.*/
				if( mode != 0)
				{
					mode = -1;
				}
				else
				{
					mode = RFC1350_OP_RRQ;
				}
				break;
			case 'w':
				/* User wants to write data to the server. Flags -r and -w cannot be set
				 * at the same time.*/
				if( mode != 0)
				{
					mode = -1;
				}
				else
				{
					mode = RFC1350_OP_WRQ;
				}
				break;
			case 'v':
				/* User wants verbose mode, so the program
				 * must give feedback to the user*/
				verbose = 1;
			case 't':
				retransmission_time = atoi( optarg);
				break;
			case 'h':
				show_help = 1;
				break;
		}
	}

	/*--------------------- param control ------------------*/
	if( target_file == NULL)
	{
		printf( NO_FILE_SET_ERR);
		fflush( stdout);
		show_help = 1;
	}
	if( server_host_name == NULL)
	{
		printf( NO_HOST_SET_ERR);
		fflush( stdout);
		show_help = 1;
	}
	if( mode == 0)
	{
		printf( NO_MODE_SET_ERR);
		fflush( stdout);
		show_help = 1;
	}
	if( mode < 0)
	{
		printf( TWO_MODES_SET_ERR);
		fflush( stdout);
		show_help = 1;
	}
	if( show_help == 1)
	{
		showHelp();
		return -1;
	}

	/*------------------- Sockets initialization ----------------*/
	int		retval = 0;
	client.sin_family	   = AF_INET;
	client.sin_port		   = htons(0);
	client.sin_addr.s_addr = INADDR_ANY;

	client_socket = socket( AF_INET, SOCK_DGRAM, 0);
	if( client_socket == -1)
	{
		printf("%s:%d socket error, %s\n",
				__FILE__, __LINE__, strerror(errno));
		return -1;
	}

	retval = bind( client_socket, (struct sockaddr*)&client, sizeof(client));
	if( retval == -1)
	{
		printf("%s:%d bind error, %s\n",
				__FILE__, __LINE__, strerror(errno));
		return -1;
	}

	server.sin_family	   = AF_INET;
	server.sin_port		   = htons( server_port);
	server_host	= gethostbyname( server_host_name);
	server.sin_addr.s_addr = inet_addr( inet_ntoa(*((struct in_addr*)server_host->h_addr)));
	/*memcpy( (char *)&server.sin_addr, (char *)server_host->h_addr, server_host->h_length);*/

	/*------------------------- Set the packages and send them ----------------------*/
	strcpy( query.filename, target_file);
	strcpy( query.mode, "octet");

	/* Switch actions depending on the mode given by the user*/
	switch( mode)
	{
		case RFC1350_OP_RRQ:
			/* The user wants to read from the server */
			/* Client sends and ACK, server sends data*/
			file = fopen( target_file, "wb"); /* File could not be created, so exit the program*/ 
			if( file == NULL)
			{
				printf(COULD_NOT_CREATE_FILE_ERR);
				fflush( stdout);
				return -1;
			}

			query.opcode = RFC1350_OP_RRQ;
			sendQuery( query, &client_socket, server);
			final = recieveData( file, &server, &client_socket, VERBOSE_SUBJECT_CLIENT);
			if( final == 0)
			{
				printf( "File transfer was successful!\n");
				fflush( stdout);
			}
			break;
		case RFC1350_OP_WRQ:
			/* The user wants to write to the server */
			/* Client sends data, server sends and ACK */
			file = fopen( target_file, "rb");
			if( file == NULL)
			{
				printf( FILE_DOES_NOT_EXITST_ERR);
				fflush( stdout);
				return -1;
			}

			query.opcode = RFC1350_OP_WRQ;
			sendQuery( query, &client_socket, server);
			final = recieveACK( 0, &server, &client_socket, VERBOSE_SUBJECT_CLIENT);
			/*printf( "final = %d\n", final); //debug*/
			if( final == 1)
			{
				final = sendData( file, &server, &client_socket, VERBOSE_SUBJECT_CLIENT);
				if( final == 1)
				{
					printf( "File transfer was sucessful !!\n");
					fflush( stdout);
				}

			}
			break;
		
	}
	close( client_socket);
	

	return 0;

}
