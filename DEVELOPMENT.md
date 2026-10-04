# Development Workflow

## Where things live
- This folder: source code, README, build scripts, and Git history.
- GitHub: private remote repository and pull requests.

## Everyday workflow
1. Start from an up-to-date `main` branch.
2. Create one branch per feature or fix, such as `feature/ram-monitoring`.
3. Make small, focused changes and inspect `git diff`.
4. Build with `build.ps1`, run the startup check, and manually check the behavior.
5. Commit with a descriptive message, such as `feat: display physical RAM usage`.
6. Push the branch and open a pull request against `main`.
7. Review the diff yourself and wait for the Windows build check to pass.
8. Squash-merge the PR, then update local `main`.

```powershell
git switch main
git pull --ff-only
git switch -c feature/ram-monitoring

# After implementing and checking the change:
powershell -ExecutionPolicy Bypass -File .\build.ps1
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build --output-on-failure
git diff
git add src/SystemMonitor.cpp
# Stage any other changed files intentionally.
git commit -m "feat: display physical RAM usage"
git push -u origin feature/ram-monitoring
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
- `main` should always build and run. Use feature branches for new work.
- GitHub Actions builds on Windows and runs the startup check for PRs and `main` pushes.
- CI uses MSVC with Qt 6.8.3 as a compatibility baseline; local builds use MinGW with Qt 6.12.0.
- Startup checks catch launch failures; manually verify actual features too.
- `.gitignore` excludes generated output and personal editor files.
- `.gitattributes` keeps line endings consistent; `.editorconfig` describes formatting.
- No automated merging or publishing is configured.
- Avoid committing secrets or force-pushing shared history.
- Keep comments focused on intent or non-obvious behavior.
