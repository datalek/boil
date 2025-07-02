import jseslint from '@eslint/js';
import tseslint from 'typescript-eslint';
import prettier from 'eslint-plugin-prettier/recommended';
import fneslint from 'eslint-plugin-functional';

const tseslintConfig = [
  ...tseslint.config(
    ...tseslint.configs.strict,
    ...tseslint.configs.stylistic,
    {
      files: ['**/*.ts', '**/*.tsx'],
      languageOptions: {
        parserOptions: {
          project: true,
          tsconfigDirName: import.meta.dirname,
        },
      },
    },
  ),
];

const prettierConfig = [
  {
    ...prettier,
    rules: {
      ...prettier.rules,
      'prettier/prettier': [
        'error',
        {
          // Prettier options
          singleQuote: true,
          jsxSingleQuote: true,
        },
      ],
    },
  },
];

const fneslintConfig = [
  {
    plugins: {
      functional: fneslint,
    },
    rules: {
      // No exceptions
      'functional/no-promise-reject': 'error',
      'functional/no-throw-statements': 'error',
      'functional/no-try-statements': 'error',
      // No mutations
      'functional/immutable-data': 'error',
      'functional/no-let': 'error',
      'functional/prefer-readonly-type': 'error',
      // No other paradigms
      'functional/no-this-expressions': 'error',
      // No statements, ignoreVoid allows expression of type void and
      // Promise<void> are not flagged as violations
      'functional/no-expression-statements': ['error', { ignoreVoid: true }],
      'functional/no-loop-statements': 'error',
      'functional/no-return-void': 'error',
      // Stylistic
      'functional/prefer-property-signatures': 'warn',
      'functional/prefer-tacit': 'warn',
      // Vanilla
      'no-var': 'error',
      'no-param-reassign': 'error',
    },
  },
  {
    // Disables some rules that would raise errors
    // in the file main, the entrypoiny of the program
    files: ['**/main.ts'],
    rules: {
      'functional/no-expression-statements': 'off',
      'functional/no-return-void': 'off',
    },
  },
  {
    // Disables some rules that would raise errors
    // in the files where the tests are defined
    files: ['**/*.+(test|spec).ts'],
    rules: {
      'functional/no-expression-statements': 'off',
      'functional/no-return-void': 'off',
    },
  },
];

export default [
  // Load js rules
  jseslint.configs.recommended,
  // Define rules to enforce functional paradigm
  ...fneslintConfig,
  // Load ts strict and stylistic config
  ...tseslintConfig,
  // Runs Prettier as an ESLint rule and reports
  // differences as individual ESLint issues
  ...prettierConfig,
  {
    // Ignore everything under any dist/ directory
    ignores: ['**/dist/'],
  },
];
