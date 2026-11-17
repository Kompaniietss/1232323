# ADR 0004: Firmware sequence endpoints and simulated payment

- **Status:** Accepted
- **Date:** 2026-11-17
- **Approval:** Option B and the implementation scope explicitly supplied by
  the user on 2026-11-17.

## Context

The firmware had protocol IDs for the modeled rent and return messages but no
handlers for the user-facing endpoints. The Arduino Mega has one physical
button, LEDs, two UARTs between its logical nodes, and USB Serial. It has no
screen or payment-provider connection. The accepted mapping decision assigns
the user terminal to USB Serial and defines state-dependent button gestures.

The documented source `.drawio` files were not present at their supplied
Downloads paths during this implementation. The unchanged `model/sequences.json`
and `model/protocol.json` were used to preserve the message IDs and order.

## Decision

- USB Serial at 115200 baud is the User terminal. It prints the rent receipt
  (`rent:m6`), return payment prompt (`return:m2`), and return receipt
  (`return:m6`).
- Payment is simulated. The return prompt identifies it as simulated; button
  confirmation does not contact a provider or transfer funds.
- The button is state-dependent on `SensorNode`:
  - Idle short press sends the rent request; idle press of at least 800 ms sends
    the return request.
  - After a return payment prompt, short press sends `return:m3`.
  - After a return receipt, short press sends `return:m7`; the Actuator replies
    with `return:m8` only after receiving that close frame.
  - After a rent receipt, two short presses within 350 ms request `rent:m7`.
    A single short press is deferred for that window, then starts another rent.
- The Sensor automatically sends `rent:m3` after receiving `rent:m2`; no extra
  button gesture is inserted into the modeled rent flow.
- Rent reservation and return locking use the existing state-machine
  transitions. The Station inventory is updated at release/dock. The return
  bike state changes only after the simulated payment confirmation.
- `rent:m6` uses the zero-duration amount from the existing pricing rule
  (100 cents, the unlock fee). Return cost uses actual elapsed firmware time.
- Keep all protocol IDs and sequence JSON unchanged. Keep the existing
  TypeScript application and its payment mock unchanged.

## Consequences

- Firmware handlers now cover rent `m1`–`m7` and return `m1`–`m8`; the tests
  exercise the full button-to-UART-to-Actuator-to-LED/terminal paths.
- `IUserTerminal` remains a pure HAL interface; `ArduinoUserTerminal` formats
  currency on USB Serial. No Arduino-specific dependency was added to the
  domain interfaces.
- The trace `frameId` is the final emitted frame for a successful rent/return
  interaction; it is null for state-only triggers. `ledColor` is populated on
  every step. `costCents` is populated when the interaction produces a cost
  or receipt and null for state-only/no-charge steps.
- The 350 ms double-press window and the rent-start receipt at the unlock fee
  are implementation assumptions made to realize the requested gestures and
  `rent:m6`; they do not add or reorder modeled messages.
- This is not real payment processing. The web application still uses its
  separate mock payment adapter.

## Alternatives

- Leave receipt/payment/close interactions unmapped: rejected by the user's
  Option B decision.
- Add a display or payment provider: rejected because neither exists on the
  specified hardware and real payment is outside scope.
- Change protocol IDs or `model/sequences.json` to fit the firmware: rejected;
  the existing model is unchanged.

## Affected requirements and diagram elements

- Rent `m1`–`m7`; return `m1`–`m8`; corresponding protocol frames in
  `model/protocol.json`.
- Endpoint/gesture mapping is an accepted implementation interpretation.
  Payment confirmation is explicitly simulated, not a real charge.
- The TypeScript application sequence declarations differ from the firmware
  sequence model. This request changes firmware only; that pre-existing
  cross-representation discrepancy is retained and reported, not silently
  reconciled.
- No source UML or model JSON was edited. Direct visual comparison to the
  supplied `.drawio` files was unavailable because they were absent at their
  documented paths.

## Verification/reporting impact

Run `npm run model:verify`, `npm run traces:verify`, `pio test -e native`, and
`pio run -e mega`. Report the trace parity, native integration results, model
alignment, and any source-diagram availability limitation.
