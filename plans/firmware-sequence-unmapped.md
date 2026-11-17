# Firmware sequence messages requiring a mapping decision

Group 1 (the real `e1`/`e2` associations) is implemented and verified. Do not
start Groups 2 or 3 until Group 1 has been committed and the following mappings
are decided.

The firmware has a button-driven SensorNode, a Bike-owning ActuatorNode, LEDs,
and a point-to-point link. The existing protocol catalog has frame IDs for each
sequence message, but an ID alone is not an implemented sender, receiver, or
behavior. Mapping a frame requires a real handler on the correct node.

## Unmapped, needs decision

| Sequence element | Diagram interaction | Why it is not mapped |
| --- | --- | --- |
| `rent:m6` | Rental → User: `cost + receipt` | Cost/receipt delivery is an App/User presentation or payment interaction; neither MCU node has that role or a display/output contract. |
| `rent:m7` | User → Station: `show receipt` | A receipt-display request has no defined physical input, display, or Station endpoint. |
| `return:m2` | Station → User: `payment request` | The MCU model has no payment-service or user payment-request output endpoint. |
| `return:m3` | User → Station: `confirm payment` | No payment confirmation input or behavior is defined for the button/SensorNode. |
| `return:m6` | Station → User: `receipt` | Receipt presentation has no MCU output/display contract. |
| `return:m7` | User → Bike: `close` | The sequence does not define a button gesture or other physical input that means `close`; the existing button distinguishes tap/rent and long-press/return only. |
| `return:m8` | Bike → User: `ride ended` | This acknowledgment depends on a real `return:m7` sender/handler and its trigger semantics; implementing it without that decision would imply an unmodeled successful close. |

These are not marked implemented merely because corresponding frame IDs exist
in `model/protocol.json`. No protocol frame, model entry, or sequence flow has
been changed to hide the gap.

## Decision requested

Specify whether these interactions should remain unmapped in firmware, or
provide the actual MCU endpoint and input/output contract for each. In
particular, decide whether payment/receipt interactions belong outside the
Sensor/Actuator firmware, and define the physical input that represents
`return:m7 close` if it is to be implemented. Do not begin Group 2 or Group 3
until this is resolved.
