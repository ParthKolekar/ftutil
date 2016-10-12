#ifndef _CLIENT_H
#define _CLIENT_H

#include "main.h"
#include "utils.h"

extern char * server_hostname;
extern char * connection_type;
extern u_short client_port;

extern char * client_receive_buffer;
extern char * client_send_buffer;

void run_as_client() {
    fflush(stdout_file_descriptor);
    fflush(stderr_file_descriptor);
    int sin_size = sizeof(struct sockaddr_in);
    socklen_t *temp_sock_len = (socklen_t *) &sin_size;

    int sock;
    struct hostent *host;
    struct sockaddr_in server_addr;

    char command[1000];
    char buffer[1024000];

    bool quit_received = false;

    host = gethostbyname(server_hostname);
    if(!host) {
        logger_error_client("INVALID HOST ADDRESS");
        cleanup_client_server();
        exit(EXIT_FAILURE);
    }

    logger_info_client("FOUND HOST");

    if (strcmp(connection_type, "tcp") == 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            logger_error_client("UNABLE TO RETRIEVE SOCKET");
            cleanup_client_server();
            exit(EXIT_FAILURE);
        }
    } else {
        if (strcmp(connection_type, "udp") == 0) {
            sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock == -1) {
                logger_error_client("UNABLE TO RETRIEVE SOCKET");
                cleanup_client_server();
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
            cleanup_client_server();
            exit(EXIT_FAILURE);
        }
    }

    logger_info_client("CONNECTED TO SOCKET");

    client_send_buffer = (char *) calloc(1024, sizeof(char));
    client_receive_buffer = (char *) calloc(1024, sizeof(char));

    while (!quit_received) {
        printf(">>  ");
        readLine(command);
        while (command[0] == '\0') {
            printf("%s ", ">>  ");
            readLine(command);
        }
        if (strncmp (command, "EXIT", 4) == 0) {
            if (strncmp (command, "EXIT SELF", 9) == 0) {
                quit_received = true;
                break;
            }
            if (strncmp (command, "EXIT REMOTE", 11) == 0) {
                quit_received = true;
            } else {
                printf("%s\n", "Usage: EXIT REMOTE|SELF");
                continue;
            }
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

        memset(client_send_buffer, 0, 1024 * sizeof(char));
        memset(client_receive_buffer, 0, 1024 * sizeof(char));

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

#endif