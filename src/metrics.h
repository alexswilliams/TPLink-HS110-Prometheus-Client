#ifndef TPLINK_HS110_METRICS_CLIENT_METRICS_H
#define TPLINK_HS110_METRICS_CLIENT_METRICS_H

#include "config.h"

void updateMetrics(const struct config *vars);

struct sysInfo {
    char *alias;
    char *id;
    char *mac;
    double state;
    double onTimeSeconds;
};

struct realTimeInfo {
    double voltageMv;
    double currentMa;
    double powerMw;
    double totalWh;
};

#endif //TPLINK_HS110_METRICS_CLIENT_METRICS_H
