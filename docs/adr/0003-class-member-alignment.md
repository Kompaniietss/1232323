# ADR 0003: Class member alignment and firmware storage

- **Status:** Accepted
- **Date:** 2026-11-17
- **Approval:** Decisions explicitly supplied by the user on 2026-11-17.

## Context

The class diagram and the generated class diagram show `User` with three
attributes and no operations. `model/classes.json` incorrectly included a
`User.constructor` operation. The model also declares `Rental.cost` as a
number, while the firmware needs integer minor-unit arithmetic and AVR's plain
`int` is only 16 bits. The domain firmware must not use heap allocation or STL.

The class diagram defines `Station.dockBike` and `Station.releaseBike`, and
relationship `e3` says a station holds zero or more bikes. The previous
firmware methods were nonfunctional stubs.

## Decision

- Remove only `User.constructor` from `model/classes.json`; retain all other
  model entries unchanged.
- Keep firmware rental cost in integer cents as `int32_t costCents`, tagged
  `// @model Rental.cost`. `Pricing::calculate` returns `int32_t`, with a
  compile-time assertion that `int32_t` is at least four bytes. The conceptual
  `Rental.cost` value remains numeric; this is the firmware's internal
  minor-unit representation.
- Implement station inventory with a fixed array of ten `Bike*` entries, with
  no allocation or STL. Ten is the demo station capacity used by the
  specification. `dockBike` ignores null and duplicate pointers and leaves
  inventory unchanged when full; `releaseBike` returns and removes the oldest
  docked bike, or `nullptr` when empty. The pointer array is the implementation
  of the existing `e3` relationship, not an additional domain relationship.

## Consequences

- The class model total decreases from 70 to 69. Therefore the overall
  `gap_pct` before and after this correction is not strictly comparable.
- Firmware money arithmetic is safe from 16-bit `int` truncation for ordinary
  modeled rental durations; a 37-hour duration is tested.
- Station inventory is bounded to ten bikes by the firmware representation.
  Configured capacity above ten is effectively capped at ten. Behavior at
  capacity and duplicate docking is an implementation rule required by the
  void API, not a rule explicitly defined by the UML; it remains a documented
  assumption.
- The model extractor maps the `Rental.cost` tag to the real C++ member
  `costCents` and validates the member declaration, so the tag alone cannot
  produce a match.

## Alternatives

- Keep the User constructor entry: rejected because the class diagram and
  specification show no User operations.
- Store cents in plain `int`: rejected because AVR `int` may be 16 bits.
- Use heap allocation or STL for station inventory: rejected by the firmware
  constraints.
- Store only one station bike: rejected because it cannot represent the
  existing `0..*` bike relationship.

## Affected elements

- `class.drawio`: `User` and its operation inventory; `Rental.cost`;
  `Station.dockBike`; `Station.releaseBike`; relationship `e3`.
- `model/classes.json`: removes only `User.constructor`; total becomes 69.
- The source diagram remains unchanged.

## Verification/reporting impact

Run `npm run model:verify`, `pio test -e native`, and `pio run -e mega`.
Report the new `gap_pct`, individual Unity result lines, and Mega RAM/Flash.

