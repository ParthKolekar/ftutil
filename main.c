#include "main.h"

struct linux_dirent {
	long           d_ino;
	off_t          d_off;
	unsigned short d_reclen;
	char           d_name[];
};

char * server_receive_buffer = NULL;
char * server_send_buffer = NULL;

char * client_receive_buffer = NULL;
char * client_send_buffer = NULL;

bool is_server = false;
bool is_client = false;

u_short server_port = 1143;
u_short client_port = 0; // 0 -> random port
char * server_hostname = NULL;

char * connection_type = NULL;

FILE * stdin_file_descriptor = NULL;
FILE * stdout_file_descriptor = NULL;
FILE * stderr_file_descriptor = NULL;

#ifdef COLOR
bool colorize = true;
#else
bool colorize = false;
#endif

void print_int(long long int n)		// Convert 'n' to string and print it
{
	static int zero=1;
	if(n==0 && zero)		// i.e, No. to be printed is actually zero and not the zero that terminates the recursion
	{
		write(1,"0",1);
		return;
	}
	char ch = '0';
	if(n)
	{
		zero = 0;
		print_int(n/10);
		ch = n%10 + '0';
		write(1,&ch,sizeof(ch));
	}
}

bool set_socket_blocking_enabled(int fd, bool blocking)
{
   if (fd < 0) return false;
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags < 0) return false;
   flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

int recv_wrapper(int sock, void *recv_data, int len, int flag) {
	int rv = -1;
	set_socket_blocking_enabled(sock,false);
	while( (rv = recv(sock,recv_data,len,flag)) < 0 )
		;
	set_socket_blocking_enabled(sock,true);
	return rv;
}

char * readLine ( char *buffer )
{
	char *ch = buffer;
	while ((*ch = fgetc(stdin_file_descriptor)) != '\n')
		ch++;
	*ch = 0;
	return buffer;
}

void logger_error (char *string) {
	if (colorize == true) {
		fprintf(stderr_file_descriptor, "%s%s%s%s\n", COLOR_RED , MESSAGE_ERROR , string , COLOR_RESET);
	} else {
		fprintf(stderr_file_descriptor, "%s%s\n", MESSAGE_ERROR, string);
	}
}

void logger_warn (char *string) {
	if (colorize == true) {
		fprintf(stderr_file_descriptor, "%s%s%s%s\n", COLOR_YELLOW , MESSAGE_WARN , string , COLOR_RESET);
	} else {
		fprintf(stderr_file_descriptor, "%s%s\n", MESSAGE_WARN, string);
	}
}

void logger_info (char *string) {
	if (colorize == true) {
		fprintf(stderr_file_descriptor, "%s%s%s%s\n", COLOR_BLUE , MESSAGE_INFO , string , COLOR_RESET);
	} else {
		fprintf(stderr_file_descriptor, "%s%s\n", MESSAGE_INFO, string);
	}	
}

void parse_to_array ( char * line, char ** argv )
{
	while (*line != '\0' && *line != '>' && *line != '<')
	{   
		while (*line == ' ' || *line == '\t' || *line == '\n')
			*line++ = '\0';
		*argv++ = line;
		while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
			line++;
	}   
	*argv = '\0';
}


void parse_arguments(const int argc, char * const *argv) {
	char option;
	while ((option = getopt(argc, argv,"i:o:e:p:l:csh:tu")) != -1) {
		switch (option) {
			case 'i':
				stdin_file_descriptor = fopen(optarg, "r");
				if (!stdin_file_descriptor) {
					fprintf(stderr, "%s\n", "INVALID FILE");
					exit(EXIT_FAILURE);
				}
				break;
			case 'o':
				stdout_file_descriptor = fopen(optarg, "a");
				if (!stdout_file_descriptor) {
					fprintf(stderr, "%s\n", "INVALID FILE");
					exit(EXIT_FAILURE);
				}
				colorize = false;
				break;
			case 'e':
				stderr_file_descriptor = fopen(optarg, "a");
				if (!stderr_file_descriptor) {
					fprintf(stderr, "%s\n", "INVALID FILE");
					exit(EXIT_FAILURE);
				}
				colorize = false;
				break;
			case 'p':
				server_port = atoi(optarg);
				break;
			case 'l':
				client_port = atoi(optarg);
				break;
			case 'c':
				is_client = true;
				break;
			case 's':
				is_server = true;
				break;
			case 'h':
				server_hostname = strdup(optarg);
				break;
			case 't':
				connection_type = strdup("tcp");
				break;
			case 'u':
				connection_type = strdup("udp");
				break;

			default: 
				print_usage();
				exit(EXIT_FAILURE);
		}
	}
	if (is_server == false and is_client == false) {
		fprintf(stderr, "%s\n", "MUST BE EITHER CLIENT OR SERVER");
		if (server_hostname) {
			free (server_hostname);
			server_hostname = NULL;
		}
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (!stderr_file_descriptor) {
		stderr_file_descriptor = stderr;
	}
	if (!stdout_file_descriptor) {
		stdout_file_descriptor = stdout;
	}
	if (!stdin_file_descriptor) {
		stdin_file_descriptor = stdin;
	}
	if (client_port == 0) {
		logger_warn("NO PORT DEFINED, USING DEFAULT 1143");
		client_port = 1143; // Will figure out what port I got from get socket binding.
	}

	if (is_server == true and is_client == true) {
		logger_warn("THE OUTPUTS OF BOTH SERVER AND CLIENT WILL BE ON SAME FILE!!");
	}
	if (!connection_type) {
		logger_warn("NO CONNECTION DEFINED, USING DEFAULT TCP");
		connection_type = strdup("tcp");
	}
	if (!server_hostname) {
		logger_warn("NO HOSTNAME DEFINED, USING DEFAULT 127.0.0.1");
		server_hostname = strdup("127.0.0.1");
	}
}

void print_usage() {
	fprintf(stderr, "%s\n", "ftutil [-c] [-s] [-t] [-u] [-h serverHostname] [-p listen_port] [-l local_port] [-i inputFile] [-o outputFile] [-i stderr_file_descriptor]");
}


void IndexGet_handler(char **argument_list, int len, int sock, int connection, struct sockaddr_in server_addr) {
	if (len < 2) {
		//printf("len: %d\n", len);
		logger_error("TOO FEW ARGUMENTS TO INDEXGET");
		return;
	}

	int list_type = 0;
	int reti;
	regex_t regex;

	if(len >= 2) {
		if (strcmp(argument_list[1], "--shortlist") == 0) 
			list_type = 1;
		else if (strcmp(argument_list[1], "--longlist") == 0) 
			list_type = 2;
		else if (strcmp(argument_list[1], "--regex") == 0) 
		{
			list_type = 3;

			/* Compile regular expression */
			reti = regcomp(&regex, argument_list[2], 0);
			if (reti) {
				logger_error("COULD NOT COMPILE REGEX");
				strcpy(server_send_buffer, "COULD NOT COMPILE REGEX");
				return;
			}

		}
		else{
			logger_error("INCORRECT FORMAT. FLAGS ALLOWED ARE shortlist, longlist, regex");
			return;
		}
	}
	char output[4096000] = {'\0'};		// This stores the string to be sent via server_send_buffer

	// Do the 'ls' part using syscalls
	int i, fd, bpos, BUF_SIZE = 1024;
	char buf[1024], d_type;
	struct linux_dirent *d;

	struct stat st_buf;
	char dirpath[1024] = {'\0'};
	strcpy(dirpath, ".");

	lstat(dirpath, &st_buf);
	fd = open(".", O_RDONLY | O_DIRECTORY);		// Open Server Share Directory
	if (fd == -1)
		logger_error("COULD NOT OPEN SHARE DIRECTORY");

	while(1) 
	{
		int nread;

		nread = syscall(SYS_getdents, fd, buf, BUF_SIZE);    // Using syscall() to indirectly call getdents as there is no wrapper for getdents in C
		if (nread == -1)
			logger_error("GETDENTS ERROR");
		if (nread == 0)		// No more files left
			break;

		for (bpos = 0; bpos < nread; bpos += d->d_reclen) 
		{	
			int hidden, direc, link, exec, allowed;
			hidden = link = direc = exec = 0;
			allowed = 1;
			d = (struct linux_dirent *) (buf + bpos);

			if((d->d_name)[0] == '.' )	// Filenames starting with '.' are hidden
				hidden = 1;
			if(!hidden)
			{
				d_type = *(buf + bpos + d->d_reclen - 1);
				if(d_type == DT_LNK)
					link = 1;
				else if(d_type == DT_DIR)
					direc = 1;

				char linkname[200] = {'\0'};
				int err = 0;
				char filepath[1024]={'\0'};
				
				// Concatenate dir (stored in 'dirpath') and file (stored in d->d_name) names to get entire path in filepath
				strcpy(filepath, dirpath);
				int slash = 0;
				i = 0;
				while(filepath[i])				// Checking whether input directory contains '/' at the end
				{
					if(filepath[i] == '/' && filepath[i+1] == '\0')
					slash = 1;
					i++;
				}
				if(!slash)
				{
					filepath[i] = '/';
					filepath[i+1] = '\0';
				}
				strcat(filepath, d->d_name);

				// Start doing 'ls'
				struct stat sb;
				if(lstat(filepath, &sb) == -1)
				{
					logger_error("Do not have search permissions in directory leading to destination");
					return;
				}

				//exec = list_contents(sb, link, direc, h);
				exec=0;
				if( sb.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH) )
					exec = 1;


				//lstat(filepath, &sb);
				if(link)
				{
					ssize_t r = readlink(filepath, linkname, sb.st_size + 1);
					if(r == -1)
						err = 1;
					linkname[r] = '\0';
				}

				// Printing file/directory name
				if(list_type == 3)		// Regex
				{
					/* Execute regular expression */
					reti = regexec(&regex, d->d_name, 0, NULL, 0);
					if (!reti) {
						printf("%s Matches regex\n", d->d_name);
						allowed = 1;
					}
					else if (reti == REG_NOMATCH) {
						allowed = 0;
					}
					else {
						// regerror(reti, &regex, msgbuf, sizeof(msgbuf));
						// fprintf(stderr, "Regex match failed: %s\n", msgbuf);
						// exit(1);
						allowed = 0;
					}
				}

				i = 0;
				if(allowed){
				char name_temp[50] = {'\0'};
				sprintf(name_temp, "%35s", d->d_name);
				strcat(output, name_temp);

				if(link)
				{
					strcat(output, " -> ");
					if(err)
						write(1,"Error in reading link (maybe Permission denied)",47);
					strcat(output, linkname);
				}

				// Print Size
				char size_temp[4096] = {'\0'};
				sprintf(size_temp, "%15d", (int)sb.st_size);
				strcat(output, size_temp);
		

				// Print Last Modified Time				
				char time_temp[1024] = {'\0'};
				strftime(time_temp, sizeof(time_temp), "%c", localtime(&sb.st_mtime));
				
				char time_temp2[4096] = {'\0'};
				sprintf(time_temp2, "%40s", time_temp);
				
				//char *temp = strdup(ctime(&sb.st_mtime));
				strcat(output, time_temp2);
				//free (temp);
				//temp = NULL;


				// Print Type of file
				//strcat(output, "    ");
				char type_temp[4096] = {'\0'};
				if(link)
					sprintf(type_temp, "%20s", "link");
				else if(direc)	
					sprintf(type_temp, "%20s", "directory");
				else if(exec)
					sprintf(type_temp, "%20s", "executable");
				else
					sprintf(type_temp, "%20s", "normal");

				strcat(output, type_temp);
				
				strcat(output, "\n");

			}	// Closing allowed wala 'for'
			
			}
			
		}
	}
	
	if (len == 2 && strcmp(argument_list[1], "--longlist") == 0) {
		printf("OUTPUT:\n%s\n", output);
		// SEND HERE
		int i = 0;
		char c;
		while(output[i] != '\0') {
			c = output[i];
			int count = 0;
			server_send_buffer[count++] = c;
			i++;
			while(count < 1000 && output[i] != '\0') {
				c = output[i++];
				server_send_buffer[count++] = c;
			}
			server_send_buffer[count] = '\0';
			if (strcmp(connection_type, "tcp") == 0) {
				send(connection, server_send_buffer, 1024 * sizeof(char),0); // packet_contents.
			} else {
				if (strcmp(connection_type, "udp")) {
					sendto(sock, server_send_buffer, 1024 * sizeof(char), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
				}
			}
		}
		logger_info("INDEXGET --longlist REQUEST SERVED");
	}
	else if (len == 4 && strcmp(argument_list[1], "--shortlist") == 0) {
	
	}

	else if (len == 3 && strcmp(argument_list[1], "--regex") == 0) {
		printf("OUTPUT:\n%s\n", output);
		// SEND HERE
		int i = 0;
		char c;
		while(output[i] != '\0') {
			c = output[i];
			int count = 0;
			server_send_buffer[count++] = c;
			i++;
			while(count < 1000 && output[i] != '\0') {
				c = output[i++];
				server_send_buffer[count++] = c;
			}
			server_send_buffer[count] = '\0';
			if (strcmp(connection_type, "tcp") == 0) {
				send(connection, server_send_buffer, 1024 * sizeof(char),0); // packet_contents.
			} else {
				if (strcmp(connection_type, "udp")) {
					sendto(sock, server_send_buffer, 1024 * sizeof(char), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
				}
			}
		}
		logger_info("INDEXGET --regex REQUEST SERVED");
	}
}

void FileHash_handler(char **argument_list, int len, int sock, int connection, struct sockaddr_in server_addr) {

}

void FileUpload_handler(char **argument_list, int len, int sock, int connection, struct sockaddr_in server_addr) {
	if (len < 2) {
		logger_error("FILENAME NOT SPECIFIED");		
		return;
	}
	if (len > 2) {
		logger_warn("MORE THAN ONE ARGUMENT; CHOOSING FIRST ONLY");
	}
	char filename[1024] = {'\0'};
	strcpy(filename, argument_list[1]);

}

void FileDownload_handler(char **argument_list, int len, int sock, int connection, struct sockaddr_in server_addr) {
	if (len < 2) {
		logger_error("FILENAME NOT SPECIFIED");		
		return;
	}
	if (len > 2) {
		logger_warn("MORE THAN ONE ARGUMENT; CHOOSING FIRST ONLY");
	}
	char filename[1024] = {'\0'};
	strcpy(filename, argument_list[1]);	
	
	if(access(filename, R_OK) != 0) {
		logger_error("UNABLE TO ACCESS FILE");
		return;
	} else {
		char c;
		
		FILE *file = fopen(filename, "r");

		while(fscanf(file, "%c",&c)!=EOF) {
			int count = 0;
			server_send_buffer[count++] = c;
			while(count < 1000 && fscanf(file, "%c",&c) != EOF) {
				server_send_buffer[count++] = c;
			}
			server_send_buffer[count] = '\0';
			if (strcmp(connection_type, "tcp") == 0) {
				send(connection, server_send_buffer, 1024 * sizeof(char),0); // packet_contents.
			} else {
				if (strcmp(connection_type, "udp")) {
					sendto(sock, server_send_buffer, 1024 * sizeof(char), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
				}
			}
		}
		fclose(file);
	}
}

void PING_handler(char ** argument_list, int len, int sock, int connection, struct sockaddr_in server_addr) {
	strcpy(server_send_buffer, "PONG");
}


void server_query_handler (int sock, int connection, struct sockaddr_in server_addr, struct sockaddr_in client_addr) {
	char *argument_list[1000];
	parse_to_array(server_receive_buffer, argument_list);
	int len;

	for (len = 0; argument_list[len]; len++)
		;

	if (strcmp(argument_list[0], "PING") == 0) {
		PING_handler(argument_list, len, sock, connection, client_addr);

		if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
		} else {
			if (strcmp(connection_type , "udp") == 0) {
				sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}
		strcpy(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");
		if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
		} else {
			if (strcmp(connection_type , "udp") == 0) {
			sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}

		return;
	}

	if (strcmp(argument_list[0], "IndexGet") == 0) {
		if (len < 2) {
			logger_error("TOO FEW ARGUMENTS TO INDEXGET");
			strcpy(server_send_buffer, "TOO FEW ARGUMENTS TO INDEXGET.\nUSAGE: IndexGet <flag> [<arg1> <arg2>]");
		}
		else if ( (strcmp(argument_list[1], "--shortlist") == 0 && len != 4) || (strcmp(argument_list[1], "--regex") == 0 && len != 3) || (strcmp(argument_list[1], "--longlist") == 0 && len != 2) ) {
			logger_error("INVALID FORMAT. PLEASE CHECK SYNTAX OF INDEXGET");
			strcpy(server_send_buffer, "INVALID FORMAT. PLEASE CHECK SYNTAX OF INDEXGET\nUSAGE: IndexGet <flag> [<arg1> <arg2>]");
		}
		else
			IndexGet_handler(argument_list, len, sock, connection, client_addr);
	
		strcat(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");

		if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
		} else {
			if (strcmp(connection_type , "udp") == 0) {
				sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}

		return;
	}

	if (strcmp(argument_list[0], "FileHash") == 0) {
		FileHash_handler(argument_list, len, sock, connection, client_addr);

		strcat(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");

		if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
		} else {
			if (strcmp(connection_type , "udp") == 0) {
				sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}

		return;
	}

	if (strcmp(argument_list[0], "FileUpload") == 0) {
		FileUpload_handler(argument_list, len, sock, connection, client_addr);

		strcat(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");

		if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
		} else {
			if (strcmp(connection_type , "udp") == 0) {
				sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}

		return;
	}

	if (strcmp(argument_list[0], "FileDownload") == 0) {
		FileDownload_handler(argument_list, len, sock, connection, client_addr);

		strcat(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");

		if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
		} else {
			if (strcmp(connection_type , "udp") == 0) {
				sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}

		return;
	}

	else
		strcpy(server_send_buffer, "INVALID COMMAND.\nList of Allowed Commands is:\n1) IndexGet\n2) FileHash\n3) FileDownload\n4) FileUpload\n5) exit");

	strcat(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");

	if (strcmp(connection_type, "tcp") == 0) {
			send(connection, server_send_buffer, 1024, 0);
	} else {
		if (strcmp(connection_type , "udp") == 0) {
			sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
		}
	}

	logger_error("UNKNOWN COMMAND");
	logger_warn("IGNORING UNKNOWN COMMAND");
}


void run_as_server() {
	fflush(stdout_file_descriptor);
	fflush(stderr_file_descriptor);
	int pid = fork();
	int status;

	if (pid == 0) {
		if (is_client == true) {
			// Do nothing
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		int sock, connection;
		struct sockaddr_in server_addr, client_addr;
		ssize_t bytes_recv;
		int sin_size;
		char print_buffer[4096];

		if(strcmp(connection_type, "tcp") == 0) {
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == -1){
				logger_error("UNABLE TO RETRIEVE SOCKET");
				exit(EXIT_FAILURE);
			}
		} else {
			if(strcmp(connection_type, "udp") == 0) {
				sock = socket(AF_INET, SOCK_DGRAM, 0);
				if (sock == -1) {
					logger_error("UNABLE TO RETRIEVE SOCKET");
					exit(EXIT_FAILURE);
				}
			}
		}

		logger_info("SOCKET RETRIEVED");
		
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(server_port);
		server_addr.sin_addr.s_addr = INADDR_ANY;
		bzero(&(server_addr.sin_zero), 8);
		if(bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)  {
			logger_error("UNABLE TO BIND SOCKET");
			exit(EXIT_FAILURE);
		}

		logger_info("SOCKET BOUND");

		if (strcmp(connection_type, "tcp") == 0) {
			if (listen(sock, 10) == -1) {
				logger_error("UNABLE TO LISTEN ON SOCKET");
				exit(EXIT_FAILURE);
			}
		}
		server_send_buffer = (char *) calloc(sizeof(char) , 1024);
		server_receive_buffer = (char *) calloc(sizeof(char), 1024);

		logger_info("LISTENING ON SOCKET");

		while(1) {
			sin_size = sizeof(struct sockaddr_in);
			socklen_t *temp_sock_len = (socklen_t *) &sin_size;
			if (strcmp(connection_type, "tcp") == 0) {
				connection = accept(sock, (struct sockaddr *)&client_addr, temp_sock_len);
				logger_info("CONNECTION ESTABLISHED");
			}
			while(1) {
				memset(server_send_buffer, 0, 1024*sizeof(char));
				memset(server_receive_buffer, 0, 1024*sizeof(char));

				if(strcmp(connection_type, "tcp") == 0) {
					bytes_recv = recv_wrapper(connection, server_receive_buffer, 1024, 0);
				} else {
					if(strcmp(connection_type, "udp") == 0) {
						bytes_recv = recvfrom(sock, server_receive_buffer, 1024, 0, (struct sockaddr *)&client_addr, temp_sock_len);
					}
				}
				server_receive_buffer[bytes_recv] = '\0';

				strcpy(print_buffer, "\nRECEIVED REQUEST:\n-----------------\n");
				strcat(print_buffer , server_receive_buffer);
				logger_info(print_buffer);

				if( bytes_recv == 0 || strcmp(server_receive_buffer,"exit") == 0) {
					logger_info("CONNECTION CLOSED");
					close(connection);
					break;
				}
				server_query_handler(sock, connection, server_addr, client_addr);

			}
		}
		close(sock);
	}
}

void run_as_client() {
	fflush(stdout_file_descriptor);
	fflush(stderr_file_descriptor);
	int pid = fork();
	int status;
	int sin_size = sizeof(struct sockaddr_in);
	socklen_t *temp_sock_len = (socklen_t *) &sin_size;

	if (pid == 0) {
		waitpid(pid, &status, 0);
	} else {
		int sock;
		struct hostent *host;
		struct sockaddr_in server_addr;

		char command[1000];
		char buffer[1024000];

		host = gethostbyname(server_hostname);
		if(!host) {
			logger_error("INVALID HOST ADDRESS");
			exit(EXIT_FAILURE);
		}

		logger_info("FOUND HOST");
		
		if (strcmp(connection_type, "tcp") == 0) {
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == -1) {
				logger_error("UNABLE TO RETRIEVE SOCKET");
				exit(EXIT_FAILURE);
			}
		} else { 
			if (strcmp(connection_type, "udp") == 0) {
				sock = socket(AF_INET, SOCK_DGRAM, 0);
				if (sock == -1) {
					logger_error("UNABLE TO RETRIEVE SOCKET");
					exit(EXIT_FAILURE);
				}
			}
		}

		logger_info("SOCKET RETRIEVED");
	
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(client_port);
		server_addr.sin_addr = *((struct in_addr *)host->h_addr);
		bzero(&(server_addr.sin_zero),8);
		if (strcmp(connection_type, "tcp") == 0) {
			if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
				logger_error("UNABLE TO CONNECT TO SOCKET");
				exit(EXIT_FAILURE);
			}
		} 

		logger_info("CONNECTED TO SOCKET");

		client_send_buffer = (char *) calloc(sizeof(char) * 1024, 0);
		client_receive_buffer = (char *) calloc(sizeof(char) * 1024, 0);

		while (1)
		{
			printf(">>  ");
			readLine(command);
			while (command[0] == '\0')
			{
				printf(">>  ");
				readLine(command);
			}
			if (strncmp (command , "quit" , 4) == 0)
				break;
			if (strncmp (command , "exit" , 4) == 0)
				break;
			if (strncmp (command , "easter-egg" , 10) == 0) {
				fprintf(stdout_file_descriptor ,
					"\n"
					"         ##\n"
					"          ###\n"
					"           ####\n"
					"            #####\n"
					"            #######\n"
					"             #######\n"
					"             ########\n"
					"             ########\n"
					"             #########\n"
					"             ##########\n"
					"            ############\n"
					"          ##############\n"
					"         ################\n"
					"          ################                                                  #\n"
					"            ##############                                                ###\n"
					"             ##############                                              ####\n"
					"             ##############                                           #####\n"
					"              ##############                                      #######\n"
					"              ##############                                 ###########\n"
					"              ###############                              #############\n"
					"              ################                           ##############\n"
					"             #################        #                ################\n"
					"             ##################      ##    #          #################\n"
					"            ####################    ###   ##         #################\n"
					"                 ################   ########         #################\n"
					"                  ################  #######         ###################\n"
					"                    #######################       #####################\n"
					"                     #####################       ###################\n"
					"                       ############################################\n"
					"                        ###########################################\n"
					"                        ##########################################\n"
					"                         ########################################\n"
					"                         ########################################\n"
					"                          ######################################\n"
					"                          ######################################\n"
					"                           ##########################      #####\n"
					"                           ###  ###################           ##\n"
					"                           ##    ###############               #\n"
					"                           #     ##  ##########\n"
					"                                     ##    ###\n"
					"                                           ###\n"
					"                                           ##\n"
					"                                           #\n"
					"\n"
					"   LONG   LIVE   THE   BAT.\n"
					"\n");
				break;
			}

			memset(client_receive_buffer, 0 , sizeof(char) * 1024);
			memset(client_send_buffer, 0, sizeof(char) * 1024);

			strcpy(client_send_buffer, command);

			if(strcmp(connection_type, "tcp") == 0) {
				send(sock, client_send_buffer, strlen(client_send_buffer), 0);
			} else {
				if (strcmp(connection_type, "udp") == 0) {
					sendto(sock, client_send_buffer, strlen(client_send_buffer), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
				}
			}

			if(strcmp(connection_type, "tcp") == 0) {
				recv_wrapper(sock, client_receive_buffer, sizeof(char) * 1024, 0);
				//printf("%s\n", client_receive_buffer);
			} else {
				if (strcmp(connection_type , "udp") == 0) {
					recvfrom(sock, client_receive_buffer, sizeof(char) * 1024, 0,(struct sockaddr *)&server_addr, temp_sock_len);
				}
			}

			strcpy(buffer, client_receive_buffer);

			while (!strstr(client_receive_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN")) {
				if(strcmp(connection_type, "tcp") == 0) {
					recv_wrapper(sock, client_receive_buffer, sizeof(char) * 1024, 0);
					//printf("%s\n", client_receive_buffer);
				} else {
					if (strcmp(connection_type , "udp") == 0) {
						recvfrom(sock, client_receive_buffer, sizeof(char) * 1024, 0,(struct sockaddr *)&server_addr, temp_sock_len);
					}
				}
				strcat (buffer, client_receive_buffer);
			}

			char * zero_out = strstr(buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");
			*zero_out = '\0';

			fprintf(stdout_file_descriptor, "%s\n", buffer);
		}
		close(sock);
	}
}

int main(const int argc, char * const * argv)
{
	parse_arguments(argc, argv);

	if (is_server) {
		run_as_server();
	}

	if (is_client) {
		run_as_client();
	}

	if (stdin_file_descriptor)
		fclose(stdin_file_descriptor);
	if (stdout_file_descriptor)
		fclose(stdout_file_descriptor);
	if (stderr_file_descriptor)
		fclose(stderr_file_descriptor);
	if (server_hostname) {
		free (server_hostname);
	}
	if (connection_type) {
		free (connection_type);
	}
	return 0;
}
