# Development Workflow

## Where things live
- This folder: source code, README, build scripts, and Git history.
- GitHub: private remote repository, pull requests, and Windows CI.
- Obsidian: milestone checklist and dated progress notes only.

## Everyday workflow
1. Update `main` with `git pull --ff-only`, then create a branch for one feature or fix.
2. Make focused changes and inspect `git diff`.
3. Build with `build.ps1`, run CTest, and manually check behavior affected by the change.
4. Commit with a descriptive message and push the branch.
5. Open a pull request against `main`. Review its diff and wait for the Windows build to pass.
6. Merge the PR, update local `main`, and delete the merged feature branch. Update Obsidian when a milestone is complete.

```powershell
git switch main
git pull --ff-only
git switch -c feature/example

# After implementing and checking the change:
powershell -ExecutionPolicy Bypass -File .\build.ps1
& 'E:\QTCreator\Tools\CMake_64\bin\ctest.exe' --test-dir build --output-on-failure
git diff
git add <changed-files>
git commit -m "feat: describe the change"
git push -u origin feature/example
```

## How Git works
- Working files: your current edits.
- Staging (`git add`): choose edits for the next checkpoint.
- Commit: save a checkpoint locally.
- Push: copy your commits to GitHub.
- Branch: an independent line of work.
- Pull request: review a branch before merging it into `main`.
- `origin`: the short name for this project's GitHub remote.

## Checks and conventions
- Keep `main` buildable and runnable; use branches for changes.
- GitHub Actions builds the C++ app and temperature helper on Windows, then runs the C++ tests and helper checks for PRs and `main` pushes.
- CI uses MSVC with Qt 6.8.3 as a compatibility baseline; local builds use MinGW with Qt 6.12.0.
- Automated checks catch regressions; manually verify relevant live hardware behavior too.
- `.gitignore` excludes generated output and personal editor files.
- `.gitattributes` keeps line endings consistent; `.editorconfig` describes formatting.
- No automated merging or publishing is configured.
- Avoid committing secrets or force-pushing shared history.
- Keep comments focused on intent or non-obvious behavior.
