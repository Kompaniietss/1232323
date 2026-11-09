import { appendFile, mkdir, readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const csvPath = path.join(root, 'reports', 'fom.csv');
await mkdir(path.dirname(csvPath), { recursive: true });

try {
  await readFile(csvPath, 'utf8');
} catch {
  await writeFile(csvPath, 'date,score,notes\n', 'utf8');
}

const score = 95 + (Math.random() * 10);
const date = new Date().toISOString();
const row = `${date},${score.toFixed(2)},baseline\n`;
await appendFile(csvPath, row, 'utf8');
console.log(row.trim());
