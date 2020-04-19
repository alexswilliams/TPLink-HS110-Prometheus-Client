#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

#include "prometheus.h"
#include "connection.h"

static const size_t responseBufferSize = 256;


void communicateWithPushGateway(const struct config *const config, const char *const bodyBuffer,
                                const char *const headerBuffer, const size_t bodySize, const size_t headerSize) {
    int sck = openConnection(config->pushGatewayHost, config->pushGatewayPort);
    if (sck == -1) {
        fprintf(stderr, "Couldn't open connection to push gateway\n");
        fflush(stderr);
        return;
    }

    const ssize_t headerWritten = send(sck, headerBuffer, headerSize, 0);
    if (headerWritten == -1) {
        fprintf(stderr, "Couldn't write request header to push gateway: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        closeConnection(sck);
        return;
    }
    if ((size_t) headerWritten != headerSize) {
        fprintf(stderr, "Couldn't write full header to push gateway: wrote %zd of %zu.\n", headerWritten, headerSize);
        fflush(stderr);
        closeConnection(sck);
        return;
    }

    if (bodyBuffer != NULL && bodySize > 0) {
        const ssize_t bodyWritten = send(sck, bodyBuffer, bodySize, 0);
        if (bodyWritten == -1) {
            fprintf(stderr, "Couldn't write request body to push gateway: error %d: %s.\n", errno, strerror(errno));
            fflush(stderr);
            closeConnection(sck);
            return;
        }
        if ((size_t) bodyWritten != bodySize) {
            fprintf(stderr, "Couldn't write full body to push gateway: wrote %zd of %zu.\n", bodyWritten, bodySize);
            fflush(stderr);
            closeConnection(sck);
            return;
        }
    }

    char readBuffer[responseBufferSize];
    memset(readBuffer, 0, responseBufferSize);

    const ssize_t bytesRead = recv(sck, readBuffer, responseBufferSize, 0);
    if (bytesRead <= 0) {
        fprintf(stderr, "Couldn't read response from push gateway: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        closeConnection(sck);
        return;
    }
    closeConnection(sck);
    readBuffer[bytesRead] = '\0';

    if (bytesRead < 10) {
        fprintf(stderr, "Invalid response received from push gateway - read %zu bytes: %s", bytesRead, readBuffer);
        fflush(stderr);
        return;
    }

    // e.g. HTTP/1.0 400 Bad Request
    //      012345678901...
    const char *responseCode = readBuffer + 9;
    if (responseCode[0] != '2') {
        const char *body = strstr(readBuffer, "\r\n\r\n");
        if (body == NULL) body = "N/A";
        fprintf(stderr, "Error received from push gateway: %s", body + 4);
        fflush(stderr);
    }
}


static const size_t bodyBufferSize = 1024;
static const size_t headerBufferSize = 256;

void registerNewMetrics(const struct config *config, const char *tags,
                        const struct sysInfo *sysInfo, const struct realTimeInfo *realTimeInfo) {

    char bodyBuffer[bodyBufferSize];
    char headerBuffer[headerBufferSize];

    snprintf(bodyBuffer, bodyBufferSize,
             "# TYPE state gauge\n"
             "state{%1$s} %2$0.0f\n"
             "# TYPE on_time gauge\n"
             "on_time{%1$s} %3$0.3f\n"
             "# TYPE voltageMv gauge\n"
             "voltageMv{%1$s} %4$0.3f\n"
             "# TYPE currentMa gauge\n"
             "currentMa{%1$s} %5$0.3f\n"
             "# TYPE powerMw gauge\n"
             "powerMw{%1$s} %6$0.3f\n"
             "# TYPE totalWh gauge\n"
             "totalWh{%1$s} %7$0.3f\n",
             tags,
             sysInfo->state, sysInfo->onTimeSeconds, realTimeInfo->voltageMv, realTimeInfo->currentMa,
             realTimeInfo->powerMw, realTimeInfo->totalWh
    );
    const size_t bodySize = strlen(bodyBuffer);
    snprintf(headerBuffer, headerBufferSize,
             "POST %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/plain\r\n"
             "\r\n",
             config->pushGatewayEndpoint, config->pushGatewayHost, bodySize
    );

    // printf("Buffer: %s%s\n\n", headerBuffer, bodyBuffer);

    communicateWithPushGateway(config, bodyBuffer, headerBuffer, bodySize, strlen(headerBuffer));
}


void deleteMetrics(const struct config *config) {

    char headerBuffer[headerBufferSize];
    snprintf(headerBuffer, headerBufferSize,
             "DELETE %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Content-Length: 0\r\n"
             "\r\n",
             config->pushGatewayEndpoint, config->pushGatewayHost
    );

    communicateWithPushGateway(config, NULL, headerBuffer, 0, strlen(headerBuffer));
}
