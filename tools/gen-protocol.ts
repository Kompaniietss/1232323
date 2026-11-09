import { readFile, writeFile, mkdir } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const protocol = JSON.parse(await readFile(path.join(root, 'model', 'protocol.json'), 'utf8'));
const outputDir = path.join(root, 'firmware', 'lib', 'protocol', 'generated');
await mkdir(outputDir, { recursive: true });

const entries = protocol.messages
  .map((msg) => `  ${msg.name.replace(/\./g, '_').toUpperCase()} = ${msg.frameId},`)
  .join('\n');

const header = `#pragma once
#include <stdint.h>

namespace firmware {

enum class FrameId : uint8_t {
${entries}
};

}  // namespace firmware
`;

await writeFile(path.join(outputDir, 'frame_ids.h'), header, 'utf8');
console.log('Generated frame IDs');
