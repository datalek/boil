import { FetchedTemplate, Template } from '../template.js';

export const responseOK = {
  ok: true,
  text: () => Promise.resolve('template content'),
} as unknown as Response;

export const responseNotFound = {
  ok: false,
  status: 404,
  statusText: 'Not Found',
} as unknown as Response;

export const anEmptyTemplateCnt = `` as FetchedTemplate;

export const anEmptyTemplate: Template = {
  files: [],
  variables: new Set(),
};

export const aSingleFileTemplateCnt = `{-# START_FILE package.json #-}
{
  "name": "{{name}}",
  "version": "1.0.0"
}` as FetchedTemplate;

export const aSingleFileTemplate: Template = {
  files: [
    {
      filename: 'package.json',
      content: '{\n  "name": "{{name}}",\n  "version": "1.0.0"\n}\n',
    },
  ],
  variables: new Set(['name']),
};

export const aMultipleFileTemplateCnt = `{-# START_FILE src/{{name}}.ts #-}
export const hello = "{{greeting}}";

{-# START_FILE README.md #-}
# {{name}}

A project by {{author}}.` as FetchedTemplate;

export const aMultipleFileTemplate: Template = {
  files: [
    {
      filename: 'src/{{name}}.ts',
      content: 'export const hello = "{{greeting}}";\n\n',
    },
    {
      filename: 'README.md',
      content: '# {{name}}\n\nA project by {{author}}.\n',
    },
  ],
  variables: new Set(['name', 'greeting', 'author']),
};
