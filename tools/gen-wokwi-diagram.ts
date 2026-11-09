import { readFile, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const hardware = JSON.parse(await readFile(path.join(root, 'model', 'hardware.json'), 'utf8'));
const outputPath = path.join(root, 'firmware', 'diagram.json');
await mkdir(path.dirname(outputPath), { recursive: true });

const diagram = {
  version: 1,
  editor: 'wokwi',
  layout: {
    width: 1200,
    height: 700,
  },
  parts: [
    { type: 'board-mega', id: 'mega', x: 400, y: 220 },
    { type: 'pushbutton', id: 'button', x: 180, y: 420, label: 'Button', value: '2' },
    { type: 'led', id: 'greenLed', x: 630, y: 180, color: 'green', label: 'Available' },
    { type: 'led', id: 'yellowLed', x: 690, y: 180, color: 'yellow', label: 'Reserved' },
    { type: 'led', id: 'redLed', x: 750, y: 180, color: 'red', label: 'Rented' },
  ],
  wires: [
    { from: 'button:1', to: 'mega:2' },
    { from: 'button:2', to: 'mega:GND' },
    { from: 'greenLed:anode', to: 'mega:5' },
    { from: 'greenLed:cathode', to: 'mega:GND' },
    { from: 'yellowLed:anode', to: 'mega:6' },
    { from: 'yellowLed:cathode', to: 'mega:GND' },
    { from: 'redLed:anode', to: 'mega:7' },
    { from: 'redLed:cathode', to: 'mega:GND' },
    { from: 'mega:18', to: 'mega:17' },
    { from: 'mega:16', to: 'mega:19' },
  ],
  metadata: hardware,
};

await writeFile(outputPath, JSON.stringify(diagram, null, 2) + '\n', 'utf8');
console.log('Generated Wokwi diagram');
