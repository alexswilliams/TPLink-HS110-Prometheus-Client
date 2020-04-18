#ifndef TPLINK_HS110_METRICS_CLIENT_CONFIG_H
#define TPLINK_HS110_METRICS_CLIENT_CONFIG_H

struct config {
    unsigned long poll_time_millis;
    const char *hostname;
    const char *port;
    const char *push_gateway_host;
    const char *push_gateway_port;
    const char *push_gateway_endpoint;
};

int get_env_vars(struct config *config);

#endif //TPLINK_HS110_METRICS_CLIENT_CONFIG_H
