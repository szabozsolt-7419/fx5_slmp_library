#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "fx5_client_app.h"
#include "fx5_client_config.h"


#ifndef FX5_CLIENT_VERSION_STRING
#define FX5_CLIENT_VERSION_STRING "0.0.0"
#endif

static void fx5_client_print_version(void)
{
    printf("fx5_client %s\n", FX5_CLIENT_VERSION_STRING);
}

static void fx5_client_print_usage(const char *program_name)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s <host> <port> [--trace] [--header=3e|4e]\n"
            "  %s --help\n"
            "  %s --version\n"
            "\n"
            "Examples:\n"
            "  %s 192.168.0.10 5000\n"
            "  %s 192.168.0.10 5000 --trace\n"
            "  %s 192.168.0.10 5000 --header=3e\n",
            program_name,
            program_name,
            program_name,
            program_name,
            program_name,
            program_name
  );
}

static bool fx5_client_parse_port(const char *text, uint16_t *out_port)
{
    char *endptr = NULL;
    long value = 0;

    if (text == NULL || out_port == NULL) {
        return false;
    }

    value = strtol(text, &endptr, 10);
    if (*text == '\0' || endptr == NULL || *endptr != '\0') {
        return false;
    }

    if (value < 1 || value > 65535) {
        return false;
    }

    *out_port = (uint16_t)value;
    return true;
}

static bool fx5_client_parse_optional_arg(const char *arg, fx5_client_config_t *config)
{
    if (arg == NULL || config == NULL) {
        return false;
    }

    if (strcmp(arg, "--trace") == 0) {
        config->trace_enabled = true;
        return true;
    }

    if (strcmp(arg, "--header=3e") == 0) {
        config->network_settings.header_type = FX5_3E_HEADER;
        return true;
    }

    if (strcmp(arg, "--header=4e") == 0) {
        config->network_settings.header_type = FX5_4E_HEADER;
        return true;
    }

    return false;
}

int main(int argc, char **argv)
{
    fx5_client_config_t config;
    uint16_t port = 0u;
    int i = 0;

    if (argc == 2) {
        if (strcmp(argv[1], "--version") == 0) {
            fx5_client_print_version();
            return 0;
        }

        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            fx5_client_print_usage(argv[0]);
            return 0;
        }
    }

    if (argc < 3) {
        fx5_client_print_usage(argv[0]);
        return 1;
    }

    if (!fx5_client_parse_port(argv[2], &port)) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        fx5_client_print_usage(argv[0]);
        return 1;
    }

    fx5_client_config_init(&config);
    fx5_client_load_ini_file("fx5_client.ini", &config);


    if (!fx5_client_config_set_host(&config, argv[1])) {
        fprintf(stderr, "Failed to set host\n");
        fx5_client_config_free(&config);
        return 1;
    }


    config.port = port;

    for (i = 3; i < argc; ++i) {
        if (!fx5_client_parse_optional_arg(argv[i], &config)) {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fx5_client_print_usage(argv[0]);
            fx5_client_config_free(&config);
            return 1;
        }
    }

    int rc = fx5_client_app_run(&config);
    fx5_client_config_free(&config);
    return rc;
}