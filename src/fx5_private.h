#ifndef FX5_PRIVATE_H
#define FX5_PRIVATE_H

#include "../include/fx5.h"


#define REQUEST_TCP_HEADER_SIZE (15)
#define RESPONSE_TCP_HEADER_SIZE (13) /** Nincs benne a monitoring timer **/
#define REQUEST_PDU_HEADER_SIZE (10)
#define RESPONSE_PDU_HEADER_SIZE (2) /** Csak a response_code van benne alapból **/

#define FX5_TCP_REQUEST_SUBHEADER_3E_BINARY (0x0054u)
#define FX5_TCP_RESPONSE_SUBHEADER_3E_BINARY (0x00D4u)

#define FX5_MAX_FRAME_BYTES (2048)
/*
 * A kimeneti bufferbe írjuk a PDU-t.
 * Meg kell nézni, hogy bit/regiszter eszközt kezelünk, és hogy írjuk, vagy olvassuk
 */
fx5_status_t fx5_build_pdu_request(
    const fx5_request_t *request, /**< FX5 request **/
    uint8_t *buffer, /**< Kimeneti buffer **/
    uint16_t max_size, /**< Kimeneti buffer mérete **/
    uint16_t *actual_size
    );

/**
 * @brief fx5_build_pdu_read_request
 * @param request
 * @param buffer
 * @param max_size
 * @param actual_size
 * @return
 */

fx5_status_t fx5_build_pdu_read_request(
    const fx5_request_t *request, /**< FX5 request **/
    uint8_t *buffer, /**< Kimeneti buffer **/
    uint16_t max_size, /**< Kimeneti buffer mérete **/
    uint16_t *actual_size /**< Ennyi helyre volt szükség **/
    );

fx5_status_t fx5_build_pdu_write_request(
    const fx5_request_t *request, /**< FX5 request, milyen eszközt, írni vagy olvasni, és mennyit **/
    uint8_t *buffer,  /**< Kimeneti buffer **/
    uint16_t max_size, /**< Kimeneti buffer mérete **/
    uint16_t *actual_size  /**< Ennyi helyre volt szükség **/
    );

#define FX5_TCP_3E_HEADER_SIZE (11u)  // subheader..monitoring timer együtt

/*
 * Beleírjuk a headert a kimeneti bufferbe
 */
fx5_status_t fx5_build_header(
    const fx5_wire_settings_t *settings, /**< Beállítások, soros vagy hálózati **/
    uint8_t *buffer, /**< Kimeneti buffer **/
    uint16_t size, /**< A kimeneti buffer mérete **/
    uint16_t *header_size  /**< A header mérete **/
    );

/*
 * Beleírjuk a fix méretű hálózati headert a kimeneti bufferbe
 */
fx5_status_t fx5_build_tcp_header_3e(
    const fx5_network_settings_t *settings, /**< Hálózati beállítások **/
    uint8_t *buffer,  /**< A kimeneti buffer **/
    uint16_t size, /**< A kimeneti buffer mérete **/
    uint16_t *header_size  /**< A header mérete **/
    );


/*
 * Beleírjuk a fix méretű soros headert a kimeneti bufferbe
 */
fx5_status_t fx5_build_serial_header(
    const fx5_serial_settings_t *settings, /**< soros beállítások **/
    uint8_t *buffer,  /**< A kimeneti buffer **/
    uint16_t size, /**< A kimeneti buffer mérete **/
    uint16_t *header_size /**< A header mérete **/
    );


/*
 * Ellenőrző összeget számolunk a generált csomagra.
 */
uint8_t fx5_calc_bcc_sum8(const uint8_t *data, uint16_t len);
uint8_t fx5_calc_bcc_xor8(const uint8_t *data, uint16_t len);




#endif // FX5_PRIVATE_H
