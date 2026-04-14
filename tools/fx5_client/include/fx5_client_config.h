/**
 * @file fx5_client_config.h
 * @brief Configuration model and INI loading helpers for fx5_client.
 */
#ifndef FX5_CLIENT_CONFIG_H
#define FX5_CLIENT_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include <fx5/fx5.h>

/**
 * @brief Client configuration used by fx5_client.
 *
 * Stores connection settings, runtime tracing options, receive timeout,
 * and SLMP network header parameters.
 */
typedef struct
{
    char *host;
    uint16_t port;
    bool trace_enabled;    
    uint32_t recv_timeout_ms;
    fx5_network_settings_t network_settings;
} fx5_client_config_t;

/**
 * @brief Initialize a configuration structure with default values.
 *
 * @param[out] config Configuration structure to initialize.
 */
void fx5_client_config_init(fx5_client_config_t *config);

/**
 * @brief Release dynamically allocated resources held by a configuration.
 *
 * @param[in,out] config Configuration structure to clean up.
 */
void fx5_client_config_free(fx5_client_config_t *config);

/**
 * @brief Set the target host name or IP address.
 *
 * The previous host string is released and replaced with a newly allocated copy.
 *
 * @param[in,out] config Configuration structure.
 * @param[in] host Host name or IP address string.
 * @return true on success, false on failure.
 */
bool fx5_client_config_set_host(fx5_client_config_t *config, const char *host);

/**
 * @brief Load configuration values from an INI-style file.
 *
 * Existing configuration values are updated key-by-key. Unknown keys are ignored.
 *
 * @param[in] path Path to the configuration file.
 * @param[in,out] config Configuration structure to update.
 * @return true if the file was opened and processed, false otherwise.
 */
bool fx5_client_load_ini_file(
    const char *path,
    fx5_client_config_t *config
    );

#endif