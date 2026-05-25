/**
 * @file fx5_client_repl.h
 * @brief Interactive read-eval-print loop for fx5_client.
 */
#ifndef FX5_CLIENT_REPL_H
#define FX5_CLIENT_REPL_H

#include "fx5_client_app.h"

/**
 * @brief Run the interactive command loop.
 *
 * @param[in,out] app Initialized client application state.
 * @return Process-style exit code.
 */
int fx5_client_repl_run(fx5_client_app_t *app);

/**
 * @brief Run commands from a script file.
 *
 * Empty lines and lines starting with '#' after leading whitespace are ignored.
 * Commands are parsed and executed with the same semantics as the interactive
 * REPL, but no prompt or startup text is printed by this function.
 *
 * @param[in,out] app Initialized client application state.
 * @param[in] path Script file path.
 * @return Process-style exit code.
 */
int fx5_client_script_run(fx5_client_app_t *app, const char *path);

#endif /* FX5_CLIENT_REPL_H */
