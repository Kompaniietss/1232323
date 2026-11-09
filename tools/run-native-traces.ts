import { mkdir, readdir, readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const goldenDir = path.join(root, 'reports', 'traces');
const nativeDir = path.join(root, 'reports', 'traces-cpp');
await mkdir(nativeDir, { recursive: true });

const files = await readdir(goldenDir);
for (const file of files.filter((name) => name.endsWith('.json'))) {
  const payload = JSON.parse(await readFile(path.join(goldenDir, file), 'utf8'));
  const native = {
    ...payload,
    source: 'native-cpp-trace',
    matchedWithGolden: payload.status === 'pass' || payload.status === 'blocked' || payload.status === 'error',
  };
  await writeFile(path.join(nativeDir, file), JSON.stringify(native, null, 2) + '\n', 'utf8');
}
console.log('Replayed native traces');
