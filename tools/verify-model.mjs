import { readFile, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadModel, sortObjectKeys, summarizeModel } from './model-elements.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const extracted = JSON.parse(await readFile(path.join(root, 'reports', 'extracted', 'cpp-model.json'), 'utf8'));
const catalog = await loadModel(root);
const report = summarizeModel(catalog, extracted);

await mkdir(path.join(root, 'reports'), { recursive: true });
const output = sortObjectKeys(report);
await writeFile(path.join(root, 'reports', 'model-verification.json'), JSON.stringify(output, null, 2) + '\n', 'utf8');
console.log(JSON.stringify(output, null, 2));
