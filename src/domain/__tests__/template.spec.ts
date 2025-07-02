import { describe, it, expect } from 'vitest';
import * as E from '../either.js';
import {
  fetchTemplate,
  parseTemplate,
  createFiles,
  FetchedTemplate,
} from '../template.js';
import { makeTestEnv } from './mocks.js';
import * as data from './data.js';

describe('template', () => {
  describe('fetchTemplate', () => {
    it('should fetch template from HTTP URL successfully', async () => {
      const { env, mocks } = makeTestEnv();

      mocks.mockFetch.mockResolvedValue(data.responseOK);

      const actual = await fetchTemplate(
        'https://example.com/template.hsfiles',
      )(env);

      expect(actual).toStrictEqual(E.right(await data.responseOK.text()));
    });

    it('should handle HTTP errors', async () => {
      const { env, mocks } = makeTestEnv();

      mocks.mockFetch.mockResolvedValue(data.responseNotFound);

      const actual = await fetchTemplate(
        'https://example.com/not-found.hsfiles',
      )(env);
      const expected = E.left(new Error('HTTP 404: Not Found'));

      expect(actual).toStrictEqual(expected);
    });

    it('should fetch template from local file successfully', async () => {
      const { env, mocks } = makeTestEnv();
      const templateContent = 'local template content';
      mocks.mockFs.readFile.mockResolvedValue(templateContent);

      const actual = await fetchTemplate('./template.hsfiles')(env);

      expect(actual).toStrictEqual(E.right(templateContent));
      expect(mocks.mockFs.readFile).toHaveBeenCalledWith(
        './template.hsfiles',
        'utf-8',
      );
    });

    it('should handle local file read errors', async () => {
      const { env, mocks } = makeTestEnv();

      mocks.mockFs.readFile.mockRejectedValue(
        new Error('ENOENT: no such file'),
      );

      const actual = await fetchTemplate('./missing.hsfiles')(env);
      const expected = E.left(
        new Error('Failed to read template file: Error: ENOENT: no such file'),
      );

      expect(actual).toStrictEqual(expected);
    });

    it('should handle network fetch errors', async () => {
      const { env, mocks } = makeTestEnv();

      mocks.mockFetch.mockRejectedValue(new Error('Network error'));

      const actual = await fetchTemplate(
        'https://example.com/template.hsfiles',
      )(env);
      const expected = E.left(
        new Error('Failed to fetch template from URL: Error: Network error'),
      );

      expect(actual).toStrictEqual(expected);
    });
  });

  describe('parseTemplate', () => {
    it('should parse template with single file', () => {
      const { env } = makeTestEnv();

      const actual = parseTemplate(data.aSingleFileTemplateCnt)(env);

      expect(actual).toStrictEqual(data.aSingleFileTemplate);
    });

    it('should parse template with multiple files', () => {
      const { env } = makeTestEnv();

      const actual = parseTemplate(data.aMultipleFileTemplateCnt)(env);

      expect(actual).toStrictEqual(data.aMultipleFileTemplate);
    });

    it('should handle empty template', () => {
      const { env } = makeTestEnv();

      const actual = parseTemplate(data.anEmptyTemplateCnt)(env);

      expect(actual).toStrictEqual(data.anEmptyTemplate);
    });

    it('should extract variables from both filename and content', () => {
      const { env } = makeTestEnv();
      const template = [
        `{-# START_FILE {{type}}/{{name}}.ts #-}\n`,
        `export const {{name}} = "{{value}}";`,
      ].join('\n') as FetchedTemplate;

      const actual = parseTemplate(template)(env);

      expect(actual.variables).toStrictEqual(
        new Set(['type', 'name', 'value']),
      );
    });
  });

  describe('createFiles', () => {
    it('should create project directory and files successfully', async () => {
      const { env, mocks } = makeTestEnv();

      // directory doesn't exist
      mocks.mockFs.access.mockRejectedValueOnce(new Error('ENOENT'));
      mocks.mockFs.mkdir.mockResolvedValueOnce(void 0);

      const template = {
        files: [
          {
            filename: '{{name}}.ts',
            content: 'export const {{name}} = "hello";',
          },
        ],
        variables: new Set(['name']),
      };
      const values = { name: 'myProject' };

      const actual = await createFiles('test-project', template, values)(env);

      expect(actual).toStrictEqual(E.right(void 0));
      expect(mocks.mockFs.mkdir).toHaveBeenCalledExactlyOnceWith(
        expect.stringContaining('test-project'),
        { recursive: true },
      );
    });

    it('should handle existing directory error', async () => {
      const { env, mocks } = makeTestEnv();

      // mock that directory already exists
      mocks.mockFs.access.mockResolvedValue(undefined);

      const actual = await createFiles(
        'existing-project',
        data.anEmptyTemplate,
        {},
      )(env);
      const expected = E.left(
        new Error(
          `The directory ${process.cwd()}/existing-project already exists`,
        ),
      );

      expect(actual).toStrictEqual(expected);
    });

    it('should create nested directories for files', async () => {
      const { env, mocks } = makeTestEnv();

      // directory does not exist
      mocks.mockFs.access.mockRejectedValue(new Error('ENOENT'));
      mocks.mockFs.mkdir.mockResolvedValue(undefined);
      mocks.mockFs.writeFile.mockResolvedValue(undefined);

      const template = {
        files: [
          {
            filename: 'src/utils/helper.ts',
            content: 'export const helper = () => {};',
          },
        ],
        variables: data.anEmptyTemplate.variables,
      };

      const actual = await createFiles('test-project', template, {})(env);

      expect(actual).toStrictEqual(E.right(void 0));
      expect(mocks.mockFs.mkdir).toHaveBeenCalledWith(
        `${process.cwd()}/test-project`,
        {
          recursive: true,
        },
      );
      expect(mocks.mockFs.mkdir).toHaveBeenCalledWith(
        `${process.cwd()}/test-project/src/utils`,
        {
          recursive: true,
        },
      );
    });
  });
});
