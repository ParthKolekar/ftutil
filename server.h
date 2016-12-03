#ifndef _SERVER_H
#define _SERVER_H

#include "main.h"
#include "utils.h"

extern char * connection_type;

extern u_short server_port;

extern char * server_receive_buffer;
extern char * server_send_buffer;

bool server_query_handler (int sock, int connection, struct sockaddr_in server_addr, struct sockaddr_in client_addr) {
    char *argument_list[1000];
    parse_to_array(server_receive_buffer, argument_list);
    int len;

    for (len = 0; argument_list[len]; len++)
        ;

    if (strcmp(argument_list[0], "PING") == 0) {
    	strcpy(server_send_buffer, "PONG");
    } else if (strcmp(argument_list[0], "EXIT") == 0) {
        return false;
    } else {
        strcpy(server_send_buffer, "INVALID COMMAND.\nList of Allowed Commands is:\n1) PING\n2) EXIT");
        logger_error_server("UNKNOWN COMMAND");
        logger_warn_server("IGNORING UNKNOWN COMMAND");
    }

    strcat(server_send_buffer, "MAGIC-STRING-SPRINKLEBERRY-MUFFIN");

    if (strcmp(connection_type, "tcp") == 0) {
        send(connection, server_send_buffer, 1024, 0);
    } else {
        if (strcmp(connection_type, "udp") == 0) {
            sendto(sock, server_send_buffer, 1024, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
        }
    }
    return true;
}

void run_as_server() {
    fflush(stdout_file_descriptor);
    fflush(stderr_file_descriptor);

    int sock = 0;
    int connection = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    ssize_t bytes_recv;
    int sin_size;
    char print_buffer[4096];
    bool quit_received = false;

    if(strcmp(connection_type, "tcp") == 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1){
            logger_error_server("UNABLE TO RETRIEVE SOCKET");
            cleanup_client_server();
            exit(EXIT_FAILURE);
        }
    } else {
        if(strcmp(connection_type, "udp") == 0) {
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock == -1) {
                logger_error_server("UNABLE TO RETRIEVE SOCKET");
                cleanup_client_server();
                exit(EXIT_FAILURE);
            }
        }
    }

    logger_info_server("SOCKET RETRIEVED");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);
    if(bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)  {
        logger_error_server("UNABLE TO BIND SOCKET");
        cleanup_client_server();
        exit(EXIT_FAILURE);
    }

    logger_info_server("SOCKET BOUND");

    if (strcmp(connection_type, "tcp") == 0) {
        if (listen(sock, 10) == -1) {
            logger_error_server("UNABLE TO LISTEN ON SOCKET");
            cleanup_client_server();
            exit(EXIT_FAILURE);
        }
    }
    server_send_buffer = (char *) calloc(1024, sizeof(char));
    server_receive_buffer = (char *) calloc(1024, sizeof(char));

    logger_info_server("LISTENING ON SOCKET");

    while(!quit_received) {
        sin_size = sizeof(struct sockaddr_in);
        socklen_t *temp_sock_len = (socklen_t *) &sin_size;
        if (strcmp(connection_type, "tcp") == 0) {
            connection = accept(sock, (struct sockaddr *)&client_addr, temp_sock_len);
            logger_info_server("CONNECTION ESTABLISHED");
        }
        while(!quit_received) {
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
            strcat(print_buffer, server_receive_buffer);
            logger_info_server(print_buffer);

            if(bytes_recv == 0 || strcmp(server_receive_buffer, "EXIT") == 0) {
                logger_info_server("CONNECTION CLOSED");
                close(connection);
                break;
            }
            quit_received = !server_query_handler(sock, connection, server_addr, client_addr);
        }
    }
    close(sock);
}

#endif
