#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

static const long defaultPollTimeMillis = 5000;
static const char *const defaultPort = "9999";


int getLongInRangeWithDefault(const char *const name, long *const out, const long long min,
                              const long long max, const long defaultValue) {
    char *value = getenv(name);

    if (value == NULL) {
        *out = defaultValue;
        return 0;
    }

    char *lastValidCharacter = NULL;
    long long val = strtoll(value, &lastValidCharacter, 10);

    if (lastValidCharacter == NULL || lastValidCharacter[0] != '\0') {
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

int getNonEmptyString(const char *const name, const char **const out) {
    char *value = getenv(name);

    if (value == NULL || value[0] == '\0') {
        fprintf(stderr, "%s was not set.\n", name);
        fflush(stderr);
        return 1;
    }

    *out = value;
    return 0;
}

int getStringWithDefault(const char *const name, const char **const out, const char *const defaultValue) {
    char *value = getenv(name);

    if (value == NULL) {
        *out = defaultValue;
        return 0;
    }
    *out = value;
    return 0;
}


int getEnvVars(struct config *config) {
    int errors = 0;

    errors += getLongInRangeWithDefault("POLL_TIME_MILLIS", &config->pollTimeMillis, 0, UINT32_MAX,
                                        defaultPollTimeMillis);
    errors += getNonEmptyString("TPLINK_HOST", &config->hostname);
    errors += getStringWithDefault("TPLINK_PORT", &config->port, defaultPort);
    errors += getNonEmptyString("PUSH_GW_HOST", &config->pushGatewayHost);
    errors += getNonEmptyString("PUSH_GW_PORT", &config->pushGatewayPort);
    errors += getNonEmptyString("PUSH_GW_ENDPOINT", &config->pushGatewayEndpoint);

    if (errors == 0) {
        printf("Polling TPLink HS110 with the following configuration: \n"
               " • Poll Frequency: %ld ms\n"
               " • Host: %s\n"
               " • Port: %s\n"
               " • Push Gateway URI: http://%s:%s%s\n",
               config->pollTimeMillis, config->hostname, config->port, config->pushGatewayHost,
               config->pushGatewayPort, config->pushGatewayEndpoint);
        fflush(stdout);
        return 0;
    }

    return errors;
}
