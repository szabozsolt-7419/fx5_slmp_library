#include <stdio.h>
#include <string.h>

#include "fx5_client_app.h"
#include "fx5_client_repl.h"
#include "fx5_client_commands.h"

static void fx5_client_hexdump(FILE *stream, const char *prefix, const uint8_t *data, uint16_t size)
{
    uint16_t i = 0u;

    if (stream == NULL || prefix == NULL || data == NULL) {
        return;
    }

    fprintf(stream, "%s", prefix);
    for (i = 0u; i < size; ++i) {
        fprintf(stream, "%s%02X", (i == 0u) ? "" : " ", (unsigned)data[i]);
    }
    fprintf(stream, "\n");
}

static bool fx5_client_receive_response(fx5_client_app_t *app)
{
    uint8_t buffer[512];

    for (;;) {
        size_t received_size = 0u;
        fx5_transport_recv_status_t recv_status =
            fx5_socket_recv_some(app->sock, buffer, sizeof(buffer), &received_size);

        if (recv_status == FX5_TRANSPORT_RECV_TIMEOUT) {
            fprintf(stderr, "ERR: recv timeout\n");
            return false;
        }

        if (recv_status == FX5_TRANSPORT_RECV_CLOSED) {
            fprintf(stderr, "ERR: connection closed\n");
            return false;
        }

        if (recv_status == FX5_TRANSPORT_RECV_ERROR) {
            fprintf(stderr, "ERR: recv failed\n");
            return false;
        }

        if (app->config->trace_enabled) {
            fx5_client_hexdump(stdout, "RX: ", buffer, (uint16_t)received_size);
        }

        {
            fx5_status_t st =
                fx5_feed_response_bytes(app->ctx, buffer, (uint16_t)received_size);
            if (st != FX5_ST_OK) {
                fprintf(stderr, "ERR: feed response failed (%d)\n", (int)st);
                return false;
            }
        }

        {
            fx5_status_t st = fx5_parse_response(app->ctx);

            if (st == FX5_ST_ERR_NEED_MORE) {
                continue;
            }

            if (st != FX5_ST_OK) {
                fprintf(stderr, "ERR: parse response failed (%d)\n", (int)st);
                return false;
            }
        }

        return true;
    }
}


bool fx5_client_app_init(fx5_client_app_t *app, fx5_client_config_t *config)
{
    fx5_status_t st;

    if (app == NULL || config == NULL) {
        return false;
    }

    memset(app, 0, sizeof(*app));
    app->config = config;

    if (!fx5_transport_global_init()) {
      return false;
    }

    st = fx5_acquire(&app->ctx);
    if (st != FX5_ST_OK) {
        app->ctx = NULL;
        return false;
    }

    app->sock = fx5_socket_open();
    if (app->sock == NULL) {
      return false;
    }


    if (!fx5_socket_connect(app->sock,config->host,config->port)) {
      fprintf(stderr,"ERR: connect failed\n");
      return false;
    }

    if (!fx5_socket_set_recv_timeout_ms(app->sock, config->recv_timeout_ms)) {
      fprintf(stderr, "WARN: failed to set recv timeout\n");
    }

    return true;
}

void fx5_client_app_shutdown(fx5_client_app_t *app)
{
    if (app == NULL) {
        return;
    }

    if (app->sock != NULL) {
      fx5_socket_close(app->sock);
    }

    if (app->ctx != NULL) {
        fx5_release(&app->ctx);
    }

    fx5_transport_global_shutdown();
}


bool fx5_client_app_execute_command(
    fx5_client_app_t *app,
    const fx5_client_command_t *cmd
    )
{
    uint8_t request_buf[FX5_MAX_REQUEST_SIZE];
    uint16_t request_size = 0u;
    fx5_status_t st;

    if (app == NULL || cmd == NULL) {
        return false;
    }

    st = fx5_client_apply_command_to_context(
        app->ctx,
        cmd,
        app->config
    );
    if (st != FX5_ST_OK) {
        fprintf(stderr, "ERR: apply failed (%d)\n", (int)st);
        return false;
    }

    st = fx5_build_request(
        app->ctx,
        request_buf,
        sizeof(request_buf),
        &request_size
    );
    if (st != FX5_ST_OK) {
        fprintf(stderr, "ERR: build failed (%d)\n", (int)st);
        return false;
    }

    if (app->config->trace_enabled) {
        fx5_client_hexdump(stdout, "TX: ", request_buf, request_size);
    }

    if (!fx5_socket_send_all(app->sock, request_buf, request_size)) {
        fprintf(stderr, "ERR: send failed\n");
        return false;
    }

    if (!fx5_client_receive_response(app)) {
        return false;
    }

    fx5_client_print_response(stdout, cmd, app->ctx);
    return true;
}

int fx5_client_app_run(fx5_client_config_t *config)
{
    fx5_client_app_t app;

    if (config == NULL) {
        fprintf(stderr, "Internal error: config is NULL\n");
        return 2;
    }

    if (!fx5_client_app_init(&app, config)) {
        fprintf(stderr, "ERR: failed to initialize client app\n");
        return 2;
    }

    printf("fx5_client starting...\n");
    printf("  host   : %s\n", config->host != NULL ? config->host : "(null)");
    printf("  port   : %u\n", (unsigned)config->port);
    printf("  trace  : %s\n", config->trace_enabled ? "on" : "off");
    printf("  header : %s\n", config->network_settings.header_type == FX5_3E_HEADER ? "3E" : "4E");

    {
        const int rc = fx5_client_repl_run(&app);
        fx5_client_app_shutdown(&app);
        return rc;
    }
}

