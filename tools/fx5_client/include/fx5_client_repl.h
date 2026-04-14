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

#endif /* FX5_CLIENT_REPL_H */