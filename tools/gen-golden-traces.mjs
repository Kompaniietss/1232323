import console from 'node:console';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createServer } from 'vite';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const tracesDir = path.join(root, 'reports', 'traces');
const scenariosPath = path.join(root, 'tools', 'trace-scenarios.json');
const scenarios = JSON.parse(await readFile(scenariosPath, 'utf8')).scenarios;
const server = await createServer({
  configFile: false,
  root,
  logLevel: 'silent',
  server: { middlewareMode: true },
  appType: 'custom',
});

try {
  const [
    { createSeededMockState },
    { rentBike },
    { returnBike },
    { applyBikeTrigger },
    { calculateRentalCost },
  ] = await Promise.all([
    server.ssrLoadModule('/src/infrastructure/mock/seedData.ts'),
    server.ssrLoadModule('/src/application/rentBike.ts'),
    server.ssrLoadModule('/src/application/returnBike.ts'),
    server.ssrLoadModule('/src/domain/stateMachine.ts'),
    server.ssrLoadModule('/src/domain/pricing.ts'),
  ]);
  const protocol = JSON.parse(await readFile(path.join(root, 'model', 'protocol.json'), 'utf8'));
  const frameIds = new Map(protocol.messages.map(({ name, frameId }) => [name, frameId]));
  const protocolFrameId = (name) => {
    const value = frameIds.get(name);
    if (!Number.isInteger(value)) throw new Error(`Missing protocol frame ${name}`);
    return value;
  };

  await mkdir(tracesDir, { recursive: true });
  for (const scenario of scenarios) {
    const RealDate = globalThis.Date;
    let clock = RealDate.parse('2025-01-01T00:00:00.000Z');
    class TraceDate extends RealDate {
      constructor(...args) {
        if (args.length === 0) super(clock);
        else super(...args);
      }

      static now() {
        return clock;
      }
    }
    globalThis.Date = TraceDate;

    try {
      const state = createSeededMockState();
      const bike = state.bikes[0];
      if (!bike) throw new Error(`Scenario ${scenario.name} has no seeded bike`);
      bike.batteryLevel = scenario.batteryLevel;
      bike.state = scenario.initialState;
      const steps = [];

      for (const input of scenario.inputs) {
        const stateBefore = bike.state;
        let costCents = null;
        let frameId = null;
        if (input === 'rentBike') {
          if (bike.state === 'Available') {
            rentBike(state, bike.bikeId);
            frameId = protocolFrameId('rent.receipt');
            costCents = Math.round(calculateRentalCost(0) * 100);
          } else {
            let rejected = false;
            try {
              rentBike(state, bike.bikeId);
            } catch (error) {
              if (!(error instanceof Error) || !error.message.includes('is not available')) {
                throw error;
              }
              rejected = true;
            }
            if (!rejected) throw new Error(`Scenario ${scenario.name} unexpectedly accepted rentBike`);
            frameId = protocolFrameId('rent.request');
          }
        } else if (input === 'returnBike') {
          clock += scenario.durationMs;
          const result = returnBike(state);
          costCents = Math.round(result.receipt.cost * 100);
          frameId = protocolFrameId('return.ended');
        } else if (input === 'reserve' || input === 'timeout' || input === 'batteryLow') {
          const triggers = {
            reserve: 'reserve()',
            timeout: 'timeout',
            batteryLow: 'batteryLow()',
          };
          applyBikeTrigger(bike, triggers[input]);
        } else if (input === 'invalidReturn') {
          let rejected = false;
          try {
            applyBikeTrigger(bike, 'returnToStation()');
          } catch (error) {
            if (!(error instanceof Error) ||
                !error.message.includes('not supported from Bike state')) {
              throw error;
            }
            rejected = true;
          }
          if (!rejected) throw new Error(`Scenario ${scenario.name} unexpectedly accepted invalidReturn`);
        } else {
          throw new Error(`Unknown trace input: ${input}`);
        }
        steps.push({
          input,
          stateBefore,
          stateAfter: bike.state,
          frameId,
          ledColor: bike.state === 'Available' ? 'Green'
            : bike.state === 'Reserved' ? 'Yellow'
              : 'Red',
          costCents,
        });
      }

      const trace = { scenario: scenario.name, steps };
      await writeFile(
        path.join(tracesDir, `${scenario.name}.json`),
        `${JSON.stringify(trace, null, 2)}\n`,
        'utf8',
      );
    } finally {
      globalThis.Date = RealDate;
    }
  }

  console.log(`Generated ${scenarios.length} TypeScript execution traces`);
} finally {
  await server.close();
}
