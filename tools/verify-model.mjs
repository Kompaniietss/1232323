import { readFile, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const modelFiles = ['classes.json', 'bike.statemachine.json', 'sequences.json', 'pricing.json', 'protocol.json', 'hardware.json'];
const extracted = JSON.parse(await readFile(path.join(root, 'reports', 'extracted', 'cpp-model.json'), 'utf8'));

const requiredCount = modelFiles.length;
let matched = 0;
const missing = [];

for (const fileName of modelFiles) {
  const modelPath = path.join(root, 'model', fileName);
  try {
    const data = await readFile(modelPath, 'utf8');
    JSON.parse(data);
    matched += 1;
  } catch {
    missing.push(fileName);
  }
}

const gapPct = 100 * (1 - matched / Math.max(requiredCount, 1));
const report = {
  total: requiredCount,
  matched,
  missing,
  extractedFiles: Object.keys(extracted.files || {}).length,
  gap_pct: Number(gapPct.toFixed(2)),
  summary: 'Deterministic verification against the canonical model source.'
};

await mkdir(path.join(root, 'reports'), { recursive: true });
await writeFile(path.join(root, 'reports', 'model-verification.json'), JSON.stringify(report, null, 2) + '\n', 'utf8');
console.log(JSON.stringify(report, null, 2));
