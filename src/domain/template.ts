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
    readonly readdir: (
      path: nodeFS.PathLike,
      options: { readonly withFileTypes: true },
    ) => Promise<readonly nodeFS.Dirent[]>;
    readonly stat: (path: nodeFS.PathLike) => Promise<nodeFS.Stats>;
  };
  readonly path: {
    readonly dirname: (typeof nodePath)['dirname'];
    readonly resolve: (typeof nodePath)['resolve'];
    readonly join: (typeof nodePath)['join'];
    readonly relative: (typeof nodePath)['relative'];
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

interface CollectedFile {
  readonly relativePath: string;
  readonly content: string;
}

const collectFiles =
  (rootPath: string, currentPath: string) =>
  async (
    env: TemplateEnv & LoggerEnv,
  ): Promise<E.Either<Error, readonly CollectedFile[]>> => {
    const { fs, path } = env;

    const entries = await E.tryCatch(
      () => fs.readdir(currentPath, { withFileTypes: true }),
      (e) => new Error(`Failed to read directory ${currentPath}: ${e}`),
    );

    if (entries.type === 'left') return entries;

    // eslint-disable-next-line functional/prefer-readonly-type
    const files: CollectedFile[] = [];

    // eslint-disable-next-line functional/no-loop-statements
    for (const entry of entries.value) {
      const fullPath = path.join(currentPath, entry.name);

      if (entry.isDirectory()) {
        const subFiles = await collectFiles(rootPath, fullPath)(env);
        if (subFiles.type === 'left') return subFiles;
        // eslint-disable-next-line functional/no-expression-statements, functional/immutable-data
        files.push(...subFiles.value);
      } else if (entry.isFile()) {
        const content = await E.tryCatch(
          () => fs.readFile(fullPath, 'utf-8'),
          (e) => new Error(`Failed to read file ${fullPath}: ${e}`),
        );
        if (content.type === 'left') return content;

        const relativePath = path.relative(rootPath, fullPath);
        // eslint-disable-next-line functional/no-expression-statements, functional/immutable-data
        files.push({ relativePath, content: content.value });
      }
    }

    return E.right(files);
  };

const generateTemplateContent = (files: readonly CollectedFile[]): string => {
  return files
    .map((file) => `{-# START_FILE ${file.relativePath} #-}\n${file.content}`)
    .join('\n');
};

export const createTemplateFromFolder =
  (folderPath: string, outputPath: string) =>
  async (env: TemplateEnv & LoggerEnv): Promise<E.Either<Error, void>> => {
    const { fs, path, logger } = env;

    const resolvedFolder = path.resolve(folderPath);
    const resolvedOutput = path.resolve(outputPath);

    logger.info(`Creating template from folder: ${resolvedFolder}`);

    // Check if folder exists
    const folderExists = await E.tryCatch(
      () => fs.stat(resolvedFolder),
      () => new Error(`Folder does not exist: ${resolvedFolder}`),
    );

    if (folderExists.type === 'left') return folderExists;
    if (!folderExists.value.isDirectory())
      return E.left(new Error(`Path is not a directory: ${resolvedFolder}`));

    // Check if output already exists
    const outputExists = await E.tryCatch(
      () => fs.access(resolvedOutput, fs.constants.F_OK),
      () => new Error(),
    );

    if (outputExists.type === 'right')
      return E.left(new Error(`Output file already exists: ${resolvedOutput}`));

    // Collect all files
    const collectedFiles = await collectFiles(
      resolvedFolder,
      resolvedFolder,
    )(env);

    if (collectedFiles.type === 'left') return collectedFiles;

    if (collectedFiles.value.length === 0)
      return E.left(new Error('No files found in folder'));

    logger.info(`Found ${collectedFiles.value.length} files`);

    // Generate template content
    const templateContent = generateTemplateContent(collectedFiles.value);

    // Ensure output directory exists
    const outputDir = path.dirname(resolvedOutput);
    const outputDirExists = await E.tryCatch(
      () => fs.access(outputDir, fs.constants.F_OK),
      () => new Error(),
    );
    if (outputDirExists.type === 'left') {
      const createDir = await E.tryCatch(
        () => fs.mkdir(outputDir, { recursive: true }),
        (e) => new Error(`Failed to create output directory: ${e}`),
      );
      if (createDir.type === 'left') return createDir;
    }

    // Write template file
    const writeResult = await E.tryCatch(
      () => fs.writeFile(resolvedOutput, templateContent),
      (e) => new Error(`Failed to write template file: ${e}`),
    );

    if (writeResult.type === 'left') return writeResult;

    logger.info(`Template created successfully: ${resolvedOutput}`);
    logger.info(`Generated template with ${collectedFiles.value.length} files`);
    return E.right(void 0);
  };
