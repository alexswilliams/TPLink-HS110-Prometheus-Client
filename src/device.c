#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>

#include "device.h"


int scramble(const char *input, unsigned char *output, size_t bufferSize, size_t *outputLength);

int unscramble(const unsigned char *input, size_t inputLength, char *output, size_t bufferSize,
               size_t *outputLength);

static const size_t scrambleBufferSize = 128;
static const size_t unscrambleBufferSize = 4096;

int queryDevice(const int connection, const char *const request, cJSON **out) {
    unsigned char requestScrambled[scrambleBufferSize];
    size_t scrambledLength;
    int scrambleResult = scramble(request, requestScrambled, scrambleBufferSize, &scrambledLength);
    if (scrambleResult != 0) {
        fprintf(stderr, "Could not scramble request.\n");
        fflush(stderr);
        return 1;
    }

    ssize_t bytesWritten = send(connection, requestScrambled, scrambledLength, 0);
    if (bytesWritten == -1) {
        fprintf(stderr, "Couldn't write request to device: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        return 1;
    }
    if ((size_t) bytesWritten != scrambledLength) {
        fprintf(stderr, "Couldn't write all bytes to device: wrote %zd of %zu.\n", bytesWritten, scrambledLength);
        fflush(stderr);
        return 1;
    }

    unsigned char response[unscrambleBufferSize];
    memset(response, 0, unscrambleBufferSize);
    const ssize_t bytesRead = recv(connection, response, unscrambleBufferSize, 0);
    if (bytesRead == -1) {
        fprintf(stderr, "Couldn't read from device: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        return 1;
    }

    char responseUnscrambled[unscrambleBufferSize];
    size_t unscrambledLength;
    int unscrambleResult = unscramble(response, (size_t) bytesRead, responseUnscrambled, unscrambleBufferSize,
                                      &unscrambledLength);
    if (unscrambleResult != 0) {
        fprintf(stderr, "Could not unscramble result.\n");
        fflush(stderr);
        return 1;
    }

    responseUnscrambled[unscrambledLength] = '\0';
    // printf("Device Response: %s\n", responseUnscrambled);

    *out = cJSON_Parse(responseUnscrambled);
    return 0;
}

void writeLongToBufferBigEndian(unsigned char *const b, unsigned long i) {
    b[0] = (i >> 24u) & 0xffu;
    b[1] = (i >> 16u) & 0xffu;
    b[2] = (i >> 8u) & 0xffu;
    b[3] = i & 0xffu;
}

int scramble(const char *const input, unsigned char *const output,
             const size_t bufferSize, size_t *const outputLength) {

    const size_t len = strlen(input);
    *outputLength = len + 4;
    if (*outputLength > bufferSize) {
        fprintf(stderr, "Message too long for buffer; given %zu, but needed %zu.\n", bufferSize, *outputLength);
        fflush(stderr);
        return 1;
    }

    unsigned char iv = 171;
    writeLongToBufferBigEndian(output, len);
    for (int i = 0; input[i] != '\0'; i++) {
        iv = iv ^ (unsigned char) (input[i]);
        output[i + 4] = iv;
    }
    return 0;
}

int unscramble(const unsigned char *const input, const size_t inputLength, char *const output,
               const size_t bufferSize, size_t *const outputLength) {

    if (inputLength < 4) {
        fprintf(stderr, "Message too short: received %zu bytes.\n", inputLength);
        fflush(stderr);
        return 1;
    }

    *outputLength = input[3] + (input[2] << 8u) + (input[1] << 16u) + (input[0] << 24u);
    if (*outputLength > bufferSize) {
        fprintf(stderr, "Message too long for buffer; given %zu, but needed %zu.\n", bufferSize, *outputLength);
        fflush(stderr);
        return 1;
    }

    unsigned char iv = 171;
    for (unsigned int i = 4; i < inputLength; i++) {
        ((unsigned char *) output)[i - 4] = iv ^ input[i];
        iv = input[i];
    }
    return 0;
}
