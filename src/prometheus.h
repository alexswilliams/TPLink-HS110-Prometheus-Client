#ifndef TPLINK_HS110_METRICS_CLIENT_PROMETHEUS_H
#define TPLINK_HS110_METRICS_CLIENT_PROMETHEUS_H

#include "config.h"
#include "metrics.h"

void register_new_metrics(const struct config *config, const char *tags, const struct sys_info *sys_info,
                          const struct real_time_info *real_time_info);

void delete_metrics(const struct config *config);

#endif //TPLINK_HS110_METRICS_CLIENT_PROMETHEUS_H
