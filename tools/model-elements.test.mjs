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
      aligned: { count: 4, pct: 23.53 },
      deviated: { count: 10, pct: 58.82 },
      unmapped: { count: 3, pct: 17.65 },
    },
  );
  assert.equal(report.umlConformance.nonAligned.length, 13);
  assert.equal(
    report.umlConformance.nonAligned.some(
      (message) =>
        message.flow === 'return' && message.id === 'm5' && message.status === 'unmapped',
    ),
    true,
  );
  assert.equal(report.umlConformance.extras.length, 6);
  assert.deepEqual(
    report.umlConformance.extras
      .filter((extra) => extra.type === 'unconsumedFrame')
      .map((extra) => extra.id),
    [
      'unconsumed-frame:RENT_STARTED',
      'unconsumed-frame:RENT_ACTIVE',
      'unconsumed-frame:RETURN_AVAILABLE',
      'unconsumed-frame:RETURN_DOCKED',
    ],
  );
  const docked = catalog.umlMapping.return.find((mapping) => mapping.id === 'm3');
  assert.equal(docked.status, 'deviated');
  assert.match(docked.reason, /SensorNode never handles that frame/);
});
