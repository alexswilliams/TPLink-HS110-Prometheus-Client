#include <cjson/cJSON.h>
#include <stdio.h>

#include "metrics.h"
#include "connection.h"
#include "device.h"
#include "prometheus.h"

static const char *const GET_SYSINFO = "{\"system\":{\"get_sysinfo\":null}}";
static const char *const GET_REALTIME = "{\"emeter\":{\"get_realtime\":{}}}";

static const size_t TAG_BUFFER_LENGTH = 1024;


const cJSON *get_sub_object(const cJSON *const object, const char *const name) {
    cJSON *sub_object = cJSON_GetObjectItemCaseSensitive(object, name);
    if (sub_object == NULL || !cJSON_IsObject(sub_object)) {
        fprintf(stderr, "The JSON response did not contain a '%s' key with an object value.\n", name);
        fflush(stderr);
        return NULL;
    }
    return sub_object;
}

int get_string_key(const cJSON *const object, const char *const name, char **const out) {
    cJSON *sub_object = cJSON_GetObjectItemCaseSensitive(object, name);
    if (sub_object == NULL || !cJSON_IsString(sub_object)) {
        fprintf(stderr, "The JSON response did not contain a '%s' key with a string value.\n", name);
        fflush(stderr);
        return 1;
    }
    *out = sub_object->valuestring;
    return 0;
}

int get_number_key(const cJSON *const object, const char *const name, double *const out) {
    cJSON *sub_object = cJSON_GetObjectItemCaseSensitive(object, name);
    if (sub_object == NULL || !cJSON_IsNumber(sub_object)) {
        fprintf(stderr, "The JSON response did not contain a '%s' key with a numeric value.\n", name);
        fflush(stderr);
        return 1;
    }
    *out = sub_object->valuedouble;
    return 0;
}


int extract_device_info(const cJSON *const sys_info_json, struct sys_info *const out) {
    if (!cJSON_IsObject(sys_info_json)) {
        fprintf(stderr, "The Sys Info JSON response was not a JSON object.\n");
        fflush(stderr);
        return 1;
    }

    const cJSON *const system_object = get_sub_object(sys_info_json, "system");
    if (system_object == NULL) return 1;

    const cJSON *const get_sysinfo_object = get_sub_object(system_object, "get_sysinfo");
    if (get_sysinfo_object == NULL) return 1;


    if (get_string_key(get_sysinfo_object, "alias", &out->alias) != 0) return 1;
    if (get_string_key(get_sysinfo_object, "deviceId", &out->id) != 0) return 1;
    if (get_string_key(get_sysinfo_object, "mac", &out->mac) != 0) return 1;
    if (get_number_key(get_sysinfo_object, "relay_state", &out->state) != 0) return 1;
    if (get_number_key(get_sysinfo_object, "on_time", &out->on_time_seconds) != 0) return 1;

    return 0;
}

int extract_real_time_info(const cJSON *const real_time_info_json, struct real_time_info *const out) {
    if (!cJSON_IsObject(real_time_info_json)) {
        fprintf(stderr, "The Real Time Info JSON response was not a JSON object.\n");
        fflush(stderr);
        return 1;
    }

    const cJSON *const emeter_object = get_sub_object(real_time_info_json, "emeter");
    if (emeter_object == NULL) return 1;

    const cJSON *const get_realtime_object = get_sub_object(emeter_object, "get_realtime");
    if (get_realtime_object == NULL) return 1;


    if (get_number_key(get_realtime_object, "voltage_mv", &out->voltage_mv) != 0) return 1;
    if (get_number_key(get_realtime_object, "current_ma", &out->current_ma) != 0) return 1;
    if (get_number_key(get_realtime_object, "power_mw", &out->power_mw) != 0) return 1;
    if (get_number_key(get_realtime_object, "total_wh", &out->total_wh) != 0) return 1;

    return 0;
}


void cleanup(const int connection, cJSON *const sys_info_json, cJSON *const real_time_info_json) {
    if (connection != -1) close_connection(connection);
    if (sys_info_json != NULL) cJSON_Delete(sys_info_json);
    if (real_time_info_json != NULL) cJSON_Delete(real_time_info_json);
}

void record_failed_and_cleanup(const int connection, cJSON *const sys_info_json, cJSON *const real_time_info_json,
                               const struct config *const vars) {
    cleanup(connection, sys_info_json, real_time_info_json);
    delete_metrics(vars);
}


void update_metrics(const struct config *const vars) {
    cJSON *sys_info_json = NULL;
    cJSON *real_time_info_json = NULL;

    const int connection = open_connection(vars->hostname, vars->port);
    if (connection == -1) {
        fprintf(stderr, "Abandoning connection attempts to %s:%s.\n", vars->hostname, vars->port);
        fflush(stderr);
        record_failed_and_cleanup(connection, sys_info_json, real_time_info_json, vars);
        return;
    }

    int sys_info_query_result = query_device(connection, GET_SYSINFO, &sys_info_json);
    if (sys_info_query_result != 0 || sys_info_json == NULL) {
        record_failed_and_cleanup(connection, sys_info_json, real_time_info_json, vars);
        return;
    }

    struct sys_info sys_info;
    int sys_info_extract_result = extract_device_info(sys_info_json, &sys_info);
    if (sys_info_extract_result != 0) {
        record_failed_and_cleanup(connection, sys_info_json, real_time_info_json, vars);
        return;
    }


    int real_time_info_query_result = query_device(connection, GET_REALTIME, &real_time_info_json);
    if (real_time_info_query_result != 0 || real_time_info_json == NULL) {
        record_failed_and_cleanup(connection, sys_info_json, real_time_info_json, vars);
        return;
    }

    struct real_time_info real_time_info;
    int real_time_info_result = extract_real_time_info(real_time_info_json, &real_time_info);
    if (real_time_info_result != 0) {
        record_failed_and_cleanup(connection, sys_info_json, real_time_info_json, vars);
        return;
    }

    char tags[TAG_BUFFER_LENGTH];
    snprintf(tags, TAG_BUFFER_LENGTH, "alias=\"%s\",id=\"%s\",mac=\"%s\"", sys_info.alias, sys_info.id, sys_info.mac);

    cleanup(connection, sys_info_json, real_time_info_json);

    register_new_metrics(vars, tags, &sys_info, &real_time_info);
}
