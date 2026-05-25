# fx5_lib Code Review

Date: 2026-05-25

Scope:
- `include/fx5/**`
- `src/fx5_core.c`
- `src/fx5_request.c`
- `src/fx5_response.c`
- `src/fx5_ringbuf.c`
- related `fx5_tests`

Out of scope:
- `tools/fx5_client/**`

## Findings

### P1 - Special/system devices are writable through the public library API

References:
- `src/fx5_request.c:22`
- `src/fx5_request.c:109`
- `src/fx5_core.c:178`
- `include/fx5/fx5.h:89`

The library now exposes `SM`, `SD`, `SB`, and `SW` as normal supported devices. `fx5_set_request()` validates only command, device, address, and count; it does not reject `FX5_CMD_BATCH_WRITE` for special/system devices. The CLI blocks writes to these devices, but any direct `fx5_lib` user can still call:

```c
fx5_set_request(ctx, FX5_CMD_BATCH_WRITE, FX5_DEV_SM, 400u, 1u);
```

and the request builder will produce a write frame.

This is a safety issue because special relays/registers may represent PLC/system state. Even if some addresses are writable by Mitsubishi design, the current API has no per-device or per-address policy, so it cannot distinguish safe diagnostics from risky system writes.

Recommendation:
- Add a library-level write policy, not only CLI-level protection.
- Minimal fix: reject `FX5_CMD_BATCH_WRITE` for `FX5_DEV_SM`, `FX5_DEV_SD`, `FX5_DEV_SB`, and `FX5_DEV_SW`.
- Better fix: introduce device descriptors with `is_bit`, `address_base`, and `allow_write` metadata, then use that single table in validation and request building.
- Add tests that writes to read-only devices return `FX5_ST_ERR_UNSUPPORTED`.

### P1 - Word response parsing trusts payload length instead of the requested count

References:
- `src/fx5_response.c:109`
- `src/fx5_response.c:118`
- `src/fx5_response.c:123`
- `src/fx5_core.c:178`

`fx5_parse_word_response_payload()` derives `context->count` from `payload_size / 2` and accepts any even payload length up to `FX5_MAX_VALUE_COUNT`. It does not compare the response payload to the request count previously set by `fx5_set_request()`.

For batch read, the expected word payload size is `requested_count * 2`. If a malformed, stale, or mismatched frame returns fewer or more words than requested, the parser still returns `FX5_ST_OK` and exposes a response count different from the active request.

This is inconsistent with the bit response parser, which now uses the requested count to avoid exposing extra packed bits.

Recommendation:
- Preserve `requested_count = context->count` before parsing.
- For word batch-read responses, require `payload_size == requested_count * 2`.
- On mismatch, drop the payload and return `FX5_ST_ERR_INVALID_COUNT` or a more specific protocol mismatch status.
- Add tests for short and long word payloads.

### P2 - Error responses leave the previous request count visible

References:
- `src/fx5_response.c:223`
- `src/fx5_response.c:226`
- `src/fx5_response.c:230`
- `src/fx5_core.c:318`

When the PLC returns a non-zero end code, `fx5_parse_response_pdu()` records `response_code`, drops the error payload, and returns `FX5_ST_RESPONSE_ERROR`. It does not clear `context->count`.

Because `context->count` also stores the request count, a caller that checks `fx5_get_response_count()` after an error response can see the requested count rather than zero response values. The CLI avoids this by checking the end code first, but the public API makes the state look like data is available.

Recommendation:
- Set `context->count = 0u` before returning `FX5_ST_RESPONSE_ERROR`.
- Add a test that parses a non-zero end-code response and asserts `fx5_get_response_count(ctx) == 0`.

### P2 - `FX5_MAX_VALUE_COUNT` is a small compile-time protocol cap, but the API docs understate the operational impact

References:
- `include/fx5/fx5.h:45`
- `src/fx5_core.c:188`
- `src/fx5_request.c:114`

The public default `FX5_MAX_VALUE_COUNT` is `32`, and `fx5_set_request()` rejects any larger count. The Mitsubishi protocol supports much larger batch sizes than this for common device access, but the current library limit is driven by static context storage.

This is not necessarily wrong for an embedded/static library, but it is easy for users to mistake it for a PLC or protocol limit. It also applies equally to word and bit devices, so bit reads are limited to 32 points even though bit payload packing would allow more without much memory pressure.

Recommendation:
- Document this as a library storage limit, not a protocol limit.
- Consider separate limits for word values and bit values.
- Consider exposing a compile-time configuration example for larger batch reads.

### P3 - Device metadata is duplicated across the library and CLI-facing layers

References:
- `include/fx5/fx5.h:89`
- `src/fx5_request.c:22`
- `src/fx5_request.c:46`

The library currently encodes device identity in the public enum, supported-device validation, and bit/word classification. The CLI has its own descriptor table for prefix parsing and address base handling.

This duplication makes future device additions error-prone. For example, adding timer/counter devices later will require coordinated changes in multiple places, and there is no single place to attach properties such as address base, write policy, or double-word semantics.

Recommendation:
- Add an internal `fx5_device_descriptor_t` table in `fx5_lib`.
- Use it for supported-device checks, bit/word classification, and write policy.
- Optionally expose read-only lookup helpers for tools if keeping parser logic outside the library remains desirable.

## Test Coverage Notes

Current coverage is useful for:
- 3E request vector generation.
- Basic bit/word response parsing.
- Error response status handling.
- Resync behavior.
- Newly added common device request construction.
- Odd-count bit response parsing.

Coverage gaps worth closing next:
- Writes to special/system devices are rejected at library level.
- Word read response payload shorter than requested is rejected.
- Word read response payload longer than requested is rejected.
- Error response clears response count.
- 4E response parsing for successful reads/writes, including serial mismatch.
- `FX5_MAX_VALUE_COUNT` boundary behavior for both bit and word devices.

## Summary

The library is compact and understandable, and the request/response split is workable. The main issue is that the public API still treats all supported device codes as equally safe and equally shaped. As device coverage grows, `fx5_lib` needs a single source of truth for device metadata and stricter response/request consistency checks.

Recommended next implementation order:
1. Add library-level write protection for special/system devices.
2. Make word response parsing validate against requested count.
3. Clear response count on non-zero PLC end code.
4. Consolidate device metadata into one internal descriptor table.
