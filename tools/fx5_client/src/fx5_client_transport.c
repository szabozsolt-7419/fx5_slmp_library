#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fx5_client_transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET fx5_native_socket_t;
    #define FX5_INVALID_SOCKET INVALID_SOCKET
    #define FX5_SOCKET_CLOSE closesocket
#else
    #include <errno.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    typedef int fx5_native_socket_t;
    #define FX5_INVALID_SOCKET (-1)
    #define FX5_SOCKET_CLOSE close
#endif

struct fx5_socket
{
    fx5_native_socket_t handle;
};

bool fx5_transport_global_init(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    const int rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    return rc == 0;
#else
    return true;
#endif
}

void fx5_transport_global_shutdown(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

fx5_socket_t *fx5_socket_open(void)
{
    fx5_socket_t *sock = (fx5_socket_t *)calloc(1u, sizeof(*sock));
    if (sock == NULL) {
        return NULL;
    }

    sock->handle = FX5_INVALID_SOCKET;
    return sock;
}

void fx5_socket_close(fx5_socket_t *sock)
{
    if (sock == NULL) {
        return;
    }

    if (sock->handle != FX5_INVALID_SOCKET) {
        FX5_SOCKET_CLOSE(sock->handle);
        sock->handle = FX5_INVALID_SOCKET;
    }

    free(sock);
}

static bool fx5_socket_set_connected_handle(fx5_socket_t *sock, fx5_native_socket_t handle)
{
    if (sock == NULL) {
        return false;
    }

    if (sock->handle != FX5_INVALID_SOCKET) {
        FX5_SOCKET_CLOSE(sock->handle);
    }

    sock->handle = handle;
    return true;
}

bool fx5_socket_connect(fx5_socket_t *sock, const char *host, uint16_t port)
{
    char port_text[16];
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *it = NULL;
    int rc = 0;

    if (sock == NULL || host == NULL) {
        return false;
    }

    snprintf(port_text, sizeof(port_text), "%u", (unsigned)port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    rc = getaddrinfo(host, port_text, &hints, &result);
    if (rc != 0) {
        return false;
    }

    for (it = result; it != NULL; it = it->ai_next) {
        fx5_native_socket_t handle =
            (fx5_native_socket_t)socket(it->ai_family, it->ai_socktype, it->ai_protocol);

        if (handle == FX5_INVALID_SOCKET) {
            continue;
        }

        rc = connect(handle, it->ai_addr, (int)it->ai_addrlen);
        if (rc == 0) {
            freeaddrinfo(result);
            return fx5_socket_set_connected_handle(sock, handle);
        }

        FX5_SOCKET_CLOSE(handle);
    }

    freeaddrinfo(result);
    return false;
}

bool fx5_socket_send_all(fx5_socket_t *sock, const uint8_t *data, size_t size)
{
    size_t total_sent = 0u;

    if (sock == NULL || data == NULL) {
        return false;
    }

    if (sock->handle == FX5_INVALID_SOCKET) {
        return false;
    }

    while (total_sent < size) {
#ifdef _WIN32
        const int sent = send(sock->handle,
                              (const char *)(data + total_sent),
                              (int)(size - total_sent),
                              0);
#else
        const ssize_t sent = send(sock->handle,
                                  data + total_sent,
                                  size - total_sent,
                                  0);
#endif
        if (sent <= 0) {
            return false;
        }

        total_sent += (size_t)sent;
    }

    return true;
}



bool fx5_socket_set_recv_timeout_ms(fx5_socket_t *sock, uint32_t timeout_ms)
{
    if (sock == NULL) {
        return false;
    }

#ifdef _WIN32
    DWORD tv = (DWORD)timeout_ms;
    return setsockopt(sock->handle, SOL_SOCKET, SO_RCVTIMEO,
                      (const char *)&tv, sizeof(tv)) == 0;
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000u;
    tv.tv_usec = (timeout_ms % 1000u) * 1000u;

    return setsockopt(sock->handle, SOL_SOCKET, SO_RCVTIMEO,
                      &tv, sizeof(tv)) == 0;
#endif
}

fx5_transport_recv_status_t fx5_socket_recv_some(
    fx5_socket_t *sock,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_size
    )
{
    if (received_size != NULL) {
        *received_size = 0u;
    }

    if (sock == NULL || buffer == NULL || received_size == NULL) {
        return FX5_TRANSPORT_RECV_ERROR;
    }

    if (sock->handle == FX5_INVALID_SOCKET) {
        return FX5_TRANSPORT_RECV_ERROR;
    }

#ifdef _WIN32
    {
        const int rc = recv(sock->handle, (char *)buffer, (int)buffer_size, 0);
        if (rc > 0) {
            *received_size = (size_t)rc;
            return FX5_TRANSPORT_RECV_OK;
        }
        if (rc == 0) {
            return FX5_TRANSPORT_RECV_CLOSED;
        }

        {
            const int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) {
                return FX5_TRANSPORT_RECV_TIMEOUT;
            }
        }

        return FX5_TRANSPORT_RECV_ERROR;
    }
#else
    {
        const ssize_t rc = recv(sock->handle, buffer, buffer_size, 0);
        if (rc > 0) {
            *received_size = (size_t)rc;
            return FX5_TRANSPORT_RECV_OK;
        }
        if (rc == 0) {
            return FX5_TRANSPORT_RECV_CLOSED;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return FX5_TRANSPORT_RECV_TIMEOUT;
        }

        return FX5_TRANSPORT_RECV_ERROR;
    }
#endif
}
