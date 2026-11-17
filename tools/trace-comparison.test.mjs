import test from 'node:test';
import assert from 'node:assert/strict';
import { compareTraceSteps, validateTrace } from './trace-comparison.mjs';

const step = {
  input: 'rentBike',
  stateBefore: 'Available',
  stateAfter: 'InUse',
  frameId: null,
  ledColor: null,
  costCents: null,
};

test('equal trace steps have no differences', () => {
  assert.deepEqual(compareTraceSteps({ steps: [step] }, { steps: [{ ...step }] }), []);
});

test('reports changed and missing steps individually', () => {
  const nativeStep = { ...step, stateAfter: 'Available', costCents: 25 };
  const differences = compareTraceSteps(
    { steps: [step, step] },
    { steps: [nativeStep] },
  );
  assert.equal(differences.length, 2);
  assert.deepEqual(differences.map(({ step: index }) => index), [0, 1]);
});

test('validates the shared trace and step shape', () => {
  validateTrace({ scenario: 'rent', steps: [step] }, 'rent');
  assert.throws(() => validateTrace({ scenario: 'rent', steps: [{ ...step, matched: true }] }, 'rent'));
});
