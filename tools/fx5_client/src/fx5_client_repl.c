#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fx5_client_repl.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "fx5_client_commands.h"
#include "fx5_client_parser.h"
#include "fx5_client_utility.h"



static void fx5_client_print_help(void)
{
    printf("Available commands:\n");
    printf("  help\n");
    printf("  quit\n");
    printf("  exit\n");
    printf("  trace on\n");
    printf("  trace off\n");
    printf("  header 3e\n");
    printf("  header 4e\n");
    printf("  get D100\n");
    printf("  get D100-110\n");
    printf("  get M51-60\n");
    printf("  set D100=55\n");
    printf("  set D88-92=33,34,35,36,37\n");
    printf("  set M51=true\n");
    printf("  set M51-60=true\n");
    printf("  set M51-56=true,false,true,false,true,true\n");
    printf("\n");
    printf("Supported devices:\n");
    printf("  bit       : M, X, Y, L, F, S, B\n");
    printf("  word      : D, W\n");
    printf("  read-only : SM, SD, SB, SW\n");
    printf("  addressing: X/Y octal, B/W/SB/SW hex, others decimal\n");
}

static bool fx5_client_execute_repl_command(
    fx5_client_app_t *app,
    const fx5_client_command_t *cmd
    )
{
    if (app == NULL || cmd == NULL) {
        return false;
    }

    switch (cmd->type) {
        case FX5_CLIENT_CMD_HELP:
            fx5_client_print_help();
            return true;

        case FX5_CLIENT_CMD_TRACE:
            app->config->trace_enabled = cmd->trace_enabled;
            printf("trace: %s\n", app->config->trace_enabled ? "on" : "off");
            return true;

        case FX5_CLIENT_CMD_HEADER:
            app->config->network_settings.header_type = cmd->header_type;
            printf("header: %s\n",
                   app->config->network_settings.header_type == FX5_3E_HEADER ? "3E" : "4E");
            return true;

        case FX5_CLIENT_CMD_GET:
        case FX5_CLIENT_CMD_SET:
            return fx5_client_app_execute_command(app, cmd);

        default:
            printf("Unsupported command.\n");
            return false;
    }
}

int fx5_client_repl_run(fx5_client_app_t *app)
{
    char line_buffer[512];

    if (app == NULL) {
        fprintf(stderr, "REPL error: app is NULL\n");
        return 2;
    }

    printf("Interactive mode.\n");
    printf("Type 'help' for commands, 'quit' to exit.\n");

    for (;;) {
        char *line = NULL;
        fx5_client_command_t cmd;

        printf("fx5> ");
        fflush(stdout);

        if (fgets(line_buffer, sizeof(line_buffer), stdin) == NULL) {
            printf("\n");
            break;
        }

        fx5_client_trim_right(line_buffer);
        line = fx5_client_skip_spaces_mut(line_buffer);

        if (line == NULL || *line == '\0') {
            continue;
        }

        if (!fx5_client_parse_command(line, &cmd)) {
            printf("Syntax error.\n");
            continue;
        }

        if (cmd.type == FX5_CLIENT_CMD_QUIT) {
            return 0;
        }

        if (!fx5_client_execute_repl_command(app, &cmd)) {
            continue;
        }
    }

    return 0;
}

int fx5_client_script_run(fx5_client_app_t *app, const char *path)
{
    FILE *script = NULL;
    char line_buffer[512];
    unsigned line_no = 0u;
    int rc = 0;

    if (app == NULL || path == NULL) {
        fprintf(stderr, "Script error: invalid argument\n");
        return 2;
    }

    script = fopen(path, "r");
    if (script == NULL) {
        fprintf(stderr, "ERR: failed to open script: %s\n", path);
        return 2;
    }

    while (fgets(line_buffer, sizeof(line_buffer), script) != NULL) {
        char *line = NULL;
        fx5_client_command_t cmd;

        line_no++;
        fx5_client_trim_right(line_buffer);
        line = fx5_client_skip_spaces_mut(line_buffer);

        if (line == NULL || *line == '\0' || *line == '#') {
            continue;
        }

        if (!fx5_client_parse_command(line, &cmd)) {
            fprintf(stderr, "ERR: script syntax error at line %u\n", line_no);
            rc = 1;
            break;
        }

        if (cmd.type == FX5_CLIENT_CMD_QUIT) {
            break;
        }

        if (!fx5_client_execute_repl_command(app, &cmd)) {
            fprintf(stderr, "ERR: script command failed at line %u\n", line_no);
            rc = 1;
            break;
        }
    }

    if (ferror(script)) {
        fprintf(stderr, "ERR: failed to read script: %s\n", path);
        rc = 2;
    }

    fclose(script);
    return rc;
}
