/**
 * @file fx5_client_parser.h
 * @brief REPL command parser for fx5_client.
 */
#ifndef FX5_CLIENT_PARSER_H
#define FX5_CLIENT_PARSER_H

#include <stdbool.h>

#include "fx5_client_commands.h"

/**
 * @brief Parse a single REPL input line into a command structure.
 *
 * @param[in] line Input line to parse.
 * @param[out] out_cmd Parsed command structure.
 * @return true on success, false on syntax error.
 */
bool fx5_client_parse_command(const char *line, fx5_client_command_t *out_cmd);

#endif /* FX5_CLIENT_PARSER_H */