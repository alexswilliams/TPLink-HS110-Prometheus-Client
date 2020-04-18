#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

#include "prometheus.h"
#include "connection.h"

static const size_t RESPONSE_BUFFER_SIZE = 256;


void communicate_with_push_gateway(const struct config *const config, const char *const body_buffer,
                                   const char *const header_buffer, const size_t body_size, const size_t header_size) {
    int sck = open_connection(config->push_gateway_host, config->push_gateway_port);
    if (sck == -1) {
        fprintf(stderr, "Couldn't open connection to push gateway\n");
        fflush(stderr);
        return;
    }

    const ssize_t header_written = send(sck, header_buffer, header_size, 0);
    if (header_written == -1) {
        fprintf(stderr, "Couldn't write request header to push gateway: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        close_connection(sck);
        return;
    }
    if ((size_t) header_written != header_size) {
        fprintf(stderr, "Couldn't write full header to push gateway: wrote %zd of %zu.\n", header_written, header_size);
        fflush(stderr);
        close_connection(sck);
        return;
    }

    if (body_buffer != NULL && body_size > 0) {
        const ssize_t body_written = send(sck, body_buffer, body_size, 0);
        if (body_written == -1) {
            fprintf(stderr, "Couldn't write request body to push gateway: error %d: %s.\n", errno, strerror(errno));
            fflush(stderr);
            close_connection(sck);
            return;
        }
        if ((size_t) body_written != body_size) {
            fprintf(stderr, "Couldn't write full body to push gateway: wrote %zd of %zu.\n", body_written, body_size);
            fflush(stderr);
            close_connection(sck);
            return;
        }
    }

    char read_buffer[RESPONSE_BUFFER_SIZE];
    memset(read_buffer, 0, RESPONSE_BUFFER_SIZE);

    const ssize_t bytes_read = recv(sck, read_buffer, RESPONSE_BUFFER_SIZE, 0);
    if (bytes_read <= 0) {
        fprintf(stderr, "Couldn't read response from push gateway: error %d: %s.\n", errno, strerror(errno));
        fflush(stderr);
        close_connection(sck);
        return;
    }
    close_connection(sck);
    read_buffer[bytes_read] = '\0';

    if (bytes_read < 10) {
        fprintf(stderr, "Invalid response received from push gateway - read %zu bytes: %s", bytes_read, read_buffer);
        fflush(stderr);
        return;
    }

    // e.g. HTTP/1.0 400 Bad Request
    //      012345678901...
    const char *response_code = read_buffer + 9;
    if (response_code[0] != '2') {
        const char *body = strstr(read_buffer, "\r\n\r\n");
        if (body == NULL) body = "N/A";
        fprintf(stderr, "Error received from push gateway: %s", body + 4);
        fflush(stderr);
    }
}


static const size_t BODY_BUFFER_SIZE = 1024;
static const size_t HEADER_BUFFER_SIZE = 256;

void register_new_metrics(const struct config *const config, const char *const tags,
                          const struct sys_info *const sys_info, const struct real_time_info *const real_time_info) {

    char body_buffer[BODY_BUFFER_SIZE];
    char header_buffer[HEADER_BUFFER_SIZE];

    snprintf(body_buffer, BODY_BUFFER_SIZE,
             "# TYPE state gauge\n"
             "state{%1$s} %2$0.0f\n"
             "# TYPE on_time gauge\n"
             "on_time{%1$s} %3$0.3f\n"
             "# TYPE voltage_mv gauge\n"
             "voltage_mv{%1$s} %4$0.3f\n"
             "# TYPE current_ma gauge\n"
             "current_ma{%1$s} %5$0.3f\n"
             "# TYPE power_mw gauge\n"
             "power_mw{%1$s} %6$0.3f\n"
             "# TYPE total_wh gauge\n"
             "total_wh{%1$s} %7$0.3f\n",
             tags,
             sys_info->state, sys_info->on_time_seconds, real_time_info->voltage_mv, real_time_info->current_ma,
             real_time_info->power_mw, real_time_info->total_wh
    );
    const size_t body_size = strlen(body_buffer);
    snprintf(header_buffer, HEADER_BUFFER_SIZE,
             "POST %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/plain\r\n"
             "\r\n",
             config->push_gateway_endpoint, config->push_gateway_host, body_size
    );

    // printf("Buffer: %s%s\n\n", header_buffer, body_buffer);

    communicate_with_push_gateway(config, body_buffer, header_buffer, body_size, strlen(header_buffer));


}


void delete_metrics(const struct config *const config) {

    char header_buffer[HEADER_BUFFER_SIZE];
    snprintf(header_buffer, HEADER_BUFFER_SIZE,
             "DELETE %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Content-Length: 0\r\n"
             "\r\n",
             config->push_gateway_endpoint, config->push_gateway_host
    );

    communicate_with_push_gateway(config, NULL, header_buffer, 0, strlen(header_buffer));
}
