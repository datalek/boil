import { describe, it, expect } from 'vitest';
import * as E from '../either.js';
import { parseInputArgs, promptForVariables } from '../cli.js';
import { makeTestEnv } from './mocks.js';

describe('cli', () => {
  describe('parseInputArgs', () => {
    it('should parse valid arguments successfully', () => {
      const { env } = makeTestEnv();
      const argv = ['node', 'boil', 'my-project', './template.hsfiles'];

      const actual = parseInputArgs(argv)(env);
      const expected = E.right({
        projectName: 'my-project',
        templatePath: './template.hsfiles',
      });

      expect(actual).toStrictEqual(expected);
    });

    it('should parse arguments with additional flags', () => {
      const { env } = makeTestEnv();
      const argv = [
        'node',
        'boil',
        'my-app',
        'https://example.com/template.hsfiles',
      ];

      const actual = parseInputArgs(argv)(env);
      const expected = E.right({
        projectName: 'my-app',
        templatePath: 'https://example.com/template.hsfiles',
      });

      expect(actual).toStrictEqual(expected);
    });

    it('should return error when insufficient arguments provided', () => {
      const { env, mocks } = makeTestEnv();
      const argv = ['node', 'boil', 'my-project'];

      const actual = parseInputArgs(argv)(env);

      expect(actual.type).toStrictEqual('left');
      expect(mocks.mockCli.write).toBeCalledTimes(1);
    });

    it('should show usage examples in error message', () => {
      const { env, mocks } = makeTestEnv();
      const argv = ['node', 'boil'];

      parseInputArgs(argv)(env);

      expect(mocks.mockCli.write.mock.calls[0][0]).to.toContain('Usage: boil');
    });
  });

  describe('promptForVariables', () => {
    it('should not prompt when variables set is empty and return only project name', async () => {
      const { env, mocks } = makeTestEnv();
      const variables = new Set(['name']);
      const projectName = 'my-project';

      const actual = await promptForVariables(variables, projectName)(env);

      expect(actual).toStrictEqual(E.right({ name: 'my-project' }));
      expect(mocks.mockCli.prompt).toBeCalledTimes(0);
    });

    it('should prompt for variables excluding name', async () => {
      const { env, mocks } = makeTestEnv();
      const variables = new Set(['name', 'author', 'description']);
      const projectName = 'my-project';

      mocks.mockCli.prompt.mockResolvedValueOnce('anAuthor');
      mocks.mockCli.prompt.mockResolvedValueOnce('aDescription');

      const actual = await promptForVariables(variables, projectName)(env);
      const expected = {
        name: 'my-project',
        author: 'anAuthor',
        description: 'aDescription',
      };

      expect(actual).toStrictEqual(E.right(expected));
    });

    it('should trim whitespace from user input', async () => {
      const { env, mocks } = makeTestEnv();
      const variables = new Set(['name', 'email']);
      const projectName = 'my-project';

      mocks.mockCli.prompt.mockResolvedValue('anemail@email.com     ');

      const actual = await promptForVariables(variables, projectName)(env);
      const expected = {
        name: 'my-project',
        email: 'anemail@email.com',
      };

      expect(actual).toStrictEqual(E.right(expected));
    });
  });
});
