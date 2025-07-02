import * as E from './either.js';
import { Template } from './template.js';

export interface CliEnv {
  readonly cli: {
    readonly write: (str: string) => boolean;
    readonly prompt: (question: string) => Promise<string>;
  };
}

interface InputArgs {
  readonly projectName: string;
  readonly templatePath: string;
}

export const parseInputArgs =
  (argv: readonly string[]) =>
  (env: CliEnv): E.Either<string, InputArgs> => {
    // the first argument is node, the second is the file executed
    const args = argv.slice(2);

    if (args.length < 2) {
      const message = [
        'Usage: boil <project-name> <template-path>',
        '',
        'Examples:',
        '  boil my-project ./templates/basic',
        '  boil my-app https://raw.githubusercontent.com/user/repo/main/template.hsfiles',
        '  boil my-project ./template.hsfiles --verbose',
      ].join('\n');

      // eslint-disable-next-line functional/no-expression-statements
      env.cli.write(message);
      return E.left(message);
    } else {
      return E.right({
        projectName: args[0],
        templatePath: args[1],
      });
    }
  };

export const promptForVariables =
  (variables: Template['variables'], projectName: string) =>
  async (env: CliEnv): Promise<E.Either<Error, Record<string, string>>> => {
    const values: Record<string, string> = { name: projectName };

    // the variable name is always the project-name, so it is already filled
    const variablesToPrompt = Array.from(variables).filter((v) => v !== 'name');

    // no variable to prompt
    if (variablesToPrompt.length === 0) {
      return E.right(values);
    }
    // there is at least one variable to prompt
    else {
      // eslint-disable-next-line functional/no-expression-statements
      env.cli.write('Please provide values for the following variables:\n');

      // eslint-disable-next-line functional/no-loop-statements
      for (const variable of variablesToPrompt) {
        const value = await E.tryCatch(
          () => env.cli.prompt(`${variable}: `),
          (e) => new Error(`Error prompting variable '${variable}', ${e}`),
        );
        if (value.type === 'left') return value;
        // eslint-disable-next-line functional/no-expression-statements, functional/immutable-data
        values[variable] = value.value.trim();
      }

      return E.right(values);
    }
  };
