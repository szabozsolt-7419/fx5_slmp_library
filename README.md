# FX5 Minimal C Library

Minimal, allocation-free C library for communicating with Mitsubishi FX5 PLCs using SLMP binary protocol over TCP.

---

## Overview

This library provides a small and predictable API for:

* Building SLMP requests (3E / 4E binary)
* Parsing SLMP responses (auto-detect 3E / 4E)
* Handling streaming TCP input
* Operating without dynamic memory allocation

The design targets embedded systems and low-level integrations.

---

## Features

* SLMP binary protocol (TCP only)
* 4E request generation (default)
* Optional 3E request generation
* Automatic 3E / 4E response detection
* Batch read / write
* Devices: D, M, X, Y
* No dynamic allocation
* Opaque context
* Internal context pool

---

## Not Supported

* Serial communication (RS232 / RS485)
* ASCII protocol variants
* Automatic transport abstraction

---

## Project Structure

```
include/
  fx5/
    fx5.h                # Public API
    buffer.h             # Byte buffer (temporary, planned rewrite)
    internal/
      fx5_private.h      # Internal API (used by tests)
      fx5_utility.h      # Low-level helpers

src/
  fx5.c
  buffer.c
```

---

## Public API Example

```c
fx5_context_t *ctx;

if (fx5_acquire(&ctx) != FX5_ST_OK) {
    // handle error
}

fx5_set_request(ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0, 10);

uint8_t tx[256];
uint16_t size;

fx5_build_request(ctx, tx, sizeof(tx), &size);

// send tx over TCP...

// receive data...
fx5_feed_response_bytes(ctx, rx_data, rx_size);

if (fx5_parse_response(ctx) == FX5_ST_OK) {
    uint16_t count = fx5_get_response_count(ctx);

    for (uint16_t i = 0; i < count; ++i) {
        uint16_t value;
        fx5_get_response_value(ctx, i, &value);
    }
}

fx5_release(ctx);
```

---

## Internal API

The file:

```
include/fx5/internal/fx5_private.h
```

contains the internal API used for:

* Request building
* Response parsing
* Low-level protocol testing

This API is **not stable** and intended only for:

* unit tests
* internal development

---

## Build

Basic CMake usage:

```
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Testing

Planned:

* Small embedded-friendly unit test framework
* Tests for:

  * PDU generation
  * Frame parsing
  * Resynchronization

---

## Design Goals

* Deterministic behavior
* No hidden allocations
* Clear separation between:

  * public API
  * internal protocol logic
* Testability at protocol level

---

## Roadmap

* [ ] Hardware validation against real FX5 PLC

---

## License

MIT

---

## Notes

The library is designed to remain small, explicit, and easy to integrate.

## Tools

A simple CLI tool for testing the library is available:

- `fx5_client` — interactive TCP client for FX5 PLC communication  
  See: `tools/fx5_client/`