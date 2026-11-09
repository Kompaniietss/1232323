import { appendFile, mkdir, readFile, writeFile } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const csvPath = path.join(root, 'reports', 'fom.csv');
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
];

function run(command: string, args: string[], cwd: string) {
  return spawnSync(command, args, { cwd, encoding: 'utf8', windowsHide: true });
}

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

function serializeCsvRow(cells: string[]): string {
  return `${cells.map((cell) => /[",\r\n]/.test(cell)
    ? `"${cell.replaceAll('"', '""')}"`
    : cell).join(',')}\n`;
}

function gitCommit(): string {
  const result = run('git', ['rev-parse', '--short', 'HEAD'], root);
  if (result.status !== 0 || !result.stdout.trim()) {
    throw new Error(`Could not read HEAD commit: ${result.stderr.trim() || result.error?.message || 'git failed'}`);
  }
  return result.stdout.trim();
}

async function readGapPct(): Promise<string> {
  try {
    const verification = JSON.parse(
      await readFile(path.join(root, 'reports', 'model-verification.json'), 'utf8'),
    ) as { gap_pct?: unknown };
    return typeof verification.gap_pct === 'number' && Number.isFinite(verification.gap_pct)
      ? String(verification.gap_pct)
      : '';
  } catch (error) {
    if (isMissingFile(error)) return '';
    throw error;
  }
}

function isMissingFile(error: unknown): boolean {
  return typeof error === 'object' && error !== null && 'code' in error && error.code === 'ENOENT';
}

function parseBuildMemory(output: string, label: 'RAM' | 'Flash'): string {
  const cleanOutput = output.replace(/\u001b\[[0-9;]*m/g, '');
  const result = new RegExp(`${label}:\\s*\\[[^\\]]*\\]\\s*[\\d.]+%\\s*\\(used\\s+(\\d+)\\s+bytes\\s+from\\s+\\d+\\s+bytes\\)`).exec(cleanOutput);
  return result?.[1] ?? '';
}

const commit = gitCommit();
let existing = '';
try {
  existing = await readFile(csvPath, 'utf8');
} catch (error) {
  if (!isMissingFile(error)) throw error;
}

if (existing) {
  const lines = existing.trimEnd().split(/\r?\n/);
  if (parseCsvRow(lines[0]).join(',') !== columns.join(',')) {
    throw new Error('reports/fom.csv has an unexpected header');
  }
  const previous = lines.slice(1).filter(Boolean).map(parseCsvRow);
  if (previous.some((row) => row[0] === commit)) {
    console.log(`FOM already recorded for ${commit}; no row appended`);
    process.exit(0);
  }
}

const build = run('pio', ['run', '-e', 'mega'], path.join(root, 'firmware'));
const buildOutput = `${build.stdout ?? ''}\n${build.stderr ?? ''}`;
if (build.error) {
  console.error(`PlatformIO could not run: ${build.error.message}`);
}
if (build.status !== 0) {
  console.error(buildOutput.trim());
}

const row = [
  commit,
  new Date().toISOString().slice(0, 10),
  build.status === 0 ? '1' : '0',
  await readGapPct(),
  '',
  '',
  '',
  parseBuildMemory(buildOutput, 'Flash'),
  parseBuildMemory(buildOutput, 'RAM'),
  '',
];

await mkdir(path.dirname(csvPath), { recursive: true });
if (!existing) {
  await writeFile(csvPath, `${columns.join(',')}\n`, 'utf8');
} else if (!existing.endsWith('\n')) {
  await appendFile(csvPath, '\n', 'utf8');
}
await appendFile(csvPath, serializeCsvRow(row), 'utf8');
console.log(serializeCsvRow(row).trimEnd());

if (build.status === null || build.status !== 0) process.exitCode = 1;
