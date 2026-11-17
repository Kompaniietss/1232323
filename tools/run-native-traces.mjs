import console from 'node:console';
import { mkdir, readFile, readdir, unlink, writeFile } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import os from 'node:os';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const scenarioPath = path.join(root, 'tools', 'trace-scenarios.json');
const scenarioDefinitions = JSON.parse(await readFile(scenarioPath, 'utf8')).scenarios;
const traceSource = path.join(root, 'tools', 'native-trace.cpp');
const nativeDir = path.join(root, 'reports', 'traces-cpp');
const binaryPath = path.join(os.tmpdir(), `ebike-native-trace-${process.pid}${process.platform === 'win32' ? '.exe' : ''}`);

function run(command, args) {
  const result = spawnSync(command, args, {
    cwd: root,
    encoding: 'utf8',
    windowsHide: true,
    maxBuffer: 16 * 1024 * 1024,
  });
  if (result.error || result.status !== 0) {
    throw new Error(
      `${command} ${args.join(' ')} failed: ${result.stderr?.trim() || result.error?.message || result.stdout?.trim() || `exit ${result.status}`}`,
    );
  }
  return result.stdout;
}

function validateNativeOutput(output, definition) {
  const trace = JSON.parse(output);
  if (trace.scenario !== definition.name || !Array.isArray(trace.steps) ||
      trace.steps.length !== definition.inputs.length) {
    throw new Error(`Native trace output has an invalid shape for ${definition.name}`);
  }
  for (let index = 0; index < trace.steps.length; index += 1) {
    const step = trace.steps[index];
    if (step.input !== definition.inputs[index] ||
        !['Available', 'Reserved', 'InUse', 'Maintenance'].includes(step.stateBefore) ||
        !['Available', 'Reserved', 'InUse', 'Maintenance'].includes(step.stateAfter) ||
        !(step.frameId === null || (Number.isInteger(step.frameId) && step.frameId >= 0 && step.frameId <= 255)) ||
        !['Green', 'Yellow', 'Red'].includes(step.ledColor) ||
        !(step.costCents === null || Number.isInteger(step.costCents))) {
      throw new Error(`Native trace step ${index} has an invalid shape for ${definition.name}`);
    }
    const keys = Object.keys(step).sort();
    if (keys.join(',') !== 'costCents,frameId,input,ledColor,stateAfter,stateBefore') {
      throw new Error(`Native trace step ${index} has unexpected fields for ${definition.name}`);
    }
  }
  return trace;
}

try {
  run('g++', ['-std=c++17', traceSource, '-o', binaryPath]);
  const outputs = [];
  for (const definition of scenarioDefinitions) {
    const stdout = run(binaryPath, [
      definition.name,
      definition.initialState,
      String(definition.batteryLevel),
      String(definition.durationMs),
      ...definition.inputs,
    ]);
    validateNativeOutput(stdout, definition);
    outputs.push({ name: definition.name, stdout });
  }

  await mkdir(nativeDir, { recursive: true });
  const currentFiles = new Set(scenarioDefinitions.map(({ name }) => `${name}.json`));
  for (const file of await readdir(nativeDir)) {
    if (file.endsWith('.json') && !currentFiles.has(file)) {
      await unlink(path.join(nativeDir, file));
    }
  }
  for (const { name, stdout } of outputs) {
    await writeFile(path.join(nativeDir, `${name}.json`), stdout.endsWith('\n') ? stdout : `${stdout}\n`, 'utf8');
  }
  console.log(`Generated ${outputs.length} C++ execution traces`);
} finally {
  await unlink(binaryPath).catch((error) => {
    if (error.code !== 'ENOENT') throw error;
  });
}
