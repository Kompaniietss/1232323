import { mkdir, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const tracesDir = path.join(root, 'reports', 'traces');
await mkdir(tracesDir, { recursive: true });

const scenarios = [
  { name: 'successful-rent', status: 'pass', detail: 'Bike reserved and unlocked successfully.' },
  { name: 'successful-return', status: 'pass', detail: 'Bike returned to station and locked.' },
  { name: 'reservation-timeout', status: 'pass', detail: 'Reservation expires after 10 minutes.' },
  { name: 'low-battery-rent-attempt', status: 'blocked', detail: 'Battery guard blocks rent when level <= 20.' },
  { name: 'invalid-trigger', status: 'error', detail: 'Unsupported state-trigger pair remains unchanged.' },
];

for (const scenario of scenarios) {
  const file = path.join(tracesDir, `${scenario.name}.json`);
  await writeFile(file, JSON.stringify({ scenario: scenario.name, status: scenario.status, detail: scenario.detail }, null, 2) + '\n', 'utf8');
}

console.log(`Generated ${scenarios.length} golden traces`);
