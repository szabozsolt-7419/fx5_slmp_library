/**
 * @file fx5_client_commands.h
 * @brief Parsed command model and PLC command execution helpers for fx5_client.
 */
#ifndef FX5_CLIENT_COMMANDS_H
#define FX5_CLIENT_COMMANDS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <fx5/fx5.h>

#include "fx5_client_config.h"


#define FX5_CLIENT_MAX_VALUES 256u

/**
 * @brief Supported command types in the interactive client.
 */
typedef enum
{
    FX5_CLIENT_CMD_INVALID = 0,
    FX5_CLIENT_CMD_GET,
    FX5_CLIENT_CMD_SET,
    FX5_CLIENT_CMD_HELP,
    FX5_CLIENT_CMD_TRACE,
    FX5_CLIENT_CMD_HEADER,
    FX5_CLIENT_CMD_QUIT
} fx5_client_command_type_t;

/**
 * @brief Parsed representation of a client command.
 *
 * This structure stores the normalized form of a REPL command, including
 * device type, address range, write values, and runtime control flags.
 */
typedef struct fx5_client_command
{
    fx5_client_command_type_t type;
    fx5_device_t device;
    uint32_t start_address;
    uint16_t count;

    bool bool_value;
    bool has_scalar_value;

    uint16_t values[FX5_CLIENT_MAX_VALUES];
    uint16_t value_count;

    bool trace_enabled;
    fx5_header_t header_type;
} fx5_client_command_t;


/**
 * @brief Check whether a command requires PLC communication.
 *
 * @param[in] cmd Parsed command.
 * @return true if the command performs PLC I/O, false otherwise.
 */
bool fx5_client_command_needs_plc_io(const fx5_client_command_t *cmd);


/**
 * @brief Apply a parsed command to an fx5 protocol context.
 *
 * Resets the protocol context, applies the configured network settings,
 * sets up the requested operation, and fills write values when needed.
 *
 * @param[in,out] ctx fx5 protocol context.
 * @param[in] cmd Parsed command.
 * @param[in] config Active client configuration.
 * @return fx5 status code.
 */
fx5_status_t fx5_client_apply_command_to_context(
    fx5_context_t *ctx,
    const fx5_client_command_t *cmd,
    const fx5_client_config_t *config
    );

/**
 * @brief Print a formatted PLC response to the given stream.
 *
 * @param[in,out] stream Output stream.
 * @param[in] cmd Command that produced the response.
 * @param[in] ctx fx5 protocol context containing the parsed response.
 */
void fx5_client_print_response(
    FILE *stream,
    const fx5_client_command_t *cmd,
    const fx5_context_t *ctx
    );

#endif /* FX5_CLIENT_COMMANDS_H */