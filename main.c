#include "main.h"
#include "utils.h"
#include "server.h"
#include "client.h"

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
                cleanup_client_server();
                exit(EXIT_FAILURE);
            }
            break;
            case 'o':
            stdout_file_descriptor = fopen(optarg, "a");
            if (!stdout_file_descriptor) {
                fprintf(stderr, "%s\n", "INVALID FILE");
                cleanup_client_server();
                exit(EXIT_FAILURE);
            }
            colorize = false;
            break;
            case 'e':
            stderr_file_descriptor = fopen(optarg, "a");
            if (!stderr_file_descriptor) {
                fprintf(stderr, "%s\n", "INVALID FILE");
                cleanup_client_server();
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
            if (connection_type) {
                free(connection_type);
                connection_type = NULL;
            }
            connection_type = strdup("tcp");
            break;
            case 'u':
            if (connection_type) {
                free(connection_type);
                connection_type = NULL;
            }
            connection_type = strdup("udp");
            break;

            default: 
            print_usage();
            if (connection_type) {
                free(connection_type);
                server_hostname = NULL;
            }
            if (server_hostname) {
                free(server_hostname);
                server_hostname = NULL;
            }
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
        fprintf(stderr, "%s\n", "CAN'T BE BOTH SERVER AND CLIENT!!");
        exit(EXIT_FAILURE);
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

int main(const int argc, char * const * argv)
{
    parse_arguments(argc, argv);

    if (is_server) {
        run_as_server();
    }

    if (is_client) {
        run_as_client();
    }
    cleanup_client_server();
    return 0;
}
