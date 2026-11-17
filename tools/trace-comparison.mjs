import { isDeepStrictEqual } from 'node:util';

export function compareTraceSteps(golden, native) {
  const differences = [];
  const stepCount = Math.max(golden.steps.length, native.steps.length);
  for (let index = 0; index < stepCount; index += 1) {
    const goldenStep = golden.steps[index] ?? null;
    const nativeStep = native.steps[index] ?? null;
    if (!isDeepStrictEqual(goldenStep, nativeStep)) {
      differences.push({ step: index, golden: goldenStep, native: nativeStep });
    }
  }
  return differences;
}

export function validateTrace(trace, scenarioName) {
  if (!trace || typeof trace !== 'object' || Array.isArray(trace) ||
      trace.scenario !== scenarioName || !Array.isArray(trace.steps) ||
      Object.keys(trace).sort().join(',') !== 'scenario,steps') {
    throw new Error(`Invalid trace object for ${scenarioName}`);
  }
  for (const [index, step] of trace.steps.entries()) {
    if (!step || typeof step !== 'object' || Array.isArray(step) ||
        Object.keys(step).sort().join(',') !== 'costCents,frameId,input,ledColor,stateAfter,stateBefore' ||
        typeof step.input !== 'string' ||
        !['Available', 'Reserved', 'InUse', 'Maintenance'].includes(step.stateBefore) ||
        !['Available', 'Reserved', 'InUse', 'Maintenance'].includes(step.stateAfter) ||
        !(step.frameId === null || (Number.isInteger(step.frameId) && step.frameId >= 0 && step.frameId <= 255)) ||
        !(step.ledColor === null || ['Green', 'Yellow', 'Red'].includes(step.ledColor)) ||
        !(step.costCents === null || Number.isInteger(step.costCents))) {
      throw new Error(`Invalid trace step ${index} for ${scenarioName}`);
    }
  }
}
