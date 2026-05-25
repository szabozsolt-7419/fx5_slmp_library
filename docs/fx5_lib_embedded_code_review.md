# fx5_lib Embedded Code Review

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

Review context:
- The library targets small devices, including AVR-class systems.
- No dynamic allocation is intentional.
- `FX5_MAX_VALUE_COUNT` and `FX5_CONTEXT_POOL_SIZE` are deliberate compile-time RAM controls.
- Findings below treat small fixed buffers as a design constraint, not a defect.

## Findings

### Fixed - Configurable limits can silently overflow on small C targets

References:
- `include/fx5/fx5.h:54`
- `include/fx5/fx5.h:62`
- `include/fx5/fx5.h:65`
- `include/fx5/fx5.h:68`
- `src/fx5_request.c:112`
- `src/fx5_request.c:124`

`FX5_MAX_VALUE_COUNT` is now a storage-slot count, and bit devices derive their logical limit as `FX5_MAX_VALUE_COUNT * 16`. That is a good RAM-saving model, but the derived macros are cast down to `uint16_t` and there is no compile-time guard for invalid custom values.

On AVR and other 16-bit `int` targets, the arithmetic behind macros such as:

```c
#define FX5_MAX_BIT_VALUE_COUNT ((uint16_t)(FX5_MAX_VALUE_COUNT * 16u))
```

can overflow earlier than it would on a desktop compiler. If a user raises `FX5_MAX_VALUE_COUNT` too far, the public max count, payload byte count, and request-size macros can wrap while the backing arrays remain sized from the original macro. That can produce confusing build-time or runtime behavior instead of a clear configuration failure.

Status:
- Fixed by adding compile-time validation for zero and too-large limits.
- Fixed by forcing wider arithmetic before narrowing the derived public macros.

Implemented behavior:

```c
#if FX5_MAX_VALUE_COUNT == 0
#error "FX5_MAX_VALUE_COUNT must be greater than zero"
#endif

#if FX5_MAX_VALUE_COUNT > 4095
#error "FX5_MAX_VALUE_COUNT must be <= 4095 because API counts are uint16_t"
#endif

#if FX5_CONTEXT_POOL_SIZE == 0
#error "FX5_CONTEXT_POOL_SIZE must be greater than zero"
#endif
```

The maximum supported custom `FX5_MAX_VALUE_COUNT` is 4095. That keeps
`FX5_MAX_BIT_VALUE_COUNT` within the `uint16_t` public API count range.

### Fixed - Bit read responses accept extra payload bytes

References:
- `src/fx5_response.c:209`
- `src/fx5_response.c:211`
- `src/fx5_response.c:220`
- `src/fx5_response.c:227`
- `src/fx5_response.c:235`

Word read responses now require an exact payload length match against the requested count. Bit read responses only reject short payloads:

```c
if (payload_size < expected_payload_size) {
    ...
    return FX5_ST_ERR_INVALID_COUNT;
}
```

If a bit read requests one point, a response payload containing two bytes is accepted. The parser exposes only the requested bit count and drops the remaining bytes. This is memory-safe, but it makes malformed, stale, or mismatched responses look successful.

For a small embedded client, deterministic protocol state is usually more valuable than permissive parsing. If the PLC returns a frame whose packet size does not exactly match the active request, the caller should be told that the response is inconsistent.

Status:
- Fixed by requiring `payload_size == expected_payload_size`, matching word-response behavior.
- Covered by `test_parse_bit_response_rejects_long_payload()`.

### P3 - Device metadata is still duplicated in switch statements

References:
- `src/fx5_request.c:22`
- `src/fx5_request.c:46`
- `src/fx5_request.c:71`
- `src/fx5_request.c:86`

The current implementation repeats device knowledge across separate switches:
- supported device
- bit/word classification
- write policy
- max-count selection through `fx5_is_bit_device()`

This is not currently a runtime bug, but it is a maintenance risk as soon as more Mitsubishi device classes are added. On embedded targets, the obvious concern is that a descriptor table might cost RAM. It does not have to: a `static const` table or compact switch-backed descriptor helper can stay in flash/ROM on most targets.

Recommendation:
- Consolidate device metadata into one internal helper, for example:

```c
typedef struct {
    fx5_device_t device;
    bool is_bit;
    bool allow_write;
} fx5_device_descriptor_t;
```

- Keep the table `static const`.
- Use it for supported-device checks, bit/word classification, and write policy.
- If AVR flash placement matters later, wrap the table with a target-specific `FX5_CONST`/`PROGMEM` abstraction.

### Fixed - `fx5_set_request()` clears the whole value array for read requests too

References:
- `src/fx5_core.c:173`
- `src/fx5_core.c:187`
- `src/fx5_core.c:192`

Every successful `fx5_set_request()` clears all `FX5_MAX_VALUE_COUNT` storage slots:

```c
for (uint16_t k = 0u; k < FX5_MAX_VALUE_COUNT; ++k) {
    ctx->values[k] = 0u;
}
```

This is safe and simple. With the current default of 32 slots it is not a problem. On small MCUs, however, users are likely to tune `FX5_MAX_VALUE_COUNT`; if they raise it for larger bit batches, every read request pays the full clear cost even though read payload parsing overwrites the requested values.

Status:
- Fixed by adding `FX5_CLEAR_VALUES_ON_SET_REQUEST`.
- The default remains enabled for deterministic behavior and stale-value protection.
- Embedded builds may define `FX5_CLEAR_VALUES_ON_SET_REQUEST=0` to skip the clear when all write values are initialized by the application before request build.

## Previously Reported Items Now Addressed

The following issues from `docs/fx5_lib_code_review.md` appear fixed in the current code:
- Special/system writes are rejected in the library-level request validation path.
- Word read response payload length is validated against the requested count.
- Non-zero PLC response codes clear the exposed response count.
- `FX5_MAX_VALUE_COUNT` is now documented as storage slots, with separate word and bit logical limits.

## Test Coverage Notes

Current tests cover important embedded-facing boundaries:
- separate word and bit count limits
- max bit count request/response
- read-only special/system write rejection
- too-long bit read response rejection
- short and long word payload rejection
- non-zero PLC response count clearing

Coverage gaps worth closing next:
- custom compile-time values for `FX5_MAX_VALUE_COUNT`
- `FX5_CONTEXT_POOL_SIZE` boundary configuration
- 4E success and serial mismatch paths under fixed-buffer conditions

## Summary

The library is moving in the right direction for small targets: fixed storage, no heap use, compact context state, and explicit transaction limits. The main remaining structural issue is duplicated device metadata. The practical P2 risks from this review have been addressed with compile-time configuration guards and strict bit-response length checking.
