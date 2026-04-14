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
    printf("  get D100-D110\n");
    printf("  get M51-M60\n");
    printf("  set D100=55\n");
    printf("  set D88-D92=33,34,35,36,37\n");
    printf("  set M51=true\n");
    printf("  set M51-M60=true\n");
    printf("  set M51-M56=true,false,true,false,true,true\n");
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

        switch (cmd.type) {
            case FX5_CLIENT_CMD_QUIT:
                return 0;

            case FX5_CLIENT_CMD_HELP:
                fx5_client_print_help();
                continue;

            case FX5_CLIENT_CMD_TRACE:
                app->config->trace_enabled = cmd.trace_enabled;
                printf("trace: %s\n", app->config->trace_enabled ? "on" : "off");
                continue;

            case FX5_CLIENT_CMD_HEADER:
                app->config->network_settings.header_type = cmd.header_type;
                printf("header: %s\n",
                       app->config->network_settings.header_type == FX5_3E_HEADER ? "3E" : "4E");
                continue;

            case FX5_CLIENT_CMD_GET:
            case FX5_CLIENT_CMD_SET:
                if (!fx5_client_app_execute_command(
                        app,
                        &cmd
                        )) {
                    continue;
                }
                continue;

            default:
                printf("Unsupported command.\n");
                continue;
        }
    }

    return 0;
}