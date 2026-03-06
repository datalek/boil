import nodeFS from 'node:fs';
import path from 'node:path';
import {
  createFiles,
  createTemplateFromFolder,
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

// Handle create mode: template -> project
const handleCreate = async (projectName: string, templatePath: string) => {
  env.cli.write(`Creating project: ${projectName}\n`);

  const templateContent = await fetchTemplate(templatePath)(env);
  if (templateContent.type === 'left')
    return logErrorAndExit(templateContent.value);

  const template = parseTemplate(templateContent.value)(env);
  if (template.files.length === 0)
    return logErrorAndExit('No files found in template');

  const values = await promptForVariables(template.variables, projectName)(env);
  if (values.type === 'left') return logErrorAndExit(values.value);

  const create = await createFiles(projectName, template, values.value)(env);
  if (create.type === 'left') return logErrorAndExit(create.value);
};

// Handle reverse mode: folder -> template
const handleReverse = async (folderPath: string, outputPath: string) => {
  env.cli.write(`Creating template from folder: ${folderPath}\n`);

  const result = await createTemplateFromFolder(folderPath, outputPath)(env);
  if (result.type === 'left') return logErrorAndExit(result.value);

  env.cli.write(`Template created: ${outputPath}\n`);
};

// Main function
const main = async () => {
  const args = parseInputArgs(process.argv)(env);
  if (args.type === 'left') return logErrorAndExit(args.value);

  if (args.value.type === 'reverse') {
    return handleReverse(args.value.folderPath, args.value.outputPath);
  } else {
    return handleCreate(args.value.projectName, args.value.templatePath);
  }
};

// Run if called directly
if (require.main === module) {
  main();
}
