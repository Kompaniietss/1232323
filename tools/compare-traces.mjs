import console from 'node:console';
import { readFile } from 'node:fs/promises';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';
import { compareTraceSteps, validateTrace } from './trace-comparison.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const definitions = JSON.parse(
  await readFile(path.join(root, 'tools', 'trace-scenarios.json'), 'utf8'),
).scenarios;
let traceDiffs = 0;
const scenarioResults = [];

for (const { name } of definitions) {
  const golden = JSON.parse(await readFile(path.join(root, 'reports', 'traces', `${name}.json`), 'utf8'));
  const native = JSON.parse(await readFile(path.join(root, 'reports', 'traces-cpp', `${name}.json`), 'utf8'));
  validateTrace(golden, name);
  validateTrace(native, name);
  const differences = compareTraceSteps(golden, native);
  traceDiffs += differences.length;
  scenarioResults.push({ scenario: name, differences });
}

console.log(JSON.stringify({ trace_diffs: traceDiffs, scenarios: scenarioResults }, null, 2));
if (traceDiffs > 0) process.exitCode = 1;
