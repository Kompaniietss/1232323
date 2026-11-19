import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadModel, summarizeModel } from './model-elements.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

test('reports UML sequence conformance independently from the IoT model gap', async () => {
  const catalog = await loadModel(root);
  const report = summarizeModel(catalog, { elements: [] });

  assert.equal(report.gap_pct, 100);
  assert.equal(report.gapVsIotModel, report.gap_pct);
  assert.deepEqual(
    Object.fromEntries(
      ['total', 'aligned', 'deviated', 'unmapped'].map((key) => [
        key,
        report.umlConformance[key],
      ]),
    ),
    {
      total: 17,
      aligned: { count: 5, pct: 29.41 },
      deviated: { count: 9, pct: 52.94 },
      unmapped: { count: 3, pct: 17.65 },
    },
  );
  assert.equal(report.umlConformance.nonAligned.length, 12);
  assert.equal(
    report.umlConformance.nonAligned.some(
      (message) =>
        message.flow === 'return' && message.id === 'm5' && message.status === 'unmapped',
    ),
    true,
  );
});
