import { readFile, writeFile } from 'node:fs/promises';
import { log } from 'node:console';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const hardware = JSON.parse(await readFile(path.join(root, 'model', 'hardware.json'), 'utf8'));
const outputPath = path.join(root, 'firmware', 'diagram.json');
const checkOnly = process.argv.includes('--check');

function stringifyCompact(value) {
  let inString = false;
  let escaped = false;
  let result = '';
  const json = JSON.stringify(value);

  for (let index = 0; index < json.length; index += 1) {
    const character = json[index];
    if (!inString && character === '{' && json[index + 1] !== '}') {
      result += '{ ';
      continue;
    }
    if (!inString && character === '}' && json[index - 1] !== '{') {
      result += ' }';
      continue;
    }
    result += character;
    if (escaped) {
      escaped = false;
    } else if (character === '\\' && inString) {
      escaped = true;
    } else if (character === '"') {
      inString = !inString;
    } else if (!inString && (character === ',' || character === ':')) {
      result += ' ';
    }
  }

  return result;
}

const diagram = Object.entries(hardware.wokwi)
  .map(([key, value]) => {
    const serializedValue =
      key === 'parts' || key === 'connections'
        ? `[\n${value.map((entry) => `    ${stringifyCompact(entry)}`).join(',\n')}\n  ]`
        : stringifyCompact(value);
    return `  ${JSON.stringify(key)}: ${serializedValue}`;
  })
  .join(',\n');
const contents = `{\n${diagram}\n}`;

if (checkOnly) {
  const existing = await readFile(outputPath, 'utf8');
  if (existing !== contents) {
    throw new Error('firmware/diagram.json is out of date; run npm run wokwi:generate.');
  }
  log('Generated Wokwi diagram matches model/hardware.json.');
} else {
  await writeFile(outputPath, contents, 'utf8');
  log('Generated firmware/diagram.json from model/hardware.json.');
}
