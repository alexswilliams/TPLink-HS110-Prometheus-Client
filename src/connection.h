#ifndef TPLINK_HS110_METRICS_CLIENT_CONNECTION_H
#define TPLINK_HS110_METRICS_CLIENT_CONNECTION_H

int open_connection(const char *hostname, const char *port);

void close_connection(int connection);

#endif //TPLINK_HS110_METRICS_CLIENT_CONNECTION_H
