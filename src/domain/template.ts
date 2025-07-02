import * as E from './either.js';
import { LoggerEnv } from './logger.js';
import nodeFS from 'node:fs';
import nodePath from 'node:path';

export interface TemplateEnv {
  readonly fetch: typeof fetch;
  readonly fs: {
    readonly constants: (typeof nodeFS)['promises']['constants'];
    // (typeof nodeFS)['promises']['readFile'] doesn't work well with mock
    // because the overloading
    readonly readFile: (
      path: nodeFS.PathLike,
      options: BufferEncoding,
    ) => Promise<string>;
    readonly access: (typeof nodeFS)['promises']['access'];
    // (typeof nodeFS)['promises']['mkdir'] doesn't work well with mock
    // because the overloading
    readonly mkdir: (
      path: nodeFS.PathLike,
      options: { readonly recursive: true },
    ) => Promise<string | undefined>;
    readonly writeFile: (typeof nodeFS)['promises']['writeFile'];
  };
  readonly path: {
    readonly dirname: (typeof nodePath)['dirname'];
    readonly resolve: (typeof nodePath)['resolve'];
    readonly join: (typeof nodePath)['join'];
  };
}

export type FetchedTemplate = string & { readonly __brand: 'FetchedTemplate' };

interface TemplateFile {
  readonly filename: string;
  readonly content: string;
}

export interface Template {
  readonly files: readonly TemplateFile[];
  readonly variables: ReadonlySet<string>;
}

export const fetchTemplate =
  (path: string) =>
  async (
    env: TemplateEnv & LoggerEnv,
  ): Promise<E.Either<Error, FetchedTemplate>> => {
    const { fetch, logger, fs } = env;

    logger.info(`Fetching template from: ${path}`);

    // Read from remote file
    if (path.startsWith('http://') || path.startsWith('https://')) {
      // Fetch from URL
      const response = await E.tryCatch(
        () => fetch(path),
        (e) => new Error(`Failed to fetch template from URL: ${e}`),
      );
      if (response.type === 'right' && response.value.ok)
        return E.right((await response.value.text()) as FetchedTemplate);
      else if (response.type === 'right' && !response.value.ok) {
        const errorMsg = `HTTP ${response.value.status}: ${response.value.statusText}`;
        return E.left(new Error(errorMsg));
      } else if (response.type === 'left') return response;
      else return E.left(new Error());
    }
    // Read from local file
    else {
      return E.tryCatch(
        () => fs.readFile(path, 'utf-8').then((s) => s as FetchedTemplate),
        (e) => new Error(`Failed to read template file: ${e}`),
      );
    }
  };

export const parseTemplate =
  (template: FetchedTemplate) =>
  (env: TemplateEnv & LoggerEnv): Template => {
    env.logger.info('Parsing template...');

    const startFileRegex = /^\{-#\s*START_FILE\s+(.+?)\s*#-\}$/;

    const lines = template.split('\n');

    const empty: Template['files'] = [];
    const file: TemplateFile = { filename: '', content: '' };
    const zero = { files: empty, file };

    const { files: partial, file: last } = lines.reduce(
      ({ files, file }, line) => {
        const startFileMatch = line.match(startFileRegex);

        // scenario: new file entry
        if (startFileMatch) {
          const filename = startFileMatch[1].trim();
          const newFile = { filename, content: '' };
          return {
            files: [...files, file],
            file: newFile,
          };
        }
        // scenario: working on a file
        else {
          const { filename, content } = file;
          const updatedFile = { filename, content: content + line + '\n' };
          return { files, file: updatedFile };
        }
      },
      zero,
    );

    const files = [
      // .slice(1) because the first file is always an empry file.
      ...partial.slice(1),
      // the last file could be and empry file, add only if it is not.
      ...(last.filename !== '' ? [last] : []),
    ];

    const extractVariables = (text: string): readonly string[] =>
      Array.from(text.matchAll(/\{\{([^}]+)\}\}/g)).map((match) =>
        match[1].trim(),
      );

    const variables = new Set(
      files.flatMap((file) => [
        ...extractVariables(file.filename),
        ...extractVariables(file.content),
      ]),
    );

    env.logger.info(
      `Found ${files.length} files and ${variables.size} variables`,
    );
    return { files, variables };
  };

const substituteVariables = (
  content: string,
  values: Record<string, string>,
): string => {
  // eslint-disable-next-line functional/no-let
  let result = content;

  // eslint-disable-next-line functional/no-loop-statements
  for (const [key, value] of Object.entries(values)) {
    const regex = new RegExp(`\\{\\{\\s*${key}\\s*\\}\\}`, 'g');
    // eslint-disable-next-line functional/no-expression-statements
    result = result.replace(regex, value);
  }

  return result;
};

export const createFiles =
  (root: string, template: Template, values: Record<string, string>) =>
  async (env: TemplateEnv & LoggerEnv): Promise<E.Either<Error, void>> => {
    const { fs, path, logger } = env;

    const rootPath = path.resolve(root);
    logger.info(`Creating project directory: ${root}`);

    const rootExists = await E.tryCatch(
      () => fs.access(rootPath, fs.constants.F_OK),
      () => new Error(),
    );

    if (rootExists.type === 'right')
      return E.left(new Error(`The directory ${rootPath} already exists`));
    else {
      const rootCreate = await E.tryCatch(
        () => fs.mkdir(rootPath, { recursive: true }),
        (e) => new Error(`Error creating ${rootPath} ${e}`),
      );

      if (rootCreate.type === 'left') return rootCreate;

      // eslint-disable-next-line functional/no-expression-statements
      await Promise.all(
        template.files.map(async (file) => {
          const filename = substituteVariables(file.filename, values);
          const content = substituteVariables(file.content, values);
          const filePath = path.join(rootPath, filename);

          logger.info(`Creating file: ${filename}`);

          // Ensure directory exists
          const dir = path.dirname(filePath);
          const dirExists = await E.tryCatch(
            () => fs.access(dir, fs.constants.F_OK),
            () => new Error(),
          );
          if (dirExists.type === 'left') {
            // eslint-disable-next-line functional/no-expression-statements
            await E.tryCatch(
              () => fs.mkdir(dir, { recursive: true }),
              (e) => new Error(`Error creating ${dir}, ${e}`),
            );
          }

          // Write file (remove trailing newline that was added during parsing)
          // eslint-disable-next-line functional/no-expression-statements
          await E.tryCatch(
            () => fs.writeFile(filePath, content.replace(/\n$/, '')),
            (e) => new Error(`Error creating ${dir}, ${e}`),
          );
        }),
      );
    }

    logger.info(`Project '${rootPath}' created successfully!`);
    logger.info(`Generated ${template.files.length} files`);
    return E.right(void 0);
  };
