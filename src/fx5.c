#include "../include/fx5.h"

#include "fx5_utility.h"
#include "fx5_private.h"

#include <stdbool.h>


void fx5_transaction_init(fx5_transaction_t *transaction) {
  if (!transaction) return;

  fx5_wire_settings_init(&transaction->wire_settings,FX5_WIRE_TCP);
  transaction->next_serial = 0u;
  transaction->resync_counter = 0u;
  transaction->response.count = 0u;
  transaction->response.response_code = FX5_RESPONSE_OK;
  transaction->request.count = 0u;
  transaction->request.address = 0x000000;

}

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



static bool is_supported_command(fx5_cmd_t cmd) {
  switch(cmd) {
    case FX5_CMD_BATCH_READ:
    case FX5_CMD_BATCH_WRITE:
      return true;
  }

  return false;
}

static bool is_supported_device(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_D:
        case FX5_DEV_M:
        case FX5_DEV_X:
        case FX5_DEV_Y:
            return true;
    }
    return false; // + debug trap
}


static bool is_bit_device(fx5_device_t device) {
  switch(device) {
    case FX5_DEV_D: return false;
    case FX5_DEV_M:
    case FX5_DEV_X:
    case FX5_DEV_Y:
      return true;

  }

  return false;
}



/**
 * @brief pdu_pack_words_to_bits_le
 * @param request
 * @param buffer
 * @return fx5_status_t
 * Nincsen ellenőrzés a buffer méretére.
 * A hívó függvény kötelessége gondoskodni a megfelelő méretű bufferről.
 */


static int pack_word_array_to_bitword_array_le(uint16_t *values,uint16_t count,uint8_t *buffer,uint16_t max_buffer_size) {
  if (values == NULL) return -1;
  if (buffer == NULL) return -1;

  uint16_t words = (uint16_t)((count + 15u) / 16u);
  uint16_t expected_size = (uint16_t)(words * 2u);
  if (max_buffer_size < expected_size) return -1;

  uint16_t index = 0;
  uint16_t value = 0;
  for(uint16_t k = 0; k < count;k++) {
    if (values[k] != 0u) value |= (uint16_t)(1u << (k & 15u));

    if ( (k & 15u) == 15 ) {
      index += pdu_wr_u16_le(&buffer[index],value);
      value = 0;
    }
  }

  if (count & 15u) index += pdu_wr_u16_le(&buffer[index],value);

  return index;
}



static uint16_t unpack_bitwords_from_cbuffer_to_word_array_le(cBuffer *buffer,uint16_t buffer_size,uint16_t *values,uint16_t max_count,uint16_t *remaining_bytes) {
  uint16_t word_count = buffer_size / 2;
  uint32_t bit_count = (uint32_t)word_count*16u;
  *remaining_bytes = 0u;
  if (bit_count > max_count) {
      bit_count = FX5_MAX_VALUE_COUNT;
      word_count = bit_count / 16;
      *remaining_bytes = buffer_size - 2*word_count;
  }

  uint16_t bit_index = 0;
  for(uint16_t word_index = 0; word_index < word_count;word_index++) {
    uint16_t value = pdu_rdc_u16_le(buffer);
    for(uint16_t pos = 0; pos < 16; pos++) {
      values[bit_index++] = (value & (uint16_t)(1u << pos))?FX5_BIT_TRUE : FX5_BIT_FALSE;

    }
  }

  return bit_count;
}



uint16_t get_required_buffer_size(fx5_transaction_t *transaction) {
  if (transaction == NULL) return 0; //this is a gray area, transaction should not be NULL, and this is a private function

  uint16_t size = 0;
  size += REQUEST_TCP_HEADER_SIZE;
  size += REQUEST_PDU_HEADER_SIZE;

  uint16_t unit_size = sizeof(uint16_t);
  uint16_t count = transaction->request.count;
  if (transaction->request.command == FX5_CMD_BATCH_WRITE) {
    if (is_bit_device(transaction->request.device)) {
      count = (uint16_t)((count + 15u) /16u); //hány word-ön fér el ennyi bit?
    }

    size += count*unit_size;
  }

  return size;
}

/*
 * Bemenet: wire_settings és pdu request
 * Kimenet: generált csomag
 * Visszatérési státusz:
 * ERR_NULL: kötelező pointer NULL
 * ERR_INVALID_COUNT: Az írandó/olvasandó eszközök száma érvénytelen (0)
 * ERR_INVALID_ADDRESS: ha a cím több mint 24 bites
 * ERR_TOO_SMALL: kicsi az output buffer
 * ERR_UNSUPPORTED: nem támogatott eszköz/parancs kombináció
 *
 * Főbb lépések:
 * 1. argumentum ellenőrzés
 * 2. szükséges hely kiszámítása
 * 3. Wire header felépítése
 * 4. PDU felépítése
 * 5. BCC számítás, és hozzáadás, ha majd lesz serial réteg is.
 *
 * Ez egy nagyon minimalista implementáció.
 * A függvény nem túl hosszú, ezért nem bontottuk fel részekre.
 * De a soros header hozzáadása + A BCC kód miatt még nőni fog.
 *
 */




fx5_status_t fx5_build_request(fx5_transaction_t *transaction, uint8_t *buffer, uint16_t max_size, uint16_t *actual_size) {
  if (buffer == NULL) return FX5_ST_ERR_NULL;
  if (actual_size == NULL) return FX5_ST_ERR_NULL;
  if (transaction == NULL) return FX5_ST_ERR_NULL;

  //soros verzió még nincsen implementálva
  if (transaction->wire_settings.wire_type == FX5_WIRE_SERIAL) return FX5_ST_ERR_UNSUPPORTED;

  if (transaction->request.count == 0u) return FX5_ST_ERR_INVALID_COUNT;
  if (transaction->request.count > FX5_MAX_VALUE_COUNT) return FX5_ST_ERR_INVALID_COUNT;
  if (transaction->request.address > FX5_MAX_ADDRESS) return FX5_ST_ERR_INVALID_ADDRESS;

  if (!is_supported_command(transaction->request.command)) return FX5_ST_ERR_UNSUPPORTED;
  if (!is_supported_device(transaction->request.device)) return FX5_ST_ERR_UNSUPPORTED;


  //előre kiszámoljuk a méretet, hogy ne kelljen lépésenként ellenőrizni, hogy beleférünk-e a bufferbe
  *actual_size = get_required_buffer_size(transaction);
  if (max_size < *actual_size) return FX5_ST_ERR_TOO_SMALL;



  uint16_t index = 0;
  //hozzáadjuk a tcp header-t
  index += pdu_wr_u16_le(&buffer[index],FX5_TCP_REQUEST_SUBHEADER_3E_BINARY);
  transaction->serial = transaction->next_serial++;
  index += pdu_wr_u32_le(&buffer[index],transaction->serial);
  buffer[index++] = (uint8_t)transaction->wire_settings.u.network_settings.network_no;
  buffer[index++] = (uint8_t)transaction->wire_settings.u.network_settings.pc_no;
  index += pdu_wr_u16_le(&buffer[index],transaction->wire_settings.u.network_settings.module_io_no);
  buffer[index++] = (uint8_t)transaction->wire_settings.u.network_settings.module_station_no;
  index += pdu_wr_u16_le(&buffer[index],*actual_size - REQUEST_TCP_HEADER_SIZE);
  index += pdu_wr_u16_le(&buffer[index],transaction->wire_settings.u.network_settings.monitoring_timer);
  //megvolt a tcp header, milyen jó is, hogy előre kiszámoltuk a csomag méretet!

  uint16_t count = transaction->request.count;
  uint16_t command = transaction->request.command;
  bool bitdev = is_bit_device(transaction->request.device);
  uint16_t subCommand = bitdev?PDU_SUB_BIT_UNIT:PDU_SUB_WORD_UNIT;

  //aztán hozzáadjuk a pdu-t
  index += pdu_wr_u16_le(&buffer[index], command);
  index += pdu_wr_u16_le(&buffer[index], subCommand);
  index += pdu_wr_u24_le(&buffer[index], transaction->request.address);
  buffer[index++] = (uint8_t)transaction->request.device;
  index += pdu_wr_u16_le(&buffer[index], transaction->request.count);

  //mivel már eleve WORD formátumban kérjük a bit-eket is, nem kell vacakolnunk a konverzióval
  if (command == FX5_CMD_BATCH_WRITE) {
    if (bitdev) {
      //bár ennek nem szabadna hibára futnia
      int written = pack_word_array_to_bitword_array_le(transaction->request.values,count,&buffer[index],(uint16_t)max_size - index);
      if (written < 0) return FX5_ST_ERR_INTERNAL;
      index += (uint16_t)written;
    } else {
      //ez így csúnya itt, lehet erre is kellene egy utility függvény
      for(uint16_t k = 0; k < count;k++) {
        index += pdu_wr_u16_le(&buffer[index],transaction->request.values[k]);
      }
    }
  }

  if (*actual_size != index) return FX5_ST_ERR_INTERNAL;

  return FX5_ST_OK;
}


#define MAGIC_NUMBER_OFFSET (0)
#define SERIAL_OFFSET (2)
#define PACKET_SIZE_OFFSET (6)

//nincsen arg ellenőrzés, szándékos
static void do_resync(cBuffer *rx_buffer,uint16_t *counter) {
  //oké szinkronizálási kísérlet
  while (bufferGetNumber(rx_buffer) > 1) {
    uint16_t magic_number = pdu_peekc_u16_le(rx_buffer,MAGIC_NUMBER_OFFSET);
    if (magic_number == FX5_TCP_RESPONSE_SUBHEADER_3E_BINARY) break;

    bufferDumpFromFront(rx_buffer,1);
    *counter++;
  }

}

/**
 * @brief copy_word_payload
 * @param buffer cBuffer, amiből másolunk, és töröljük is belőle
 * @param count  Ennyi word-öt másolunk ki belőle
 * @param response  Ide másoljuk
 * @return nincs
 * Szándékosan nincsen argumentum ellenőrzés.
 */

static uint16_t copy_word_payload(cBuffer *rx_buffer,uint16_t value_count,fx5_response_t *response) {
      uint16_t count = value_count;
      uint16_t remaining_bytes = 0;
      if (value_count > FX5_MAX_VALUE_COUNT) {
          count = FX5_MAX_VALUE_COUNT;
          remaining_bytes = (value_count - FX5_MAX_VALUE_COUNT)*2;
      }
      response->count = count;
      for(uint16_t k = 0; k < count;k++) {
        //ez is takarítja a cBuffer-t
        response->values[k] = pdu_rdc_u16_le(rx_buffer);
      }

      if (value_count > FX5_MAX_VALUE_COUNT) {
        //de nekünk kell kidobni a maradékot

        if (remaining_bytes > 0) {
          bufferDumpFromFront(rx_buffer,remaining_bytes);
        }
      }

  return remaining_bytes;
}

/*
 * Úgy tűnik, hogy a response TCP header-ből hiányzik a monitoring_timer
 * A pdu is csak egy árva response_code-ból áll, ami default 0x0000.
 * A madarék adat az olvasási parancs eredménye (remélem)
 */
fx5_status_t fx5_parse_response(fx5_transaction_t *transaction,cBuffer *rx_buffer) {
  if (transaction == NULL) return FX5_ST_ERR_NULL;
  if (rx_buffer == NULL) return FX5_ST_ERR_NULL;

  if (transaction->wire_settings.wire_type == FX5_WIRE_SERIAL) return FX5_ST_ERR_UNSUPPORTED;

  transaction->response.count = 0u; //csak a biztonság kedvéért
  transaction->resync_counter = 0u;
  while (true) {
    do_resync(rx_buffer,&transaction->resync_counter);
    if (transaction->resync_counter > FX5_RESYNC_LIMIT) return FX5_ST_ERR_RESYNC;

    uint32_t size = bufferGetNumber(rx_buffer);

    if (size < RESPONSE_TCP_HEADER_SIZE + RESPONSE_PDU_HEADER_SIZE) return FX5_ST_ERR_NEED_MORE;

    uint32_t serial = pdu_peekc_u32_le(rx_buffer,SERIAL_OFFSET);

    if (serial != transaction->serial) {
      bufferDumpFromFront(rx_buffer,1);
      continue; //visszaugrunk a resync-re
    }

    uint16_t packet_size = pdu_peekc_u16_le(rx_buffer,PACKET_SIZE_OFFSET);
    if (packet_size < 2 || packet_size > FX5_MAX_FRAME_BYTES) {
      bufferDumpFromFront(rx_buffer,1);
      continue; //resync
    }

    uint32_t total_needed = packet_size + RESPONSE_TCP_HEADER_SIZE;
    size = bufferGetNumber(rx_buffer);
    //ha nem jött át a teljes csomag, itt megállunk
    if (size < total_needed) return FX5_ST_ERR_NEED_MORE;

    //hanyagoljuk a teljes TCP header-t, ha a serial stimmel, ennek nincs jelentősége
    bufferDumpFromFront(rx_buffer, RESPONSE_TCP_HEADER_SIZE);

    transaction->response.response_code = pdu_rdc_u16_le(rx_buffer);
    uint16_t remaining_bytes = packet_size - 2;

    if (transaction->response.response_code != FX5_RESPONSE_OK) {
      bufferDumpFromFront(rx_buffer,remaining_bytes);
      return FX5_ST_RESPONSE_ERROR;
    }

    if (is_bit_device(transaction->request.device)) {
      //ez takarítja a cBuffer-t
      transaction->response.count = unpack_bitwords_from_cbuffer_to_word_array_le(rx_buffer,packet_size - 2,transaction->response.values,FX5_MAX_VALUE_COUNT,&remaining_bytes);

    } else {
      uint16_t value_count = remaining_bytes / 2;
      remaining_bytes = copy_word_payload(rx_buffer,value_count,&transaction->response);
      if (remaining_bytes > 0) {
        return FX5_ST_ERR_INVALID_COUNT;
      }
    }

    break;
  }

  return FX5_ST_OK;
}

