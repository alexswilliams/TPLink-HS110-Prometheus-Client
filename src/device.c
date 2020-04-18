#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>

#include "device.h"



int scramble(const char *input, unsigned char *output, size_t buffer_size, size_t *output_length);

int unscramble(const unsigned char *input, size_t input_length, char *output, size_t buffer_size,
               size_t *output_length);

static const size_t SCRAMBLE_BUFFER_SIZE = 128;
static const size_t UNSCRAMBLE_BUFFER_SIZE = 4096;

int query_device(const int connection, const char *const request, cJSON **out) {
    unsigned char request_scrambled[SCRAMBLE_BUFFER_SIZE];
    size_t scrambled_length;
    int scramble_result = scramble(request, request_scrambled, SCRAMBLE_BUFFER_SIZE, &scrambled_length);
    if (scramble_result != 0) {
        fprintf(stderr, "Could not scramble request.\n");
        fflush(stderr);
        return 1;
    }

    ssize_t bytes_written = send(connection, request_scrambled, scrambled_length, 0);
    if (bytes_written == -1) {
        fprintf(stderr, "Couldn't write request to device: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        return 1;
    }
    if ((size_t) bytes_written != scrambled_length) {
        fprintf(stderr, "Couldn't write all bytes to device: wrote %zd of %zu.\n", bytes_written, scrambled_length);
        fflush(stderr);
        return 1;
    }

    unsigned char response[UNSCRAMBLE_BUFFER_SIZE];
    memset(response, 0, UNSCRAMBLE_BUFFER_SIZE);
    const ssize_t bytes_read = recv(connection, response, UNSCRAMBLE_BUFFER_SIZE, 0);
    if (bytes_read == -1) {
        fprintf(stderr, "Couldn't read from device: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        return 1;
    }

    char response_unscrambled[UNSCRAMBLE_BUFFER_SIZE];
    size_t unscrambled_length;
    int unscramble_result = unscramble(response, (size_t) bytes_read, response_unscrambled, UNSCRAMBLE_BUFFER_SIZE,
                                       &unscrambled_length);
    if (unscramble_result != 0) {
        fprintf(stderr, "Could not unscramble result.\n");
        fflush(stderr);
        return 1;
    }

    response_unscrambled[unscrambled_length] = '\0';
    // printf("Device Response: %s\n", response_unscrambled);

    *out = cJSON_Parse(response_unscrambled);
    return 0;
}

void write_long_to_buffer_big_endian(unsigned char *const b, unsigned long i) {
    b[0] = (i >> 24u) & 0xffu;
    b[1] = (i >> 16u) & 0xffu;
    b[2] = (i >> 8u) & 0xffu;
    b[3] = i & 0xffu;
}

int scramble(const char *const input, unsigned char *const output,
             const size_t buffer_size, size_t *const output_length) {

    const size_t len = strlen(input);
    *output_length = len + 4;
    if (*output_length > buffer_size) {
        fprintf(stderr, "Message too long for buffer; given %zu, but needed %zu.\n", buffer_size, *output_length);
        fflush(stderr);
        return 1;
    }

    unsigned char iv = 171;
    write_long_to_buffer_big_endian(output, len);
    for (int i = 0; input[i] != '\0'; i++) {
        iv = iv ^ (unsigned char) (input[i]);
        output[i + 4] = iv;
    }
    return 0;
}

int unscramble(const unsigned char *const input, const size_t input_length, char *const output,
               const size_t buffer_size, size_t *const output_length) {

    if (input_length < 4) {
        fprintf(stderr, "Message too short: received %zu bytes.\n", input_length);
        fflush(stderr);
        return 1;
    }

    *output_length = input[3] + (input[2] << 8u) + (input[1] << 16u) + (input[0] << 24u);
    if (*output_length > buffer_size) {
        fprintf(stderr, "Message too long for buffer; given %zu, but needed %zu.\n", buffer_size, *output_length);
        fflush(stderr);
        return 1;
    }

    unsigned char iv = 171;
    for (unsigned int i = 4; i < input_length; i++) {
        ((unsigned char *) output)[i - 4] = iv ^ input[i];
        iv = input[i];
    }
    return 0;
}
