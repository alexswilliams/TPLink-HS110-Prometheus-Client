#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

static const unsigned long DEFAULT_POLL_TIME_MILLIS = 5000;
static const char *const DEFAULT_PORT = "9999";


int get_ul_in_range_with_default(const char *const name, unsigned long *const out, const long long min,
                                 const long long max, const unsigned long default_value) {
    char *value = getenv(name);

    if (value == NULL) {
        *out = default_value;
        return 0;
    }

    char *last_valid_character = NULL;
    long long val = strtoll(value, &last_valid_character, 10);

    if (last_valid_character == NULL || last_valid_character[0] != '\0') {
        fprintf(stderr, "%s was not an integer: %s\n", name, value);
        fflush(stderr);
        return 1;
    }
    if (val < min || val > max) {
        fprintf(stderr, "%s was outside the acceptable range (%llu..%llu): %s\n", name, min, max, value);
        fflush(stderr);
        return 1;
    }
    *out = val;
    return 0;
}

int get_non_empty_string(const char *const name, const char **const out) {
    char *value = getenv(name);

    if (value == NULL || value[0] == '\0') {
        fprintf(stderr, "%s was not set.\n", name);
        fflush(stderr);
        return 1;
    }

    *out = value;
    return 0;
}

int get_string_with_default(const char *const name, const char **const out, const char *const default_value) {
    char *value = getenv(name);

    if (value == NULL) {
        *out = default_value;
        return 0;
    }
    *out = value;
    return 0;
}


int get_env_vars(struct config *const config) {
    int errors = 0;

    errors += get_ul_in_range_with_default("POLL_TIME_MILLIS", &config->poll_time_millis, 0, UINT32_MAX,
                                           DEFAULT_POLL_TIME_MILLIS);
    errors += get_non_empty_string("TPLINK_HOST", &config->hostname);
    errors += get_string_with_default("TPLINK_PORT", &config->port, DEFAULT_PORT);
    errors += get_non_empty_string("PUSH_GW_HOST", &config->push_gateway_host);
    errors += get_non_empty_string("PUSH_GW_PORT", &config->push_gateway_port);
    errors += get_non_empty_string("PUSH_GW_ENDPOINT", &config->push_gateway_endpoint);

    if (errors == 0) {
        printf("Polling TPLink HS110 with the following configuration: \n"
               " • Poll Frequency: %ld ms\n"
               " • Host: %s\n"
               " • Port: %s\n"
               " • Push Gateway URI: http://%s:%s%s\n",
               config->poll_time_millis, config->hostname, config->port, config->push_gateway_host,
               config->push_gateway_port, config->push_gateway_endpoint);
        fflush(stdout);
        return 0;
    }

    return errors;
}
