#ifndef TPLINK_HS110_METRICS_CLIENT_CONNECTION_H
#define TPLINK_HS110_METRICS_CLIENT_CONNECTION_H

int openConnection(const char *hostname, const char *port);

void closeConnection(int connection);

#endif //TPLINK_HS110_METRICS_CLIENT_CONNECTION_H
