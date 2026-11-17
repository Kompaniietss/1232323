# IoT Extension Overview

This repository keeps the existing TypeScript e-bike rental app as the reference implementation for the domain logic while adding a constrained C++ firmware layer for one Arduino Mega 2560. The source of truth for the extension lives in the `model/` folder and must be generated or validated against the actual C++ sources.

## Roles and wiring

- Sensor node: a state-dependent button and USB Serial user terminal. An idle short press requests rent; an idle long press requests return. While awaiting simulated payment, a short press confirms it; after a return receipt, a short press closes the flow. A double press after a rent receipt requests it again.
- Actuator node: owns the Bike state machine, reservation timeout, low-battery guard, pricing logic, and LED outputs.
- Link: the only communication path between the two nodes. It is modeled as a framed UART channel.

USB Serial runs at 115200 baud and is the User terminal for payment prompts and
receipts. Payment is simulated: no payment provider is contacted or charged.
The return prompt says this explicitly; a short button press confirms the
simulation. The double-press window for `rent:m7` is 350 ms.

The intended Mega wiring is:

- Button to digital pin 2, pulldown to GND
- Green LED on pin 5
- Yellow LED on pin 6
- Red LED on pin 7
- Serial1 TX/RX tied to Serial2 RX/TX across the two logical nodes

## Model extraction and deterministic verification

The firmware uses a constrained tag style for extraction scripts so they can confirm that every `@model` declaration maps to a real C++ symbol or transition. Example tags include:

```cpp
// @model Bike.unlock()
// @model state:Reserved
// @model t3
// @model m4
```

The extractor validates the tag against a real signature, enum member, state name, or protocol mapping. The generated reports are deterministic and sorted by key so the extracted files are reproducible.

## File layout

- `model/` contains the canonical JSON model and schemas.
- `firmware/lib/domain/` contains pure C++ domain classes and generated transition tables.
- `firmware/lib/protocol/` contains frame encoding, checksum logic, and generated frame IDs.
- `firmware/lib/hal/` holds interface abstractions and fake/Arduino implementations.
- `firmware/lib/nodes/` contains the sensor and actuator node logic.
- `tools/` contains the deterministic generation and verification scripts.

## Validation

Core validation is built around JSON model checks, generated files, and deterministic extraction comparisons. The repository still runs the original TypeScript checks that validate the web app and the UML-generated diagrams.
