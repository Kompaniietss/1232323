import { readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const csvPath = path.join(root, 'reports', 'fom.csv');
const svgPath = path.join(root, 'reports', 'fom.svg');
const expectedHeader = [
  'commit',
  'date',
  'mega_build_ok',
  'gap_pct',
  'tests_pass',
  'tests_total',
  'trace_diffs',
  'flash_bytes',
  'ram_bytes',
  'transition_us',
];

function parseCsvRow(line: string): string[] {
  const cells: string[] = [];
  let cell = '';
  let quoted = false;
  for (let index = 0; index < line.length; index += 1) {
    const character = line[index];
    if (quoted && character === '"' && line[index + 1] === '"') {
      cell += '"';
      index += 1;
    } else if (character === '"') {
      quoted = !quoted;
    } else if (character === ',' && !quoted) {
      cells.push(cell);
      cell = '';
    } else {
      cell += character;
    }
  }
  if (quoted) throw new Error('Malformed CSV: unterminated quoted field');
  cells.push(cell);
  return cells;
}

function csvNumber(row: string[], column: number): number | null {
  const value = row[column]?.trim() ?? '';
  if (!value) return null;
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : null;
}

function xml(value: string): string {
  return value.replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;').replaceAll("'", '&apos;');
}

function pointsFor(
  rows: string[][],
  valueColumn: number,
  panel: { x: number; y: number; width: number; height: number },
  maxValue: number,
): string {
  return rows.flatMap((row, index) => {
    const value = csvNumber(row, valueColumn);
    if (value === null) return [];
    const x = rows.length === 1
      ? panel.x + panel.width / 2
      : panel.x + index * panel.width / (rows.length - 1);
    const y = panel.y + panel.height - Math.max(0, Math.min(value, maxValue)) / maxValue * panel.height;
    return [`${x.toFixed(1)},${y.toFixed(1)}`];
  }).join(' ');
}

function panelAxes(panel: { x: number; y: number; width: number; height: number }, maxValue: number, step: number): string {
  const lines = [];
  for (let value = 0; value <= maxValue; value += step) {
    const y = panel.y + panel.height - value / maxValue * panel.height;
    lines.push(`<line x1="${panel.x}" y1="${y}" x2="${panel.x + panel.width}" y2="${y}" stroke="#d1d5db"/>`);
    lines.push(`<text x="${panel.x - 12}" y="${y + 4}" text-anchor="end" font-size="12" fill="#374151">${value}</text>`);
  }
  lines.push(`<line x1="${panel.x}" y1="${panel.y}" x2="${panel.x}" y2="${panel.y + panel.height}" stroke="#111827"/>`);
  lines.push(`<line x1="${panel.x}" y1="${panel.y + panel.height}" x2="${panel.x + panel.width}" y2="${panel.y + panel.height}" stroke="#111827"/>`);
  return lines.join('\n');
}

const csv = await readFile(csvPath, 'utf8');
const lines = csv.trimEnd().split(/\r?\n/);
if (lines.length === 0 || parseCsvRow(lines[0]).join(',') !== expectedHeader.join(',')) {
  throw new Error('reports/fom.csv is missing or has an unexpected header');
}
const rows = lines.slice(1).filter(Boolean).map(parseCsvRow);
if (rows.some((row) => row.length !== expectedHeader.length)) {
  throw new Error('reports/fom.csv contains a row with an unexpected number of columns');
}

const width = 1100;
const height = 720;
const upper = { x: 90, y: 80, width: 970, height: 210 };
const lower = { x: 90, y: 405, width: 970, height: 210 };
const flashValues = rows.map((row) => csvNumber(row, 7)).filter((value): value is number => value !== null);
const ramValues = rows.map((row) => csvNumber(row, 8)).filter((value): value is number => value !== null);
const maximumBytes = Math.max(1, ...flashValues, ...ramValues);
const bytesStep = maximumBytes <= 100 ? 20 : Math.ceil(maximumBytes / 5 / 100) * 100;
const bytesAxisMax = Math.max(bytesStep, Math.ceil(maximumBytes / bytesStep) * bytesStep);

const xLabels = rows.map((row, index) => {
  const x = rows.length === 1
    ? upper.x + upper.width / 2
    : upper.x + index * upper.width / (rows.length - 1);
  const y = lower.y + lower.height + 22;
  return `<text x="${x}" y="${y}" text-anchor="end" transform="rotate(-35 ${x} ${y})" font-size="11" fill="#374151">${xml(row[0] ?? '')}</text>`;
}).join('\n');
const emptyMessage = rows.length === 0
  ? '<text x="575" y="188" text-anchor="middle" font-size="14" fill="#6b7280">No measurements in CSV</text>'
  : '';

const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}">
<rect width="100%" height="100%" fill="#ffffff"/>
<text x="90" y="35" font-size="20" font-weight="bold" fill="#111827">Firmware Figure of Merit</text>
<text x="90" y="62" font-size="15" fill="#111827">Model gap (%) by commit index</text>
${panelAxes(upper, 100, 25)}
<polyline fill="none" stroke="#2563eb" stroke-width="3" points="${pointsFor(rows, 3, upper, 100)}"/>
${emptyMessage}
${rows.map((row, index) => {
  const value = csvNumber(row, 3);
  if (value === null) return '';
  const x = rows.length === 1 ? upper.x + upper.width / 2 : upper.x + index * upper.width / (rows.length - 1);
  const y = upper.y + upper.height - Math.max(0, Math.min(value, 100)) / 100 * upper.height;
  return `<circle cx="${x}" cy="${y}" r="4" fill="#2563eb"/>`;
}).join('\n')}
<text x="90" y="355" font-size="15" fill="#111827">Flash / RAM (bytes) by commit index</text>
${panelAxes(lower, bytesAxisMax, bytesStep)}
<polyline fill="none" stroke="#7c3aed" stroke-width="3" points="${pointsFor(rows, 7, lower, bytesAxisMax)}"/>
<polyline fill="none" stroke="#f97316" stroke-width="3" points="${pointsFor(rows, 8, lower, bytesAxisMax)}"/>
${rows.map((row, index) => {
  const x = rows.length === 1 ? lower.x + lower.width / 2 : lower.x + index * lower.width / (rows.length - 1);
  return [7, 8].map((column, series) => {
    const value = csvNumber(row, column);
    if (value === null) return '';
    const y = lower.y + lower.height - Math.max(0, Math.min(value, bytesAxisMax)) / bytesAxisMax * lower.height;
    return `<circle cx="${x}" cy="${y}" r="4" fill="${series === 0 ? '#7c3aed' : '#f97316'}"/>`;
  }).join('\n');
}).join('\n')}
<line x1="90" y1="665" x2="112" y2="665" stroke="#7c3aed" stroke-width="3"/>
<text x="120" y="670" font-size="12" fill="#111827">Flash</text>
<line x1="190" y1="665" x2="212" y2="665" stroke="#f97316" stroke-width="3"/>
<text x="220" y="670" font-size="12" fill="#111827">RAM</text>
${xLabels}
<text x="575" y="710" text-anchor="middle" font-size="12" fill="#374151">Commit index (short hash)</text>
</svg>
`;

await writeFile(svgPath, svg, 'utf8');
console.log(`Generated ${path.relative(root, svgPath)} from ${rows.length} CSV rows`);
