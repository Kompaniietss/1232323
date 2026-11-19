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
  const [classes, stateMachine, sequences, protocol, umlSequences, umlMapping] = await Promise.all([
    readFile(path.join(modelDir, 'classes.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'bike.statemachine.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'sequences.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'protocol.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'uml-sequences.json'), 'utf8').then(JSON.parse),
    readFile(path.join(modelDir, 'uml-mapping.json'), 'utf8').then(JSON.parse),
  ]);
  validateUmlMapping(umlSequences, umlMapping);
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
  return {
    elements: ordered,
    byId,
    classes,
    stateMachine,
    sequences,
    protocol,
    umlSequences,
    umlMapping,
    relationships: classes.relationships,
  };
}

function validateUmlMapping(umlSequences, umlMapping) {
  const expectedFlows = ['rent', 'return'];
  const sequenceFlows = Object.keys(umlSequences).sort(compareText);
  const mappingFlows = Object.keys(umlMapping).sort(compareText);
  if (sequenceFlows.join(',') !== expectedFlows.join(',') ||
      mappingFlows.join(',') !== expectedFlows.join(',')) {
    throw new Error('UML sequence and mapping must contain exactly the rent and return flows');
  }

  for (const flow of sequenceFlows) {
    const messages = umlSequences[flow];
    const mappings = umlMapping[flow];
    const expectedLength = flow === 'rent' ? 8 : 9;
    const participants = new Set(['User', 'App', 'Station', 'Bike', 'Rental', 'PaymentService']);
    if (!Array.isArray(messages) || messages.length !== expectedLength ||
        !Array.isArray(mappings) || messages.length !== mappings.length) {
      throw new Error(`UML mapping for ${flow} must contain one entry per sequence message`);
    }

    const messageIds = new Set();
    messages.forEach((message, index) => {
      const mapping = mappings[index];
      if (!message || message.id !== `m${index}` ||
          !participants.has(message.from) || !participants.has(message.to) ||
          typeof message.message !== 'string' || !message.message ||
          messageIds.has(message.id)) {
        throw new Error(`UML sequence ${flow} contains an invalid or duplicate message`);
      }
      messageIds.add(message.id);
      if (!mapping || mapping.id !== message.id) {
        throw new Error(`UML mapping for ${flow} is missing, duplicating, or reordering ${message.id}`);
      }

      if (mapping.status === 'aligned') {
        if (typeof mapping.firmware !== 'string' || !mapping.firmware.trim() ||
            Object.hasOwn(mapping, 'reason') ||
            Object.keys(mapping).some((key) => !['id', 'status', 'firmware'].includes(key))) {
          throw new Error(`Aligned UML mapping ${flow}:${message.id} requires only a named firmware reference`);
        }
      } else if (mapping.status === 'deviated' || mapping.status === 'unmapped') {
        if (typeof mapping.reason !== 'string' || !mapping.reason.trim() ||
            /[\r\n]/.test(mapping.reason)) {
          throw new Error(`Non-aligned UML mapping ${flow}:${message.id} requires a one-line reason`);
        }
        if (mapping.status === 'deviated' &&
            (typeof mapping.firmware !== 'string' || !mapping.firmware.trim() ||
             Object.keys(mapping).some((key) => !['id', 'status', 'firmware', 'reason'].includes(key)))) {
          throw new Error(`Deviated UML mapping ${flow}:${message.id} requires a named firmware counterpart`);
        }
        if (mapping.status === 'unmapped' && Object.hasOwn(mapping, 'firmware')) {
          throw new Error(`Unmapped UML mapping ${flow}:${message.id} must not claim a firmware counterpart`);
        }
        if (mapping.status === 'unmapped' &&
            Object.keys(mapping).some((key) => !['id', 'status', 'reason'].includes(key))) {
          throw new Error(`Unmapped UML mapping ${flow}:${message.id} contains unsupported fields`);
        }
      } else {
        throw new Error(`UML mapping ${flow}:${message.id} has an invalid status`);
      }
    });
  }
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
      const sendsFrame = new RegExp(
        `(?:link_->send|sendCost)\\s*\\(\\s*static_cast\\s*<\\s*uint8_t\\s*>\\s*\\(\\s*FrameId::${enumName}`,
      ).test(afterTag);
      const internalAction =
        (flow === 'rent' && messageId === 'm1' &&
          /StateMachine::apply\s*\(\s*bike_\s*,\s*BikeTrigger::Reserve\s*\)/.test(afterTag)) ||
        (flow === 'return' && messageId === 'm4' &&
          /StateMachine::apply\s*\(\s*bike_\s*,\s*BikeTrigger::ReturnToStation\s*\)/.test(afterTag));
      valid = sendsFrame || internalAction;
    }
  } else if (tag.startsWith('relationship:')) {
    elementId = tag;
    const relation = catalog.relationships.find((item) => `relationship:${item.id}` === elementId);
    if (relation) {
      valid = [relation.source, relation.target].some((endpoint) =>
        new RegExp(`\\b${endpoint}\\s*\\*?\\s+[A-Za-z_]\\w*\\s*(?:\\[[^\\]]*\\])?(?:\\s*=\\s*\\{\\s*\\})?\\s*;`).test(afterTag),
      );
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
  const umlCounts = { aligned: 0, deviated: 0, unmapped: 0 };
  const nonAligned = [];
  for (const [flow, messages] of Object.entries(catalog.umlSequences)) {
    const mappings = catalog.umlMapping[flow];
    messages.forEach((message, index) => {
      const mapping = mappings[index];
      umlCounts[mapping.status] += 1;
      if (mapping.status !== 'aligned') {
        nonAligned.push({
          flow,
          ...message,
          status: mapping.status,
          ...(mapping.firmware ? { firmware: mapping.firmware } : {}),
          reason: mapping.reason,
        });
      }
    });
  }
  const umlTotal = Object.values(umlCounts).reduce((sum, count) => sum + count, 0);
  const umlConformance = {
    total: umlTotal,
    aligned: {
      count: umlCounts.aligned,
      pct: Number((100 * umlCounts.aligned / umlTotal).toFixed(2)),
    },
    deviated: {
      count: umlCounts.deviated,
      pct: Number((100 * umlCounts.deviated / umlTotal).toFixed(2)),
    },
    unmapped: {
      count: umlCounts.unmapped,
      pct: Number((100 * umlCounts.unmapped / umlTotal).toFixed(2)),
    },
    nonAligned,
    extras: [
      {
        id: 'rent-receipt',
        behavior: 'Rent receipt displayed over the user terminal',
        firmware: 'ActuatorNode::sendRentReceipt() -> RENT_RECEIPT; SensorNode::handleFrame() -> showReceipt()',
        reason: 'The UML rent flow has no receipt message.',
      },
      {
        id: 'rent-show-receipt-gesture',
        behavior: 'Double-press gesture requests the rent receipt again',
        firmware: 'SensorNode::handlePress() -> RENT_SHOWRECEIPT; ActuatorNode::sendRentReceipt()',
        reason: 'The UML rent flow has no double-press gesture or receipt-display request.',
      },
      {
        id: 'unconsumed-frame:RENT_STARTED',
        type: 'unconsumedFrame',
        behavior: 'RENT_STARTED frame is sent but not consumed',
        firmware: 'ActuatorNode::sendRentStarted() -> SensorNode::handleFrame()',
        reason: 'SensorNode has no RENT_STARTED handler; the received frame is ignored.',
      },
      {
        id: 'unconsumed-frame:RENT_ACTIVE',
        type: 'unconsumedFrame',
        behavior: 'RENT_ACTIVE frame is sent but not consumed',
        firmware: 'ActuatorNode::sendRentActive() -> SensorNode::handleFrame()',
        reason: 'SensorNode has no RENT_ACTIVE handler; the received frame is ignored.',
      },
      {
        id: 'unconsumed-frame:RETURN_AVAILABLE',
        type: 'unconsumedFrame',
        behavior: 'RETURN_AVAILABLE frame is sent but not consumed',
        firmware: 'ActuatorNode::sendReturnAvailable() -> SensorNode::handleFrame()',
        reason: 'SensorNode has no RETURN_AVAILABLE handler; the received frame is ignored.',
      },
      {
        id: 'unconsumed-frame:RETURN_DOCKED',
        type: 'unconsumedFrame',
        behavior: 'RETURN_DOCKED frame is sent but not consumed',
        firmware: 'ActuatorNode::sendReturnDocked() -> SensorNode::handleFrame()',
        reason: 'SensorNode has no RETURN_DOCKED handler; the received frame is ignored.',
      },
    ],
  };
  return {
    total,
    matched,
    gap_pct: Number((100 * (1 - matched / total)).toFixed(2)),
    gapVsIotModel: Number((100 * (1 - matched / total)).toFixed(2)),
    breakdown,
    missing,
    extra,
    umlConformance,
  };
}
