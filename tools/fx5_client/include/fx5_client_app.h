/**
 * @file fx5_client_app.h
 * @brief Application-level state and execution helpers for the fx5_client tool.
 *
 * This module owns the runtime objects required by the interactive client,
 * including the TCP transport socket and the fx5 protocol context.
 */
#ifndef FX5_CLIENT_APP_H
#define FX5_CLIENT_APP_H

#include <stdbool.h>

#include <fx5/fx5.h>


#include "fx5_client_transport.h"
#include "fx5_client_config.h"

typedef struct fx5_client_command fx5_client_command_t;

/**
 * @brief Runtime state of the fx5_client application.
 *
 * The application keeps a reference to the shared client configuration and
 * owns the protocol context and transport socket used during execution.
 */
typedef struct
{
    fx5_client_config_t *config;
    fx5_context_t *ctx;
    fx5_socket_t *sock;
} fx5_client_app_t;


/**
 * @brief Initialize the client application state.
 *
 * Acquires an fx5 protocol context, initializes the transport layer,
 * opens a TCP socket, connects to the configured endpoint, and applies
 * the configured receive timeout.
 *
 * @param[out] app Application state to initialize.
 * @param[in,out] config Client configuration used by the application.
 * @return true on success, false on failure.
 */
bool fx5_client_app_init(fx5_client_app_t *app, fx5_client_config_t *config);

/**
 * @brief Shut down the client application state.
 *
 * Closes the TCP socket, releases the fx5 protocol context, and shuts down
 * the transport layer.
 *
 * @param[in,out] app Application state to shut down.
 */
void fx5_client_app_shutdown(fx5_client_app_t *app);

/**
 * @brief Run the interactive client application.
 *
 * Initializes the application, enters the REPL loop, and performs shutdown
 * before returning.
 *
 * @param[in,out] config Client configuration used during execution.
 * @return Process-style exit code.
 */
int fx5_client_app_run(fx5_client_config_t *config);

/**
 * @brief Execute a parsed REPL command.
 *
 * For PLC I/O commands, this function builds the SLMP request, sends it
 * over TCP, receives and parses the response, and prints the formatted result.
 *
 * @param[in,out] app Application state.
 * @param[in] cmd Parsed command to execute.
 * @return true on success, false on failure.
 */
bool fx5_client_app_execute_command(
    fx5_client_app_t *app,
    const fx5_client_command_t *cmd
    );

#endif /* FX5_CLIENT_APP_H */