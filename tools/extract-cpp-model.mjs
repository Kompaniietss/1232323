import { readFile, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { extractTaggedElements, findSourceFiles, loadModel, sortObjectKeys } from './model-elements.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const firmwareRoot = path.join(root, 'firmware', 'lib');
const reportsDir = path.join(root, 'reports', 'extracted');
await mkdir(reportsDir, { recursive: true });

const catalog = await loadModel(root);
const files = await Promise.all((await findSourceFiles(firmwareRoot)).map(async (file) => ({
  relativePath: path.relative(root, file).split(path.sep).join('/'),
  source: await readFile(file, 'utf8'),
})));
const extracted = extractTaggedElements(files, catalog);
const payload = {
  elements: extracted.elements,
  files: extracted.tagsByFile,
};

await writeFile(path.join(reportsDir, 'cpp-model.json'), JSON.stringify(sortObjectKeys(payload), null, 2) + '\n', 'utf8');
console.log(`Extracted ${extracted.elements.length} implemented model elements`);
