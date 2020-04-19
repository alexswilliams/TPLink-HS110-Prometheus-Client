#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include "connection.h"

int openConnection(const char *const hostname, const char *const port) {
    struct addrinfo hint;
    memset(&hint, 0, sizeof hint);
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    struct addrinfo *addrInfoFirst;
    const int result = getaddrinfo(hostname, port, &hint, &addrInfoFirst);
    if (result != 0) {
        fprintf(stderr, "Could not resolve '%s' - %s\n", hostname, gai_strerror(result));
        fflush(stderr);
        return -1;
    }

    for (struct addrinfo *addrInfo = addrInfoFirst; addrInfo != NULL; addrInfo = addrInfo->ai_next) {
        const int sck = socket(addrInfo->ai_family, SOCK_STREAM, addrInfo->ai_protocol);
        if (sck == -1) {
            fprintf(stderr, "Could not create socket - error %d (%s).\n", errno, strerror(errno));
            fflush(stderr);
            continue;
        }

        const int connResult = connect(sck, addrInfo->ai_addr, addrInfo->ai_addrlen);
        if (connResult == -1) {
            fprintf(stderr, "Could not connect - error %d (%s).\n", errno, strerror(errno));
            fflush(stderr);
            close(sck);
            continue;
        }

        // printf("Successfully connected to %s!\n", hostname);
        return sck;
    }
    if (addrInfoFirst != NULL) freeaddrinfo(addrInfoFirst);
    return -1;
}

void closeConnection(const int connection) {
    shutdown(connection, SHUT_RDWR);
    close(connection);
}
