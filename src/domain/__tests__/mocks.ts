import { vi } from 'vitest';
import nodeFS from 'node:fs';
import nodePath from 'node:path';
import { LoggerEnv } from '../logger.js';
import { TemplateEnv } from '../template.js';
import { CliEnv } from '../cli.js';

export const makeTestEnv = () => {
  const mocks = {
    mockFetch: vi.fn<TemplateEnv['fetch']>(),
    mockFs: {
      constants: nodeFS.promises.constants,
      readFile: vi.fn<TemplateEnv['fs']['readFile']>(),
      access: vi.fn<TemplateEnv['fs']['access']>(),
      mkdir: vi.fn<TemplateEnv['fs']['mkdir']>(),
      writeFile: vi.fn<TemplateEnv['fs']['writeFile']>(),
    },
    mockLogger: {
      info: vi.fn<LoggerEnv['logger']['info']>(),
      warn: vi.fn<LoggerEnv['logger']['warn']>(),
      error: vi.fn<LoggerEnv['logger']['error']>(),
      debug: vi.fn<LoggerEnv['logger']['debug']>(),
      log: vi.fn<LoggerEnv['logger']['log']>(),
    },
    mockCli: {
      write: vi.fn<CliEnv['cli']['write']>(),
      prompt: vi.fn<CliEnv['cli']['prompt']>(),
    },
  };

  const env: LoggerEnv & TemplateEnv & CliEnv = {
    fetch: mocks.mockFetch,
    path: nodePath,
    fs: mocks.mockFs,
    logger: mocks.mockLogger,
    cli: mocks.mockCli,
  };

  return { env, mocks };
};
