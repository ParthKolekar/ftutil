#include "main.h"
#include "utils.h"
#include "server.h"

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
    if (is_server == false && is_client == false) {
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
    if (is_client && client_port == 0) {
        fprintf(stderr, "%s\n", "NO PORT DEFINED, USING DEFAULT 1143");
        client_port = 1143; // Will figure out what port I got from get socket binding.
    }

    if (is_server == true && is_client == true) {
        fprintf(stderr, "%s\n", "THE OUTPUTS OF BOTH SERVER AND CLIENT WILL BE ON SAME FILE!!");
    }
    if (!connection_type) {
        fprintf(stderr, "%s\n", "NO CONNECTION DEFINED, USING DEFAULT TCP");
        connection_type = strdup("tcp");
    }
    if (!server_hostname) {
        fprintf(stderr, "%s\n", "NO HOSTNAME DEFINED, USING DEFAULT 127.0.0.1");
        server_hostname = strdup("127.0.0.1");
    }
}

void print_usage() {
    fprintf(stderr, "%s\n", "ftutil [-c] [-s] [-t] [-u] [-h serverHostname] [-p listen_port] [-l local_port] [-i inputFile] [-o outputFile] [-i stderr_file_descriptor]");
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

        bool quit_received = false;

        host = gethostbyname(server_hostname);
        if(!host) {
            logger_error_client("INVALID HOST ADDRESS");
            exit(EXIT_FAILURE);
        }

        logger_info_client("FOUND HOST");

        if (strcmp(connection_type, "tcp") == 0) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                logger_error_client("UNABLE TO RETRIEVE SOCKET");
                exit(EXIT_FAILURE);
            }
        } else { 
            if (strcmp(connection_type, "udp") == 0) {
                sock = socket(AF_INET, SOCK_DGRAM, 0);
                if (sock == -1) {
                    logger_error_client("UNABLE TO RETRIEVE SOCKET");
                    exit(EXIT_FAILURE);
                }
            }
        }

        logger_info_client("SOCKET RETRIEVED");

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(client_port);
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        memset(&(server_addr.sin_zero), 0, 8);

        if (strcmp(connection_type, "tcp") == 0) {
            if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
                logger_error_client("UNABLE TO CONNECT TO SOCKET");
                exit(EXIT_FAILURE);
            }
        } 

        logger_info_client("CONNECTED TO SOCKET");

        client_send_buffer = (char *) calloc(sizeof(char) * 1024, 0);
        client_receive_buffer = (char *) calloc(sizeof(char) * 1024, 0);

        while (!quit_received) {
            printf(">>  ");
            readLine(command);
            while (command[0] == '\0') {
                printf("%s ", ">>  ");
                readLine(command);
            }
            if (strcmp (command, "EXIT") == 0) {
                printf("%s\n", "Usage: EXIT REMOTE|SELF");
                continue;
            }
            if (strcmp (command, "EXIT SELF") == 0) {
                quit_received = true;
                break;
            }
            if (strcmp (command, "EXIT REMOTE") == 0) {
                quit_received = true;
            }
            if (strncmp (command, "easter-egg", 10) == 0) {
                fprintf(stdout_file_descriptor,
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
                continue;
            }

            memset(client_receive_buffer, 0, sizeof(char) * 1024);
            memset(client_send_buffer, 0, sizeof(char) * 1024);

            strcpy(client_send_buffer, command);

            if(strcmp(connection_type, "tcp") == 0) {
                send(sock, client_send_buffer, strlen(client_send_buffer), 0);
            } else {
                if (strcmp(connection_type, "udp") == 0) {
                    sendto(sock, client_send_buffer, strlen(client_send_buffer), 0,(struct sockaddr *)&server_addr, sizeof(struct sockaddr));
                }
            }

            if (quit_received) {
                break;
            }

            if(strcmp(connection_type, "tcp") == 0) {
                recv_wrapper(sock, client_receive_buffer, sizeof(char) * 1024, 0);
            } else {
                if (strcmp(connection_type, "udp") == 0) {
                    recvfrom(sock, client_receive_buffer, sizeof(char) * 1024, 0,(struct sockaddr *)&server_addr, temp_sock_len);
                }
            }

            strcpy(buffer, client_receive_buffer);

            while (!strstr(client_receive_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN")) {
                if(strcmp(connection_type, "tcp") == 0) {
                    recv_wrapper(sock, client_receive_buffer, sizeof(char) * 1024, 0);
                } else {
                    if (strcmp(connection_type, "udp") == 0) {
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

    if (stdin_file_descriptor && stdin_file_descriptor != stdin)
        fclose(stdin_file_descriptor);
    if (stdout_file_descriptor && stdout_file_descriptor != stdout)
        fclose(stdout_file_descriptor);
    if (stderr_file_descriptor && stderr_file_descriptor != stderr)
        fclose(stderr_file_descriptor);
    if (server_hostname) {
        free (server_hostname);
    }
    if (connection_type) {
        free (connection_type);
    }
    return 0;
}
