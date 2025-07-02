import readline from 'node:readline';
import { CliEnv } from '../../domain/cli.js';

export const makeCli = (): CliEnv['cli'] => ({
  write: (str: string) =>
    // maybe wrap in a promise in the future
    process.stdout.write(str),

  prompt: (question: string): Promise<string> =>
    // eslint-disable-next-line functional/no-return-void
    new Promise((resolve) => {
      const terminal = readline.createInterface({
        input: process.stdin,
        output: process.stdout,
      });

      // eslint-disable-next-line functional/no-return-void
      terminal.question(question, (answer) => {
        terminal.close();
        resolve(answer.trim());
      });
    }),
});
