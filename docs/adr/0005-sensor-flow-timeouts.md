# ADR 0005: Sensor flow timeouts and receipt gesture

- **Status:** PROPOSED
- **Date:** 2026-11-19
- **Supersedes:** The ADR 0004 decision that a single short press after the
  rent receipt starts another rent.

## Context

SensorNode can wait indefinitely when an ActuatorNode reply is missing. Also,
the return request frame was treated as if it were already a payment request,
so a short press could send payment confirmation before the user had received
the payment prompt. After a rent receipt, the 350 ms single-press delay could
also issue a new rent request while the bike was already in use.

## Decision

- After a rent receipt, a lone short press sends no frame. Two short presses
  within 350 ms still request `rent:m7` to display the receipt again.
- After sending `return:m0`, SensorNode enters `AwaitingPaymentRequest`. It
  enters `AwaitingPaymentConfirmation` only after receiving the payment-request
  frame. Short presses before that frame do nothing.
- Each flow waiting for an ActuatorNode reply has a 2000 ms response timeout,
  measured through `IClock`. The timed waits are for rent reservation, rent
  receipt, rent receipt display, return payment request, return receipt, and
  return-ended replies.
- On timeout SensorNode returns to `Idle`, or `RentReceiptAvailable` if a rent
  receipt has already been shown, and prints one timeout line on the user
  terminal. Waiting for user input, including payment confirmation, return
  close, and a possible second receipt press, has no timeout.
- A SensorNode timeout does not cancel or undo work already performed by
  ActuatorNode. No protocol frame, frame ID, or model JSON is changed.

## Consequences

- A missing reply no longer leaves SensorNode permanently in a reply-waiting
  flow. A later interaction can proceed according to its recovered flow.
- The timeout is local to SensorNode; it does not imply transactional rollback
  or add an acknowledgment/cancellation protocol.
- `model/*.json`, protocol IDs, and the frame format remain unchanged.

## Verification

Native integration tests cover the post-receipt lone press, an idle long press
without a payment request and timeout recovery, payment confirmation only after
the request, lost rent-reply recovery, and user-wait states without timeouts.
