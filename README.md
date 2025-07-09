# Boil - Project Scaffolding Tool

<p align="center">
    <img src="/docs/boil-logo.gif">
</p>

A simple project scaffolding tool that generates project structures from hsfiles-style templates.

## Features

- Generate project scaffolds from templates
- Support for local files and remote URLs
- Variable substitution with `{{variable}}` syntax
- Interactive prompts for missing variables
- Verbose output option
- Easy installation and usage

## Installation

TODO

## Usage

### Basic Usage

```bash
boil <project-name> <template-path>
```

### Examples

```bash
# Local template file
boil my-project ./templates/basic

# Remote template
boil my-project https://raw.githubusercontent.com/user/repo/main/template
```

### Remote Runner

For convenience, you can also use the remote runner script:

``` bash
curl -fsSL https://raw.githubusercontent.com/datalek/boil/main/remote.sh | bash
```

## Template Format

Boil uses a simplified hsfiles format:

```
{-# START_FILE package.json #-}
{
  "name": "{{name}}",
  "version": "1.0.0",
  "description": "{{description}}",
  "author": "{{author}}"
}

{-# START_FILE src/{{entry-point}} #-}
console.log("Hello from {{name}}!");

{-# START_FILE README.md #-}
# {{name}}

Created by {{author}}.
```

### Variable Substitution

- Use `{{variable}}` syntax for variables
- `{{name}}` is automatically set to the project name
- All other variables will be prompted interactively
- Variables can be used in both filenames and content

## Development

1. Install dependencies and setup tools: (see [Requirements](#requirements) below).
2. Install project dependencies: `npm i`.
3. Run development tasks: `npm run build`, `npm run dev`.

### Requirements

This project requires the following tools to works as expected. Make sure to install all the following dependencies using the recommended installation methods.

- **Node.js**</br>
  Use [fnm](https://github.com/Schniz/fnm#installation) to install the required version of `Node.js`.

  ```sh
  # uses `engines.node` field in `package.json`
  fnm install
  # check that the installed version is correct
  node --version
  ```

- **npm**</br>
  Already provided by node!

### Local Development

```bash
# Clone the repository
git clone https://github.com/datalek/boil.git
cd boil

# Install dependencies
npm i

# Use the dev script (builds and runs)
npm run local my-test-project ./template --verbose
```

### Building

```bash
# Production build
npm run build

# Clean build artifacts
npm run clean
```

## License

MIT
