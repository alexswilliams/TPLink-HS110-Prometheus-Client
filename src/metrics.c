#include <cjson/cJSON.h>
#include <stdio.h>

#include "metrics.h"
#include "connection.h"
#include "device.h"
#include "prometheus.h"

static const char *const getSysinfo = "{\"system\":{\"get_sysinfo\":null}}";
static const char *const getRealtime = "{\"emeter\":{\"get_realtime\":{}}}";

static const size_t tagBufferLength = 1024;


const cJSON *getSubObject(const cJSON *const object, const char *const name) {
    cJSON *subObject = cJSON_GetObjectItemCaseSensitive(object, name);
    if (subObject == NULL || !cJSON_IsObject(subObject)) {
        fprintf(stderr, "The JSON response did not contain a '%s' key with an object value.\n", name);
        fflush(stderr);
        return NULL;
    }
    return subObject;
}

int getStringKey(const cJSON *const object, const char *const name, char **const out) {
    cJSON *subObject = cJSON_GetObjectItemCaseSensitive(object, name);
    if (subObject == NULL || !cJSON_IsString(subObject)) {
        fprintf(stderr, "The JSON response did not contain a '%s' key with a string value.\n", name);
        fflush(stderr);
        return 1;
    }
    *out = subObject->valuestring;
    return 0;
}

int getNumberKey(const cJSON *const object, const char *const name, double *const out) {
    cJSON *subObject = cJSON_GetObjectItemCaseSensitive(object, name);
    if (subObject == NULL || !cJSON_IsNumber(subObject)) {
        fprintf(stderr, "The JSON response did not contain a '%s' key with a numeric value.\n", name);
        fflush(stderr);
        return 1;
    }
    *out = subObject->valuedouble;
    return 0;
}


int extractDeviceInfo(const cJSON *const sysInfoJson, struct sysInfo *const out) {
    if (!cJSON_IsObject(sysInfoJson)) {
        fprintf(stderr, "The Sys Info JSON response was not a JSON object.\n");
        fflush(stderr);
        return 1;
    }

    const cJSON *const systemObject = getSubObject(sysInfoJson, "system");
    if (systemObject == NULL) return 1;

    const cJSON *const getSysinfoObject = getSubObject(systemObject, "get_sysinfo");
    if (getSysinfoObject == NULL) return 1;


    if (getStringKey(getSysinfoObject, "alias", &out->alias) != 0) return 1;
    if (getStringKey(getSysinfoObject, "deviceId", &out->id) != 0) return 1;
    if (getStringKey(getSysinfoObject, "mac", &out->mac) != 0) return 1;
    if (getNumberKey(getSysinfoObject, "relay_state", &out->state) != 0) return 1;
    if (getNumberKey(getSysinfoObject, "on_time", &out->onTimeSeconds) != 0) return 1;

    return 0;
}

int extractRealTimeInfo(const cJSON *const realTimeInfoJson, struct realTimeInfo *const out) {
    if (!cJSON_IsObject(realTimeInfoJson)) {
        fprintf(stderr, "The Real Time Info JSON response was not a JSON object.\n");
        fflush(stderr);
        return 1;
    }

    const cJSON *const emeterObject = getSubObject(realTimeInfoJson, "emeter");
    if (emeterObject == NULL) return 1;

    const cJSON *const getRealtimeObject = getSubObject(emeterObject, "get_realtime");
    if (getRealtimeObject == NULL) return 1;


    if (getNumberKey(getRealtimeObject, "voltageMv", &out->voltageMv) != 0) return 1;
    if (getNumberKey(getRealtimeObject, "currentMa", &out->currentMa) != 0) return 1;
    if (getNumberKey(getRealtimeObject, "powerMw", &out->powerMw) != 0) return 1;
    if (getNumberKey(getRealtimeObject, "totalWh", &out->totalWh) != 0) return 1;

    return 0;
}


void cleanup(const int connection, cJSON *const sysInfoJson, cJSON *const realTimeInfoJson) {
    if (connection != -1) closeConnection(connection);
    if (sysInfoJson != NULL) cJSON_Delete(sysInfoJson);
    if (realTimeInfoJson != NULL) cJSON_Delete(realTimeInfoJson);
}

void recordFailedAndCleanup(const int connection, cJSON *const sysInfoJson, cJSON *const realTimeInfoJson,
                            const struct config *const vars) {
    cleanup(connection, sysInfoJson, realTimeInfoJson);
    deleteMetrics(vars);
}


void updateMetrics(const struct config *vars) {
    cJSON *sysInfoJson = NULL;
    cJSON *realTimeInfoJson = NULL;

    const int connection = openConnection(vars->hostname, vars->port);
    if (connection == -1) {
        fprintf(stderr, "Abandoning connection attempts to %s:%s.\n", vars->hostname, vars->port);
        fflush(stderr);
        recordFailedAndCleanup(connection, sysInfoJson, realTimeInfoJson, vars);
        return;
    }

    int sysInfoQueryResult = queryDevice(connection, getSysinfo, &sysInfoJson);
    if (sysInfoQueryResult != 0 || sysInfoJson == NULL) {
        recordFailedAndCleanup(connection, sysInfoJson, realTimeInfoJson, vars);
        return;
    }

    struct sysInfo sysInfo;
    int sysInfoExtractResult = extractDeviceInfo(sysInfoJson, &sysInfo);
    if (sysInfoExtractResult != 0) {
        recordFailedAndCleanup(connection, sysInfoJson, realTimeInfoJson, vars);
        return;
    }


    int realTimeInfoQueryResult = queryDevice(connection, getRealtime, &realTimeInfoJson);
    if (realTimeInfoQueryResult != 0 || realTimeInfoJson == NULL) {
        recordFailedAndCleanup(connection, sysInfoJson, realTimeInfoJson, vars);
        return;
    }

    struct realTimeInfo realTimeInfo;
    int realTimeInfoResult = extractRealTimeInfo(realTimeInfoJson, &realTimeInfo);
    if (realTimeInfoResult != 0) {
        recordFailedAndCleanup(connection, sysInfoJson, realTimeInfoJson, vars);
        return;
    }

    char tags[tagBufferLength];
    snprintf(tags, tagBufferLength, "alias=\"%s\",id=\"%s\",mac=\"%s\"", sysInfo.alias, sysInfo.id, sysInfo.mac);

    cleanup(connection, sysInfoJson, realTimeInfoJson);

    registerNewMetrics(vars, tags, &sysInfo, &realTimeInfo);
}
