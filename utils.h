#ifndef _UTILS_H
#define _UTILS_H

#include "main.h"

extern FILE * stdin_file_descriptor;
extern FILE * stdout_file_descriptor;
extern FILE * stderr_file_descriptor;

extern bool colorize;

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

void logger_error_client (char *string) {
    if (colorize == true) {
        fprintf(stderr_file_descriptor, "%s%s%s%s%s\n", MESSAGE_CLIENT, COLOR_RED, MESSAGE_ERROR, string, COLOR_RESET);
    } else {
        fprintf(stderr_file_descriptor, "%s%s%s\n", MESSAGE_CLIENT, MESSAGE_ERROR, string);
    }
}

void logger_warn_client (char *string) {
    if (colorize == true) {
        fprintf(stderr_file_descriptor, "%s%s%s%s%s\n", MESSAGE_CLIENT, COLOR_YELLOW, MESSAGE_WARN, string, COLOR_RESET);
    } else {
        fprintf(stderr_file_descriptor, "%s%s%s\n", MESSAGE_CLIENT, MESSAGE_WARN, string);
    }
}

void logger_info_client (char *string) {
    if (colorize == true) {
        fprintf(stderr_file_descriptor, "%s%s%s%s%s\n", MESSAGE_CLIENT, COLOR_BLUE, MESSAGE_INFO, string, COLOR_RESET);
    } else {
        fprintf(stderr_file_descriptor, "%s%s%s\n", MESSAGE_CLIENT, MESSAGE_INFO, string);
    }	
}

void logger_error_server (char *string) {
    if (colorize == true) {
        fprintf(stderr_file_descriptor, "%s%s%s%s%s\n", MESSAGE_SERVER, COLOR_RED, MESSAGE_ERROR, string, COLOR_RESET);
    } else {
        fprintf(stderr_file_descriptor, "%s%s%s\n", MESSAGE_SERVER, MESSAGE_ERROR, string);
    }
}

void logger_warn_server (char *string) {
    if (colorize == true) {
        fprintf(stderr_file_descriptor, "%s%s%s%s%s\n", MESSAGE_SERVER, COLOR_YELLOW, MESSAGE_WARN, string, COLOR_RESET);
    } else {
        fprintf(stderr_file_descriptor, "%s%s%s\n", MESSAGE_SERVER, MESSAGE_WARN, string);
    }
}

void logger_info_server (char *string) {
    if (colorize == true) {
        fprintf(stderr_file_descriptor, "%s%s%s%s%s\n", MESSAGE_SERVER, COLOR_BLUE, MESSAGE_INFO, string, COLOR_RESET);
    } else {
        fprintf(stderr_file_descriptor, "%s%s%s\n", MESSAGE_SERVER, MESSAGE_INFO, string);
    }	
}

void parse_to_array ( char * line, char ** argv )
{
    while (*line != '\0' && *line != '>' && *line != '<') {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';
        *argv++ = line;
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
            line++;
    }
    *argv = 0;
}

#endif