#ifndef TPLINK_HS110_METRICS_CLIENT_PROMETHEUS_H
#define TPLINK_HS110_METRICS_CLIENT_PROMETHEUS_H

#include "config.h"
#include "metrics.h"

void registerNewMetrics(const struct config *config, const char *tags, const struct sysInfo *sysInfo,
                        const struct realTimeInfo *realTimeInfo);

void deleteMetrics(const struct config *config);

#endif //TPLINK_HS110_METRICS_CLIENT_PROMETHEUS_H
