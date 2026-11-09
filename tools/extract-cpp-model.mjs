import { readFile, readdir, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const firmwareRoot = path.join(root, 'firmware');
const reportsDir = path.join(root, 'reports', 'extracted');
await mkdir(reportsDir, { recursive: true });

async function findCppFiles(dir) {
  const entries = await readdir(dir, { withFileTypes: true });
  const files = [];
  for (const entry of entries) {
    const current = path.join(dir, entry.name);
    if (entry.isDirectory()) files.push(...await findCppFiles(current));
    else if (entry.name.endsWith('.h') || entry.name.endsWith('.hpp') || entry.name.endsWith('.cpp')) files.push(current);
  }
  return files;
}

const matches = new Map();
for (const file of await findCppFiles(firmwareRoot)) {
  const text = await readFile(file, 'utf8');
  const tags = [...text.matchAll(/\/\/\s*@model\s+([^\n]+)/g)].map((match) => match[1].trim());
  if (tags.length > 0) {
    matches.set(path.relative(root, file).split(path.sep).join('/'), tags);
  }
}

const payload = {
  generatedAt: null,
  files: Object.fromEntries([...matches.entries()].sort()),
};

await writeFile(path.join(reportsDir, 'cpp-model.json'), JSON.stringify(payload, null, 2) + '\n', 'utf8');
console.log('Extracted model tags');
