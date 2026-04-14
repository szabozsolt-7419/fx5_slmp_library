/**
 * @file fx5_client_utility.h
 * @brief Small shared helper functions used by fx5_client modules.
 */
#ifndef FX5_CLIENT_UTILITY_H
#define FX5_CLIENT_UTILITY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/** @brief Duplicate a C string using heap allocation. */
char *fx5_client_strdup(const char *text);

/** @brief Trim leading and trailing whitespace in-place and return the trimmed start pointer. */
char *fx5_client_trim(char *text);

/** @brief Remove trailing whitespace from a mutable string buffer. */
void fx5_client_trim_right(char *text);

/** @brief Skip leading whitespace and return the first non-space character. */
const char *fx5_client_skip_spaces(const char *text);

/** @brief Mutable variant of fx5_client_skip_spaces(). */
char *fx5_client_skip_spaces_mut(char *text);

/** @brief Parse the text "true" or "false". */
bool fx5_client_parse_bool(const char *text, bool *out_value);

/** @brief Parse a full string as a 16-bit unsigned integer. */
bool fx5_client_parse_uint16_text(const char *text, uint16_t *out_value);

/** @brief Parse a full string as a 32-bit unsigned integer. */
bool fx5_client_parse_uint32_text(const char *text, uint32_t *out_value);


/**
 * @brief Parse a 32-bit unsigned integer from a string cursor and advance it.
 *
 * Useful for token-by-token command parsing.
 */
bool fx5_client_parse_uint32_cursor(const char **text, uint32_t *out_value);

#endif /* FX5_CLIENT_UTILITY_H */