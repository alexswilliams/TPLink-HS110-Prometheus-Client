#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "connection.h"

int open_connection(const char *const hostname, const char *const port) {
    struct addrinfo hint;
    memset(&hint, 0, sizeof hint);
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    struct addrinfo *addrinfo_first;
    const int result = getaddrinfo(hostname, port, &hint, &addrinfo_first);
    if (result != 0) {
        fprintf(stderr, "Could not resolve '%s' - %s\n", hostname, gai_strerror(result));
        fflush(stderr);
        return -1;
    }

    for (struct addrinfo *addrinfo = addrinfo_first; addrinfo != NULL; addrinfo = addrinfo->ai_next) {
        const int sck = socket(addrinfo->ai_family, SOCK_STREAM, addrinfo->ai_protocol);
        if (sck == -1) {
            fprintf(stderr, "Could not create socket - error %d (%s).\n", errno, strerror(errno));
            fflush(stderr);
            continue;
        }

        const int conn_result = connect(sck, addrinfo->ai_addr, addrinfo->ai_addrlen);
        if (conn_result == -1) {
            fprintf(stderr, "Could not connect - error %d (%s).\n", errno, strerror(errno));
            fflush(stderr);
            close(sck);
            continue;
        }

        // printf("Successfully connected to %s!\n", hostname);
        return sck;
    }
    if (addrinfo_first != NULL) freeaddrinfo(addrinfo_first);
    return -1;
}

void close_connection(const int connection) {
    shutdown(connection, SHUT_RDWR);
    close(connection);
}
