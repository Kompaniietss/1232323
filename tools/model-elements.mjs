import { readFile, readdir } from 'node:fs/promises';
import path from 'node:path';

const transitionTriggerNames = new Map([
  ['reserve()', 'Reserve'],
  ['timeout', 'Timeout'],
  ['cancel()', 'Cancel'],
  ['unlock()', 'Unlock'],
  ['returnToStation()', 'ReturnToStation'],
  ['reportFault()', 'ReportFault'],
  ['batteryLow()', 'BatteryLow'],
  ['repaired()', 'Repaired'],
]);

const attributeAliases = new Map([
  ['Rental.cost', 'costCents'],
]);

const compareText = (left, right) => left < right ? -1 : left > right ? 1 : 0;

export async function loadModel(root) {
  const modelDir = path.join(root, 'model');
  const [classes, stateMachine, sequences, protocol] = await Promise.all([
    readFile(path.join(modelDir, 'classes.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'bike.statemachine.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'sequences.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'protocol.json'), 'utf8').then(JSON.parse),
  ]);
  const elements = [];
  const add = (category, id) => elements.push({ category, id });

  for (const modelClass of classes.classes) {
    for (const attribute of modelClass.attributes) {
      add('classMembers', `class:${modelClass.name}.attribute:${attribute.name}`);
    }
    for (const operation of modelClass.operations) {
      add('classMembers', `class:${modelClass.name}.operation:${operation.name}`);
    }
  }
  for (const relation of classes.relationships) {
    add('relationships', `relationship:${relation.id}`);
  }
  for (const state of stateMachine.states) add('states', `state:${state}`);
  add('transitions', `transition:${stateMachine.initial.id}:initial`);
  for (const transition of stateMachine.transitions) {
    add('transitions', `transition:${transition.id}:${transition.trigger}`);
  }
  for (const [flow, messages] of Object.entries(sequences)) {
    for (const message of messages) add('sequenceMessages', `sequence:${flow}:${message.id}`);
  }
  for (const message of protocol.messages) add('protocolFrames', `frame:${message.name}`);

  const ordered = elements.sort((left, right) => compareText(left.id, right.id));
  const byId = new Map(ordered.map((element) => [element.id, element]));
  return { elements: ordered, byId, classes, stateMachine, sequences, protocol, relationships: classes.relationships };
}

export function sortObjectKeys(value) {
  if (Array.isArray(value)) return value.map(sortObjectKeys);
  if (value && typeof value === 'object') {
    return Object.fromEntries(Object.entries(value)
      .sort(([left], [right]) => compareText(left, right))
      .map(([key, nested]) => [key, sortObjectKeys(nested)]));
  }
  return value;
}

export async function findSourceFiles(dir) {
  const entries = await readdir(dir, { withFileTypes: true });
  const files = [];
  for (const entry of entries) {
    const current = path.join(dir, entry.name);
    if (entry.isDirectory()) files.push(...await findSourceFiles(current));
    else if (/\.(?:h|hpp|cpp)$/.test(entry.name)) files.push(current);
  }
  return files.sort();
}

function isImplementedOperation(text, offset, name) {
  const afterTag = text.slice(offset, offset + 500);
  const signature = new RegExp(
    `\\b${name}\\s*\\([^;{}]*\\)\\s*(?:(?:const|override|noexcept)\\s*)*\\{`,
  ).exec(afterTag);
  if (!signature) return false;
  const openBrace = offset + signature.index + signature[0].lastIndexOf('{');
  let depth = 0;
  let closeBrace = -1;
  for (let i = openBrace; i < text.length; i += 1) {
    if (text[i] === '{') depth += 1;
    if (text[i] === '}' && --depth === 0) {
      closeBrace = i;
      break;
    }
  }
  if (closeBrace < 0) return false;
  const body = text.slice(openBrace + 1, closeBrace)
    .replace(/\/\/[^\n]*/g, '')
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .replace(/\(void\)\s*\w+\s*;/g, '')
    .replace(/return\s+nullptr\s*;/g, '')
    .trim();
  return body.length > 0;
}

function tagElement(tag, source, offset, catalog) {
  const afterTag = source.slice(offset, offset + 500);
  let elementId;
  let valid = false;

  if (tag.startsWith('state:')) {
    elementId = tag;
    const state = tag.slice('state:'.length);
    valid = catalog.elements.some((item) => item.id === elementId)
      && new RegExp(`^\\s*${state}\\s*(?:,|\\r?\\n\\s*})`).test(afterTag);
  } else if (tag.startsWith('transition:')) {
    elementId = tag;
    const [, id, trigger] = /^transition:([^:]+):(.+)$/.exec(tag) || [];
    if (id && trigger === 'initial') {
      valid = /\bstate\s*=\s*BikeState::Available\s*;/.test(afterTag);
    } else if (id && trigger) {
      const triggerName = transitionTriggerNames.get(trigger);
      valid = Boolean(triggerName)
        && new RegExp(`\\{\\s*"${id}"\\s*,[^}]*BikeTrigger::${triggerName}\\s*\\}`).test(afterTag);
    }
  } else if (tag.startsWith('frame:')) {
    elementId = tag;
    const frame = catalog.protocol.messages.find((item) => `frame:${item.name}` === elementId);
    if (frame) {
      const enumName = frame.name.replace(/\./g, '_').toUpperCase();
      valid = new RegExp(`\\b${enumName}\\s*=\\s*${frame.frameId}\\b`).test(afterTag);
    }
  } else if (tag.startsWith('sequence:')) {
    elementId = tag;
    const [, flow, messageId] = /^sequence:([^:]+):([^:]+)$/.exec(tag) || [];
    const message = catalog.sequences[flow]?.find((item) => item.id === messageId);
    const frame = message && catalog.protocol.messages.find(
      (item) => item.sequenceId === message.id && item.name.startsWith(`${flow}.`),
    );
    if (frame) {
      const enumName = frame.name.replace(/\./g, '_').toUpperCase();
      valid = new RegExp(`link_->send\\s*\\(\\s*static_cast\\s*<\\s*uint8_t\\s*>\\s*\\(\\s*FrameId::${enumName}`).test(afterTag);
    }
  } else if (tag.startsWith('relationship:')) {
    elementId = tag;
    const relation = catalog.relationships.find((item) => `relationship:${item.id}` === elementId);
    if (relation) {
      valid = new RegExp(`\\b${relation.target}\\s*\\*?\\s+[A-Za-z_]\\w*\\s*(?:\\[[^\\]]*\\])?(?:\\s*=\\s*\\{\\s*\\})?\\s*;`).test(afterTag);
    }
  } else {
    const operation = /^([A-Za-z_]\w*)\.([A-Za-z_]\w*)\(\)$/.exec(tag);
    const attribute = /^([A-Za-z_]\w*)\.([A-Za-z_]\w*)$/.exec(tag);
    if (operation) {
      elementId = `class:${operation[1]}.operation:${operation[2]}`;
      valid = isImplementedOperation(source, offset, operation[2]);
    } else if (attribute) {
      elementId = `class:${attribute[1]}.attribute:${attribute[2]}`;
      const memberName = attributeAliases.get(`${attribute[1]}.${attribute[2]}`) ?? attribute[2];
      valid = new RegExp(`\\b${memberName}\\s*(?:=[^;]*)?;`).test(afterTag);
    } else {
      return null;
    }
  }

  if (!valid) return null;
  const category = catalog.byId.get(elementId)?.category ?? (
    elementId.startsWith('class:') ? 'classMembers' :
      elementId.startsWith('state:') ? 'states' :
        elementId.startsWith('transition:') ? 'transitions' :
          elementId.startsWith('sequence:') ? 'sequenceMessages' :
            elementId.startsWith('frame:') ? 'protocolFrames' : 'relationships'
  );
  return { category, id: elementId };
}

export function extractTaggedElements(files, catalog) {
  const elements = new Map();
  const tagsByFile = {};
  for (const { relativePath, source } of files) {
    const tags = [];
    for (const match of source.matchAll(/\/\/\s*@model\s+([^\n]+)/g)) {
      const tag = match[1].trim();
      const element = tagElement(tag, source, match.index + match[0].length, catalog);
      if (!element) continue;
      elements.set(element.id, element);
      tags.push({ tag, id: element.id });
    }
    if (tags.length > 0) tagsByFile[relativePath] = tags;
  }
  return {
    elements: [...elements.values()].sort((left, right) => compareText(left.id, right.id)),
    tagsByFile: Object.fromEntries(Object.entries(tagsByFile).sort(([a], [b]) => compareText(a, b))),
  };
}

export function summarizeModel(catalog, extracted) {
  const matchedIds = new Set(extracted.elements.map((element) => element.id));
  const modelIds = new Set(catalog.elements.map((element) => element.id));
  const categories = [...new Set(catalog.elements.map((element) => element.category))].sort(compareText);
  const breakdown = {};
  for (const category of categories) {
    const required = catalog.elements.filter((element) => element.category === category);
    const matched = required.filter((element) => matchedIds.has(element.id));
    breakdown[category] = {
      total: required.length,
      matched: matched.length,
      gap_pct: Number((100 * (1 - matched.length / required.length)).toFixed(2)),
    };
  }
  const missing = catalog.elements
    .filter((element) => !matchedIds.has(element.id))
    .map(({ category, id }) => ({ category, id }))
    .sort((left, right) => compareText(left.id, right.id));
  const extra = extracted.elements
    .filter((element) => !modelIds.has(element.id))
    .map(({ category, id }) => ({ category, id }))
    .sort((left, right) => compareText(left.id, right.id));
  const total = catalog.elements.length;
  const matched = catalog.elements.length - missing.length;
  return {
    total,
    matched,
    gap_pct: Number((100 * (1 - matched / total)).toFixed(2)),
    breakdown,
    missing,
    extra,
  };
}
