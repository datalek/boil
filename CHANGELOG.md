# boil

## 0.1.0

### Minor Changes

- 0da729a: Add --reverse command to create templates from folders

  This adds a new reverse mode that generates hsfiles templates from existing project folders. Use `boil --reverse <folder-path> <output-template-path>` to convert any project into a reusable template.

### Patch Changes

- 2895125: Fix release PR not triggering required workflows

  Switch from GITHUB_TOKEN to PAT_TOKEN in the release workflow so that PRs created by changesets/action trigger the pr-checks workflow.

## 0.0.1

### Patch Changes

- 015d47c: Update permission on release workflow
- 75a95bc: Test the release workflow
