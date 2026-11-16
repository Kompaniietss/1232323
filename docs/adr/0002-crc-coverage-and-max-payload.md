# ADR 0002: CRC coverage and maximum payload of IoT frames

## Status

Accepted

## Context

`model/protocol.json` defines the frame `[0xA5][id][len][payload...][crc8]`, polynomial 0x07 and
initial value 0x00, but does not say which bytes the CRC covers or how large a payload may be.
The first firmware stub left the CRC at 0, so frames were never protected.

## Decision

- CRC-8 (poly 0x07, init 0x00) is computed over `id`, `len` and `payload`. The start byte and the
  CRC byte itself are excluded. The start byte is checked separately when parsing.
- Maximum payload is 15 bytes (19 bytes on the wire).
- Both values are recorded in `model/protocol.json` as `crcCoverage` and `maxPayloadBytes`.

## Consequences

- Corrupted headers are detected, not only corrupted payloads.
- Any other implementation of the protocol (for example the TypeScript reference, if it encodes
  frames) must compute the CRC the same way.
- Unit tests cover the standard CRC-8 check vector 0xF4 for "123456789", encode/decode,
  serialize/parse, corrupted CRC, bad start byte, truncated and oversized frames.
