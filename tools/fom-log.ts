import { mkdir, readFile, rename, writeFile } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const reportsDir = path.join(root, 'reports');
const csvPath = path.join(reportsDir, 'fom.csv');
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
const legacyColumns = columns.slice(0, -1);
type CsvRow = string[];

function run(command: string, args: string[], cwd = root) {
  return spawnSync(command, args, {
    cwd,
    encoding: 'utf8',
    windowsHide: true,
    maxBuffer: 16 * 1024 * 1024,
  });
}

function runChecked(command: string, args: string[], label: string): void {
  const result = run(command, args);
  if (result.status !== 0) {
    throw new Error(
      `${label} failed: ${result.stderr.trim() || result.error?.message || result.stdout.trim() || 'command failed'}`,
    );
  }
}

function parseCsv(text: string): CsvRow[] {
  const rows: CsvRow[] = [];
  let row: CsvRow = [];
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

function serializeCsv(rows: CsvRow[]): string {
  return rows.map((row) => `${row.map((cell) => /[",\r\n]/.test(cell)
    ? `"${cell.replaceAll('"', '""')}"`
    : cell).join(',')}\n`).join('');
}

function requiredFlag(flags: Map<string, string>, name: string): string {
  const value = flags.get(name);
  if (value === undefined || value.length === 0) throw new Error(`Missing required --${name} value`);
  return value;
}

function parseFlags(args: string[]): Map<string, string> {
  const flags = new Map<string, string>();
  for (let index = 0; index < args.length; index += 1) {
    const name = args[index];
    if (!name?.startsWith('--') || index + 1 >= args.length || args[index + 1]?.startsWith('--')) {
      throw new Error(`Expected a flag and value, got ${name ?? ''}`);
    }
    if (flags.has(name.slice(2))) throw new Error(`Duplicate --${name.slice(2)} flag`);
    flags.set(name.slice(2), args[index + 1]!);
    index += 1;
  }
  for (const name of flags.keys()) {
    if (!['commit', 'build-ok', 'gap', 'notes'].includes(name)) {
      throw new Error(`Unknown baseline flag --${name}`);
    }
  }
  return flags;
}

function gitOutput(args: string[], label: string): string {
  const result = run('git', args);
  if (result.status !== 0 || !result.stdout.trim()) {
    throw new Error(`${label}: ${result.stderr.trim() || result.error?.message || 'git failed'}`);
  }
  return result.stdout.trim();
}

function checkCleanWorkingTree(): void {
  const result = run('git', ['status', '--porcelain=v1', '--untracked-files=all', '-z']);
  if (result.status !== 0) {
    throw new Error(`Could not check the working tree: ${result.stderr.trim() || 'git status failed'}`);
  }

  const entries = result.stdout.split('\0');
  const changedPaths: string[] = [];
  for (let index = 0; index < entries.length; index += 1) {
    const entry = entries[index];
    if (!entry) continue;
    const filePath = entry.slice(3);
    changedPaths.push(filePath);
    if (/^[RC]/.test(entry.slice(0, 2)) && entries[index + 1]) {
      changedPaths.push(entries[index + 1]!);
      index += 1;
    }
  }

  const outsideReports = changedPaths.filter((filePath) => !/^reports[\\/]+fom\.[^\\/]+$/.test(filePath));
  if (outsideReports.length > 0) {
    throw new Error(
      `Refusing to record FOM for an uncommitted checkout. Commit or stash changes outside reports/fom.* first:\n${outsideReports.map((filePath) => `  ${filePath}`).join('\n')}`,
    );
  }
}

function parseBuildMemory(output: string, label: 'RAM' | 'Flash'): string {
  const ansiEscape = String.fromCharCode(27);
  const cleanOutput = output.replace(new RegExp(`${ansiEscape}\\[[0-9;]*m`, 'g'), '');
  const match = new RegExp(`${label}:.*?\\(used\\s+(\\d+)\\s+bytes`, 'is').exec(cleanOutput);
  return match?.[1] ?? '';
}

function runBuild(): { status: number | null; output: string; error?: Error } {
  const firmwareDir = path.join(root, 'firmware');
  const pio = run('pio', ['run', '-e', 'mega'], firmwareDir);
  if (!isMissingCommandError(pio.error)) {
    return {
      status: pio.status,
      output: `${pio.stdout ?? ''}\n${pio.stderr ?? ''}`,
      error: pio.error,
    };
  }

  const py = run('py', ['-m', 'platformio', 'run', '-e', 'mega'], firmwareDir);
  return {
    status: py.status,
    output: `${py.stdout ?? ''}\n${py.stderr ?? ''}`,
    error: py.error,
  };
}

async function loadExistingRows(): Promise<CsvRow[]> {
  let text: string;
  try {
    text = await readFile(csvPath, 'utf8');
  } catch (error) {
    if (isMissingFile(error)) return [];
    throw error;
  }

  const rows = parseCsv(text.replace(/^\uFEFF/, ''));
  if (rows.length === 0) return [];
  const header = rows.shift()!;
  const isLegacyHeader = header.join(',') === legacyColumns.join(',');
  if (!isLegacyHeader && header.join(',') !== columns.join(',')) {
    throw new Error('reports/fom.csv has an unexpected header');
  }

  return rows.map((row) => {
    if (row.length !== header.length) {
      throw new Error('reports/fom.csv contains a row with an unexpected number of columns');
    }
    return isLegacyHeader ? [...row, ''] : row;
  });
}

function isMissingFile(error: unknown): boolean {
  return typeof error === 'object' && error !== null && 'code' in error && error.code === 'ENOENT';
}

function isMissingCommandError(error: Error | undefined): boolean {
  return error !== undefined && 'code' in error && error.code === 'ENOENT';
}

function sortRowsByDate(rows: CsvRow[]): CsvRow[] {
  return rows.sort((left, right) => (left[1] ?? '').localeCompare(right[1] ?? ''));
}

async function writeRows(rows: CsvRow[]): Promise<void> {
  await mkdir(reportsDir, { recursive: true });
  const temporaryPath = `${csvPath}.tmp`;
  await writeFile(temporaryPath, serializeCsv([columns, ...sortRowsByDate(rows)]), 'utf8');
  await rename(temporaryPath, csvPath);
}

async function baseline(flags: Map<string, string>): Promise<void> {
  const commit = requiredFlag(flags, 'commit');
  const buildOk = requiredFlag(flags, 'build-ok');
  const gap = requiredFlag(flags, 'gap');
  const notes = flags.get('notes') ?? '';
  if (buildOk !== '0' && buildOk !== '1') throw new Error('--build-ok must be 0 or 1');
  if (!/^(?:\d+(?:\.\d*)?|\.\d+)$/.test(gap) || Number(gap) > 100) {
    throw new Error('--gap must be a percentage from 0 to 100');
  }

  const date = gitOutput(['show', '-s', '--format=%cI', commit], `Could not read commit date for ${commit}`);
  const dateMatch = /^(\d{4}-\d{2}-\d{2})T/.exec(date);
  if (!dateMatch) throw new Error(`Git returned an invalid commit date for ${commit}: ${date}`);

  const rows = await loadExistingRows();
  const matchingRows = rows.filter((row) => row[0] === commit);
  if (matchingRows.length > 1) throw new Error(`reports/fom.csv contains duplicate rows for ${commit}`);
  const remainingRows = rows.filter((row) => row[0] !== commit);
  const row = [commit, dateMatch[1]!, buildOk, gap, '', '', '', '', '', '', notes];
  await writeRows([...remainingRows, row]);
  console.log(serializeCsv([row]).trimEnd());
}

async function recordCurrentCheckout(): Promise<void> {
  checkCleanWorkingTree();

  const commit = gitOutput(['rev-parse', '--short', 'HEAD'], 'Could not read HEAD commit');
  const commitDate = gitOutput(['show', '-s', '--format=%cI', 'HEAD'], 'Could not read HEAD commit date');
  const dateMatch = /^(\d{4}-\d{2}-\d{2})T/.exec(commitDate);
  if (!dateMatch) throw new Error(`Git returned an invalid HEAD commit date: ${commitDate}`);

  runChecked(process.execPath, [path.join(root, 'tools', 'extract-cpp-model.mjs')], 'Model extraction');
  runChecked(process.execPath, [path.join(root, 'tools', 'verify-model.mjs')], 'Model verification');
  const verification = JSON.parse(
    await readFile(path.join(reportsDir, 'model-verification.json'), 'utf8'),
  ) as { gap_pct?: unknown };
  if (typeof verification.gap_pct !== 'number' || !Number.isFinite(verification.gap_pct)) {
    throw new Error('reports/model-verification.json does not contain a numeric gap_pct');
  }

  const build = runBuild();
  if (build.error) console.error(`PlatformIO could not run: ${build.error.message}`);
  if (build.status !== 0) console.error(build.output.trim() || 'PlatformIO build failed without output');

  const buildOk = build.status === 0;
  const row = [
    commit,
    dateMatch[1]!,
    buildOk ? '1' : '0',
    String(verification.gap_pct),
    '',
    '',
    '',
    buildOk ? parseBuildMemory(build.output, 'Flash') : '',
    buildOk ? parseBuildMemory(build.output, 'RAM') : '',
    '',
    '',
  ];

  const rows = await loadExistingRows();
  const matchingRows = rows.filter((existingRow) => existingRow[0] === commit);
  if (matchingRows.length > 1) throw new Error(`reports/fom.csv contains duplicate rows for ${commit}`);
  await writeRows([...rows.filter((existingRow) => existingRow[0] !== commit), row]);
  console.log(serializeCsv([row]).trimEnd());
  if (!buildOk) process.exitCode = 1;
}

try {
  const args = process.argv.slice(2);
  if (args[0] === '--baseline') {
    await baseline(parseFlags(args.slice(1)));
  } else {
    if (args.length > 0) throw new Error('Usage: node tools/fom-log.ts [--baseline --commit HASH --build-ok 0|1 --gap PERCENT [--notes TEXT]]');
    await recordCurrentCheckout();
  }
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
}
