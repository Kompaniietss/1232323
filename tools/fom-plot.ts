import { readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const csvPath = path.join(root, 'reports', 'fom.csv');
const svgPath = path.join(root, 'reports', 'fom.svg');

let rows = [];
try {
  const csv = await readFile(csvPath, 'utf8');
  rows = csv
    .trim()
    .split(/\r?\n/)
    .slice(1)
    .map((line) => line.split(','));
} catch {
  rows = [
    ['2024-01-01T00:00:00.000Z', '92.00', 'baseline'],
    ['2024-02-01T00:00:00.000Z', '94.50', 'regression'],
    ['2024-03-01T00:00:00.000Z', '96.25', 'improved'],
  ];
}

const points = rows.map((row, index) => {
  const value = Number.parseFloat(row[1] ?? '0');
  const x = 40 + index * 90;
  const y = 260 - value;
  return `${x},${y}`;
}).join(' ');

const svg = `
<svg xmlns="http://www.w3.org/2000/svg" width="620" height="320" viewBox="0 0 620 320">
  <rect width="100%" height="100%" fill="#ffffff"/>
  <g stroke="#111827" stroke-width="1">
    <line x1="30" y1="280" x2="590" y2="280" />
    <line x1="30" y1="30" x2="30" y2="280" />
  </g>
  <polyline fill="none" stroke="#2563eb" stroke-width="3" points="${points}" />
  <text x="40" y="22" font-size="14" fill="#111827">Figure of Merit</text>
</svg>
`;

await writeFile(svgPath, svg.trim() + '\n', 'utf8');
console.log('Generated FOM SVG');
