#ifndef TPLINK_HS110_METRICS_CLIENT_CONFIG_H
#define TPLINK_HS110_METRICS_CLIENT_CONFIG_H

struct config {
    long pollTimeMillis;
    const char *hostname;
    const char *port;
    const char *pushGatewayHost;
    const char *pushGatewayPort;
    const char *pushGatewayEndpoint;
};

int getEnvVars(struct config *config);

#endif //TPLINK_HS110_METRICS_CLIENT_CONFIG_H
