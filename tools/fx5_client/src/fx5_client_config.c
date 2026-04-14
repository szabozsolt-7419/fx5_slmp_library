#include "fx5_client_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fx5_client_utility.h"



static void apply_kv(const char *key, const char *value, fx5_client_config_t *config)
{
    uint16_t tmp16;
    if (strcmp(key, "host") == 0) {
        fx5_client_config_set_host(config,value);
    }
    else if (strcmp(key, "port") == 0) {
        fx5_client_parse_uint16_text(value, &config->port);
    }
    else if (strcmp(key, "trace") == 0) {
        fx5_client_parse_bool(value, &config->trace_enabled);
    }

    else if (strcmp(key, "recv_timeout_ms") == 0) {
        fx5_client_parse_uint32_text(value, &config->recv_timeout_ms);
    } else if (strcmp(key, "network_no") == 0) {
      fx5_client_parse_uint16_text(value, &tmp16);
      config->network_settings.network_no = (uint8_t)tmp16;
    } else if (strcmp(key, "pc_no") == 0) {
      fx5_client_parse_uint16_text(value, &tmp16);
      config->network_settings.pc_no = (uint8_t)tmp16;
    } else if (strcmp(key, "module_io_no") == 0) {
      fx5_client_parse_uint16_text(value, &config->network_settings.module_io_no);
    } else if (strcmp(key, "module_station_no") == 0) {
      fx5_client_parse_uint16_text(value, &tmp16);
      config->network_settings.module_station_no = (uint8_t)tmp16;
    } else if (strcmp(key, "monitoring_timer") == 0) {
      fx5_client_parse_uint16_text(value, &config->network_settings.monitoring_timer);
    } else if (strcmp(key, "header") == 0) {
      if (strcmp(value, "3e") == 0) {
        config->network_settings.header_type = FX5_3E_HEADER;
      } else if (strcmp(value, "4e") == 0) {
        config->network_settings.header_type = FX5_4E_HEADER;
      }
    }
}

void fx5_client_config_init(fx5_client_config_t *config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->recv_timeout_ms = 1000u;

    fx5_network_settings_init(&config->network_settings);
    config->network_settings.header_type = FX5_4E_HEADER;
}

void fx5_client_config_free(fx5_client_config_t *config)
{
    if (config == NULL) {
        return;
    }

    free(config->host);
    config->host = NULL;
}

bool fx5_client_config_set_host(fx5_client_config_t *config, const char *host)
{
    char *copy;

    if (config == NULL || host == NULL) {
        return false;
    }

    copy = fx5_client_strdup(host);
    if (copy == NULL) {
        return false;
    }

    free(config->host);
    config->host = copy;
    return true;
}

bool fx5_client_load_ini_file(const char *path, fx5_client_config_t *config)
{
    FILE *f;
    char line[256];

    if (path == NULL || config == NULL) {
        return false;
    }

    f = fopen(path, "r");
    if (f == NULL) {
        return false;
    }

    while (fgets(line, sizeof(line), f)) {
        char *p = fx5_client_trim(line);

        if (*p == '#' || *p == '\0') {
            continue;
        }

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';

        char *key = fx5_client_trim(p);
        char *val = fx5_client_trim(eq + 1);

        apply_kv(key, val, config);
    }

    fclose(f);
    return true;
}