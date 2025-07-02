import nodeFS from 'node:fs';
import path from 'node:path';
import {
  createFiles,
  fetchTemplate,
  parseTemplate,
} from './domain/template.js';
import { parseInputArgs, promptForVariables } from './domain/cli.js';
import { makeCli } from './adapters/nodejs/cli.js';

const env = {
  fetch,
  fs: nodeFS.promises,
  path,
  logger: console,
  cli: makeCli(),
};

const logErrorAndExit = <T>(value: T) => {
  console.error(value);
  process.exit(1);
};

// Main function
const main = async () => {
  const args = parseInputArgs(process.argv)(env);
  if (args.type === 'left') return logErrorAndExit(args.value);

  env.cli.write(`Creating project: ${args.value.projectName}\n`);

  const templateContent = await fetchTemplate(args.value.templatePath)(env);
  if (templateContent.type === 'left')
    return logErrorAndExit(templateContent.value);

  const template = parseTemplate(templateContent.value)(env);
  if (template.files.length === 0)
    return logErrorAndExit('No files found in template');

  const values = await promptForVariables(
    template.variables,
    args.value.projectName,
  )(env);
  if (values.type === 'left') return logErrorAndExit(values.value);

  const create = await createFiles(
    args.value.projectName,
    template,
    values.value,
  )(env);
  if (create.type === 'left') return logErrorAndExit(create.value);
};

// Run if called directly
if (require.main === module) {
  main();
}
