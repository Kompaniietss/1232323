import { readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const csvPath = path.join(root, 'reports', 'fom.csv');
const svgPath = path.join(root, 'reports', 'fom.svg');
const columns = [
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
  'notes',
];
type Panel = { x: number; y: number; width: number; height: number };

function parseCsv(text: string): string[][] {
  const rows: string[][] = [];
  let row: string[] = [];
  let cell = '';
  let quoted = false;

  for (let index = 0; index < text.length; index += 1) {
    const character = text[index];
    if (quoted) {
      if (character === '"' && text[index + 1] === '"') {
        cell += '"';
        index += 1;
      } else if (character === '"') {
        quoted = false;
      } else {
        cell += character;
      }
    } else if (character === '"') {
      if (cell.length !== 0) throw new Error('Malformed CSV: quote in an unquoted field');
      quoted = true;
    } else if (character === ',') {
      row.push(cell);
      cell = '';
    } else if (character === '\n' || character === '\r') {
      row.push(cell);
      rows.push(row);
      row = [];
      cell = '';
      if (character === '\r' && text[index + 1] === '\n') index += 1;
    } else {
      cell += character;
    }
  }

  if (quoted) throw new Error('Malformed CSV: unterminated quoted field');
  if (cell.length > 0 || row.length > 0) {
    row.push(cell);
    rows.push(row);
  }
  return rows;
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

function xAt(index: number, rowCount: number, panel: Panel): number {
  return rowCount === 1
    ? panel.x + panel.width / 2
    : panel.x + index * panel.width / (rowCount - 1);
}

function yAt(value: number, panel: Panel, maxValue: number): number {
  return panel.y + panel.height - Math.max(0, Math.min(value, maxValue)) / maxValue * panel.height;
}

function seriesPoints(rows: string[][], column: number, panel: Panel, maxValue: number): string {
  return rows.flatMap((row, index) => {
    const value = csvNumber(row, column);
    return value === null ? [] : [`${xAt(index, rows.length, panel).toFixed(1)},${yAt(value, panel, maxValue).toFixed(1)}`];
  }).join(' ');
}

function seriesMarkers(rows: string[][], column: number, panel: Panel, maxValue: number, color: string): string {
  return rows.flatMap((row, index) => {
    const value = csvNumber(row, column);
    if (value === null) return [];
    return [`<circle cx="${xAt(index, rows.length, panel).toFixed(1)}" cy="${yAt(value, panel, maxValue).toFixed(1)}" r="4" fill="${color}"/>`];
  }).join('\n');
}

function panelAxes(panel: Panel, maxValue: number, step: number): string {
  const lines: string[] = [];
  for (let value = 0; value <= maxValue; value += step) {
    const y = yAt(value, panel, maxValue);
    lines.push(`<line x1="${panel.x}" y1="${y}" x2="${panel.x + panel.width}" y2="${y}" stroke="#d1d5db"/>`);
    lines.push(`<text x="${panel.x - 12}" y="${y + 4}" text-anchor="end" font-size="12" fill="#374151">${value}</text>`);
  }
  lines.push(`<line x1="${panel.x}" y1="${panel.y}" x2="${panel.x}" y2="${panel.y + panel.height}" stroke="#111827"/>`);
  lines.push(`<line x1="${panel.x}" y1="${panel.y + panel.height}" x2="${panel.x + panel.width}" y2="${panel.y + panel.height}" stroke="#111827"/>`);
  return lines.join('\n');
}

function commitLabels(rows: string[][], panel: Panel): string {
  const y = panel.y + panel.height + 24;
  return rows.map((row, index) => {
    const x = xAt(index, rows.length, panel);
    return `<text x="${x}" y="${y}" text-anchor="end" transform="rotate(-35 ${x} ${y})" font-size="11" fill="#374151">${xml(row[0] ?? '')}</text>`;
  }).join('\n');
}

async function main(): Promise<void> {
  let csv: string;
  try {
    csv = await readFile(csvPath, 'utf8');
  } catch (error) {
    if (typeof error === 'object' && error !== null && 'code' in error && error.code === 'ENOENT') {
      throw new Error('reports/fom.csv does not exist', { cause: error });
    }
    throw error;
  }

  const parsedRows = parseCsv(csv.replace(/^\uFEFF/, ''));
  const header = parsedRows.shift();
  if (header?.join(',') !== columns.join(',')) {
    throw new Error('reports/fom.csv has an unexpected header');
  }
  if (parsedRows.some((row) => row.length !== columns.length)) {
    throw new Error('reports/fom.csv contains a row with an unexpected number of columns');
  }

  const rows = parsedRows;
  const width = 1100;
  const height = 750;
  const upper: Panel = { x: 90, y: 80, width: 970, height: 190 };
  const lower: Panel = { x: 90, y: 410, width: 970, height: 190 };
  const flashValues = rows.map((row) => csvNumber(row, 7)).filter((value): value is number => value !== null);
  const ramValues = rows.map((row) => csvNumber(row, 8)).filter((value): value is number => value !== null);
  const maxBytes = Math.max(0, ...flashValues, ...ramValues);
  const bytesStep = maxBytes <= 100 ? 20 : Math.ceil(maxBytes / 5 / 100) * 100;
  const bytesAxisMax = Math.max(bytesStep, Math.ceil(maxBytes / bytesStep) * bytesStep);
  const noGapValues = rows.every((row) => csvNumber(row, 3) === null);
  const noMemoryValues = flashValues.length === 0 && ramValues.length === 0;
  const xLabels = commitLabels(rows, lower);

  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}">
<rect width="100%" height="100%" fill="#ffffff"/>
<text x="90" y="35" font-size="20" font-weight="bold" fill="#111827">Firmware Figure of Merit</text>
<text x="90" y="62" font-size="15" fill="#111827">Model gap (%) by commit</text>
${panelAxes(upper, 100, 25)}
<polyline fill="none" stroke="#2563eb" stroke-width="3" points="${seriesPoints(rows, 3, upper, 100)}"/>
${seriesMarkers(rows, 3, upper, 100, '#2563eb')}
${noGapValues ? `<text x="${width / 2}" y="${upper.y + upper.height / 2}" text-anchor="middle" font-size="14" fill="#6b7280">No gap measurements</text>` : ''}
<text x="90" y="365" font-size="15" fill="#111827">Flash / RAM (bytes) by commit</text>
${panelAxes(lower, bytesAxisMax, bytesStep)}
<polyline fill="none" stroke="#7c3aed" stroke-width="3" points="${seriesPoints(rows, 7, lower, bytesAxisMax)}"/>
<polyline fill="none" stroke="#f97316" stroke-width="3" points="${seriesPoints(rows, 8, lower, bytesAxisMax)}"/>
${seriesMarkers(rows, 7, lower, bytesAxisMax, '#7c3aed')}
${seriesMarkers(rows, 8, lower, bytesAxisMax, '#f97316')}
${noMemoryValues ? `<text x="${width / 2}" y="${lower.y + lower.height / 2}" text-anchor="middle" font-size="14" fill="#6b7280">No memory measurements</text>` : ''}
<line x1="90" y1="665" x2="112" y2="665" stroke="#7c3aed" stroke-width="3"/>
<text x="120" y="670" font-size="12" fill="#111827">Flash</text>
<line x1="190" y1="665" x2="212" y2="665" stroke="#f97316" stroke-width="3"/>
<text x="220" y="670" font-size="12" fill="#111827">RAM</text>
${xLabels}
<text x="575" y="738" text-anchor="middle" font-size="12" fill="#374151">Commit hash</text>
</svg>
`;

  await writeFile(svgPath, svg, 'utf8');
  console.log(`Generated ${path.relative(root, svgPath)} from ${rows.length} CSV rows`);
}

main().catch((error: unknown) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
});
