#ifndef TPLINK_HS110_METRICS_CLIENT_METRICS_H
#define TPLINK_HS110_METRICS_CLIENT_METRICS_H

#include "config.h"

void update_metrics(const struct config *vars);

struct sys_info {
    char *alias;
    char *id;
    char *mac;
    double state;
    double on_time_seconds;
};

struct real_time_info {
    double voltage_mv;
    double current_ma;
    double power_mw;
    double total_wh;
};

#endif //TPLINK_HS110_METRICS_CLIENT_METRICS_H
