#include "../include/fx5.h"


void fx5_wire_settings_init(
fx5_wire_settings_t *settings,
        fx5_wire_type_t wire_type)
{
    if (!settings) return;

    settings->wire_type = wire_type;

    if (wire_type == FX5_WIRE_SERIAL) {
        settings->u.serial_settings = (fx5_serial_settings_t)
                        FX5_SERIAL_SETTINGS_DEFAULT;
    } else {
        settings->wire_type = FX5_WIRE_TCP;
        settings->u.network_settings = (fx5_network_settings_t)
                        FX5_NETWORK_SETTINGS_DEFAULT;
    }
}

/*
 * Bemenet: wire_settings és pdu request
 * Kimenet: generált csomag
 * Visszatérési státusz:
 * ERR_NULL: kötelező pointer NULL
 * ERR_COUNT_ZERO: Az írandó/olvasandó eszközök száma érvénytelen (0)
 * ERR_ADDR_RANGE: ha a cím több mint 24 bites
 * TOO_SMALL: kicsi az output buffer
 * UNSUPPORTED: nem támogatott eszköz/parancs kombináció
 *
 * Főbb lépések:
 * 1. argumentum ellenőrzés
 * 2. PDU payload felépítése
 * 3. Wire header felépítése
 * 4. BCC számítás
 *
 */

fx5_status_t fx5_build_request(fx5_transaction_t *transaction, uint8_t *buffer, uint16_t max_size, uint16_t *actual_size) {
  if (transaction == NULL) return FX5_ST_ERR_NULL;
  if (transaction->request.count == 0u) return FX5_ST_ERR_INVALID_COUNT;
  if (transaction->request.count > FX5_MAX_VALUE_COUNT) return FX5_ST_ERR_INVALID_COUNT;
  if (transaction->request.address > FX5_MAX_ADDRESS) return FX5_ST_ERR_INVALID_ADDRESS;


  return FX5_ST_OK;
}
