/**
 * @file fx5_client_transport.h
 * @brief Minimal cross-platform TCP transport wrapper for fx5_client.
 */
#ifndef FX5_CLIENT_TRANSPORT_H
#define FX5_CLIENT_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct fx5_socket fx5_socket_t;

/**
 * @brief Result of a receive operation.
 */
typedef enum
{
    FX5_TRANSPORT_RECV_OK = 0,
    FX5_TRANSPORT_RECV_TIMEOUT,
    FX5_TRANSPORT_RECV_CLOSED,
    FX5_TRANSPORT_RECV_ERROR
} fx5_transport_recv_status_t;

/**
 * @brief Initialize global transport resources.
 *
 * Required on platforms such as Windows that need socket subsystem startup.
 *
 * @return true on success, false on failure.
 */
bool fx5_transport_global_init(void);

/**
 * @brief Shut down global transport resources.
 */
void fx5_transport_global_shutdown(void);

/**
 * @brief Allocate a transport socket wrapper.
 *
 * @return Newly allocated socket wrapper, or NULL on failure.
 */
fx5_socket_t *fx5_socket_open(void);

/**
 * @brief Close and free a transport socket wrapper.
 *
 * @param[in,out] sock Socket wrapper to close and release.
 */
void fx5_socket_close(fx5_socket_t *sock);

/**
 * @brief Connect a socket to a remote TCP endpoint.
 *
 * @param[in,out] sock Socket wrapper.
 * @param[in] host Remote host name or IP address.
 * @param[in] port Remote TCP port.
 * @return true on success, false on failure.
 */
bool fx5_socket_connect(fx5_socket_t *sock, const char *host, uint16_t port);

/**
 * @brief Set the receive timeout for a connected socket.
 *
 * @param[in,out] sock Socket wrapper.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return true on success, false on failure.
 */
bool fx5_socket_set_recv_timeout_ms(fx5_socket_t *sock, uint32_t timeout_ms);

/**
 * @brief Send the entire buffer over the socket.
 *
 * @param[in,out] sock Socket wrapper.
 * @param[in] data Buffer to send.
 * @param[in] size Number of bytes to send.
 * @return true on success, false on failure.
 */
bool fx5_socket_send_all(fx5_socket_t *sock, const uint8_t *data, size_t size);

/**
 * @brief Receive some bytes from the socket.
 *
 * @param[in,out] sock Socket wrapper.
 * @param[out] buffer Destination buffer.
 * @param[in] buffer_size Size of the destination buffer.
 * @param[out] received_size Number of bytes actually received.
 * @return Receive status value.
 */
fx5_transport_recv_status_t fx5_socket_recv_some(
    fx5_socket_t *sock,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_size
    );

#endif /* FX5_CLIENT_TRANSPORT_H */