#ifndef TPLINK_HS110_METRICS_CLIENT_DEVICE_H
#define TPLINK_HS110_METRICS_CLIENT_DEVICE_H

#include <cjson/cJSON.h>

int query_device(int connection, const char *request, cJSON **out);

#endif //TPLINK_HS110_METRICS_CLIENT_DEVICE_H
