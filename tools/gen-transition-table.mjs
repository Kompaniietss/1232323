import { readFile, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const model = JSON.parse(await readFile(path.join(root, 'model', 'bike.statemachine.json'), 'utf8'));
const outputDir = path.join(root, 'firmware', 'lib', 'domain', 'generated');
await mkdir(outputDir, { recursive: true });

const stateMap = new Map([
  ['Available', 'BikeState::Available'],
  ['Reserved', 'BikeState::Reserved'],
  ['InUse', 'BikeState::InUse'],
  ['Maintenance', 'BikeState::Maintenance'],
]);

const triggerMap = new Map([
  ['reserve()', 'BikeTrigger::Reserve'],
  ['timeout', 'BikeTrigger::Timeout'],
  ['cancel()', 'BikeTrigger::Cancel'],
  ['unlock()', 'BikeTrigger::Unlock'],
  ['returnToStation()', 'BikeTrigger::ReturnToStation'],
  ['reportFault()', 'BikeTrigger::ReportFault'],
  ['batteryLow()', 'BikeTrigger::BatteryLow'],
  ['repaired()', 'BikeTrigger::Repaired'],
]);

const entries = model.transitions
  .filter((t) => t.id && t.from && t.to && t.trigger)
  .map((t) => `  // @model transition:${t.id}:${t.trigger}\n  {"${t.id}", ${stateMap.get(t.from)}, ${stateMap.get(t.to)}, ${triggerMap.get(t.trigger)} }`)
  .join(',\n');
const states = model.states.map((state) => `  // @model state:${state}\n  ${state}`).join(',\n');

const header = `#pragma once
#include <stdint.h>
#include "../Progmem.h"

namespace firmware {

enum class BikeState : uint8_t {
${states}
};

enum class BikeTrigger : uint8_t {
  Reserve,
  Timeout,
  Cancel,
  Unlock,
  ReturnToStation,
  ReportFault,
  BatteryLow,
  Repaired
};

struct TransitionEntry {
  const char* id;
  BikeState from;
  BikeState to;
  BikeTrigger trigger;
};

inline constexpr TransitionEntry kBikeTransitions[] FIRMWARE_PROGMEM = {
${entries}
};

}  // namespace firmware
`;

await writeFile(path.join(outputDir, 'transition_table.h'), header, 'utf8');
console.log('Generated transition table');
