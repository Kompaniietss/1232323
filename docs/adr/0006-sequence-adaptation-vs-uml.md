# ADR 0006: Firmware sequence adaptation versus specification UML

- **Status:** PROPOSED
- **Date:** 2026-11-19
- **Approval:** None. This ADR records a proposal for explicit user review; it
  is not an accepted specification change.

## Context

Specification sections 6.1–6.2 define the UML rent and return messages and
participants. `model/sequences.json` is a separate IoT-adapted model for the
Mega firmware. Its participants, message names, and ordering differ from the
specification. The existing sequence-message gap therefore measures only
conformance to that IoT model, not to the specification UML. This proposal
keeps both models visible and reports firmware conformance to them separately.

The UML-to-firmware mapping records messages as aligned only when a named
firmware frame or handler has the same meaning and the participating roles
correspond. Deviations and messages without MCU counterparts remain explicit;
they are not folded into the existing IoT-model gap.

## Proposed decision

- Keep `model/sequences.json` as the IoT-adapted firmware sequence model.
- Preserve specification messages separately in `model/uml-sequences.json`.
- Maintain a per-message status and named firmware reference in
  `model/uml-mapping.json`.
- Report UML sequence conformance separately from the existing `gap_pct`.

## Recorded adaptations and deviations

- The firmware inserts reservation handling (`RENT_REQUEST`,
  `Station::releaseBike()`, and `RENT_RESERVED`) before unlocking. FR-007
  forbids adding a reservation step to the web application's rent flow; this
  ADR does not change that requirement or claim the firmware behavior is UML
  aligned.
- `PaymentService` is a UML sequence participant but is not present on the
  Mega. Firmware payment confirmation is simulated by a button; no provider is
  contacted and no funds are charged. The `charge(cost)` and `paid` messages
  have no MCU counterpart.
- The UML `App` participant is split across `SensorNode` and `ActuatorNode`.
  The Sensor handles the button and user terminal; the Actuator handles bike,
  station, and rental operations. This is an implementation adaptation, not a
  UML participant rename or proof of alignment by itself.
- Other message-level differences and unmapped user-visible rent feedback are
  listed in the generated `umlConformance.nonAligned` report.

## Consequences

- `gap_pct` retains its existing meaning against the IoT-adapted model and is
  duplicated as `gapVsIotModel` for clarity.
- `umlConformance` reports counts, percentages, and every deviated or unmapped
  UML message; the FOM logger records the aligned percentage when available.
- The source specification and `model/sequences.json` remain unchanged.

## Decision required

The user must explicitly accept, revise, or reject this proposal before it is
treated as an approved model interpretation. No implementation phase or
diagram-defined UML change is approved by this proposed ADR.
