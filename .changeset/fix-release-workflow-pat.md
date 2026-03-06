---
"boil": patch
---

Fix release PR not triggering required workflows

Switch from GITHUB_TOKEN to PAT_TOKEN in the release workflow so that PRs created by changesets/action trigger the pr-checks workflow.
