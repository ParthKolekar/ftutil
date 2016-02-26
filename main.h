#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <openssl/md5.h>
#include <stddef.h>
#include <dirent.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdlib.h>
#include <syscall.h>

void print_usage();

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#ifndef and
#define and &&
#endif

#ifndef or
#define or ||
#endif

#ifndef COLOR_RED
#define COLOR_RED "\033[1m\033[31m"
#endif

#ifndef COLOR_RESET
#define COLOR_RESET "\033[0m"
#endif

#ifndef COLOR_YELLOW
#define COLOR_YELLOW "\033[1m\033[33m" 
#endif

#ifndef COLOR_BLUE
#define COLOR_BLUE "\033[1m\033[34m"
#endif

#ifndef MESSAGE_ERROR
#define MESSAGE_ERROR "[ERROR] "
#endif

#ifndef MESSAGE_DEBUG
#define MESSAGE_DEBUG "[DEBUG] "
#endif

#ifndef MESSAGE_INFO
#define MESSAGE_INFO "[INFO] "
#endif

#ifndef MESSAGE_WARN
#define MESSAGE_WARN "[WARN] "
#endif

#ifndef MESSAGE_CLIENT
#define MESSAGE_CLIENT "[CLIENT] "
#endif

#ifndef MESSAGE_SERVER
#define MESSAGE_SERVER "[SERVER] "
#endif