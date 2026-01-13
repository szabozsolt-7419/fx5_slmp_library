/**
 * @file fx5.h
 * @brief High-level FX5 communication API.
 *
 * This header defines the public API for building FX5 requests and
 * parsing FX5 responses. The library is designed for embedded systems
 * and does not perform dynamic memory allocation.
 *
 * The implementation separates:
 *  - PDU (protocol data unit) handling
 *  - wire layer (TCP / serial framing)
 *
 * @note This API is not thread-safe.
 */

#ifndef FX5_LIB_H
#define FX5_LIB_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "buffer.h"

//Legfeljebb ennyi eszköz írását/olvasását támogatjuk egy tranzakció során
#define FX5_MAX_VALUE_COUNT (32)

/*
 * Eszköz típusok
 */
typedef enum {
    FX5_DEV_D = 0xA8,
    FX5_DEV_X = 0x9C,
    FX5_DEV_Y = 0x9D,
    FX5_DEV_M = 0x90
} fx5_device_t;

/*
 * Támogatott parancsok
 */
typedef enum {
    FX5_CMD_BATCH_WRITE = 0x1401,
    FX5_CMD_BATCH_READ = 0x0401
} fx5_cmd_t;

/*
 * Támogatott protokollok
 */
typedef enum {
    FX5_WIRE_TCP,
    FX5_WIRE_SERIAL
} fx5_wire_type_t;


#define FX5_MAX_ADDRESS 0x00FFFFFF

/*
 * TCP hálózati beállítások
 */
typedef struct fx5_network_settings {
    uint8_t network_no; //0x00 for same network
    uint8_t pc_no; //0xFF for own station
    uint16_t module_io_no; //0x03FF for CPU
    uint8_t module_station_no; //0x00;
    uint16_t monitoring_timer; //0x0010 = 4s
} fx5_network_settings_t;

/*
 * BCC mód soros protokollnál
 */
typedef enum {
    FX5_BCC_NONE = 0,
    FX5_BCC_SUM8,      // "sum check code" jelleg (gyakori)
    FX5_BCC_XOR8       // ha később mégis ilyen kell
} fx5_bcc_mode_t;

/*
 * Soros beállítások
 */
typedef struct fx5_serial_settings {
    uint8_t station_no;     // destination station
    fx5_bcc_mode_t bcc;     // checksum on/off (+mode)
} fx5_serial_settings_t;


#define FX5_SERIAL_SETTINGS_DEFAULT \
{                               \
    .station_no = 0x00,         \
    .bcc        = FX5_BCC_SUM8  \
}

#define FX5_NETWORK_SETTINGS_DEFAULT \
{                               \
    .network_no        = 0x00,  \
    .pc_no             = 0xFF,  \
    .module_io_no      = 0x03FF,\
    .module_station_no = 0x00,  \
    .monitoring_timer  = 0x0010 \
}


typedef struct fx5_wire_settings {
    fx5_wire_type_t wire_type;
    union {
        fx5_network_settings_t network_settings;
        fx5_serial_settings_t serial_settings;
    } u;
} fx5_wire_settings_t;

typedef struct fx5_request {
    fx5_device_t device;
    fx5_cmd_t command;
    uint32_t address;
    uint16_t count;      // argument count (words or bits)
    uint16_t values[FX5_MAX_VALUE_COUNT]; // write-only
} fx5_request_t;

typedef struct fx5_response {
    uint16_t response_code;
    uint16_t values[FX5_MAX_VALUE_COUNT]; // read-only
    uint16_t count;
} fx5_response_t;

typedef struct fx5_transaction {
    fx5_request_t request;
    fx5_response_t response;
    fx5_wire_settings_t wire_settings;
} fx5_transaction_t;

typedef enum {
    FX5_ST_OK = 0,   /**< Minden rendben **/
    FX5_ST_ERR_NULL, /**< Kötelező paraméter értéke NULL **/
    FX5_ST_ERR_TOO_SMALL, /**< A kimenő buffer túl kicsi a lekérdezés generálásához **/
    FX5_ST_ERR_NEED_MORE, /**< A bejövő ringbuffer-ben még nincs meg a teljes válasz **/
    FX5_ST_ERR_INVALID_ADDRESS,   /**< Hibás cím **/
    FX5_ST_ERR_INVALID_COUNT, /**< Hibás az olvasandó/írandó eszközök száma **/
    FX5_ST_ERR_UNSUPPORTED /**< Nem támogatott parancs **/
} fx5_status_t;




/*
 * Legenerálja a teljes csomagot (PDU + fejléc), amit el kell küldeni a PLC felé.
 * Ha túl kicsi a kimeneti buffer, akkor ERR_TOO_SMALL hibával tér vissza.
 */
fx5_status_t fx5_build_request(fx5_transaction_t *transaction, uint8_t *buffer, uint16_t max_size, uint16_t *actual_size);


/*
 * Elemzi a PLC válaszát a lekérdezésre. A felhasználó felelőssége beállítani a megfelelő wire_type-ot.
 * A parser fogyasztja a bejövő byte-okat a ringbuffer-ből.
 * Ha OK, akkor a teljes frame-et kidobja a ringbufferből.
 * Ha NEED_MORE, akkor nem dob el semmit.
 * Ha hibás a frame formátum, akkor resync-elni kezd, addig dobál byte-okat, amíg talál egy frame kezdetet, vagy amíg van adat a ringbufferben.
 */
fx5_status_t fx5_parse_response(fx5_transaction_t *transaction,cBuffer *rx_buffer);

/*
 * Első használat előtt inicializálni kell a beállításokat.
 * Ez alapján generálódik a header a kiküldött csomagokhoz, és a bejövő csomagok elemzéséhez is kell.
 * Az alapbeállításokra inicializál. Ha routing kell, vagy egyedi konfiguráció, akkor utána azt külön be kell állítani.
 */
void fx5_wire_settings_init(
    fx5_wire_settings_t *settings,
    fx5_wire_type_t wire_type);




#ifdef __cplusplus
}
#endif

#endif // FX5_LIB_H
