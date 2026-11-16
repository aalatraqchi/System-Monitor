# Development

## Workflow

1. Update `main` with `git pull --ff-only`, then create a branch for one feature or fix.
2. Build with `build.ps1`, run CTest, and check any behavior the change affects by hand, especially live hardware readings.
3. Commit with a descriptive message and push the branch.
4. Open a pull request against `main` and wait for the Windows build to pass before merging.

```powershell
git switch main
git pull --ff-only
git switch -c feature/example

powershell -ExecutionPolicy Bypass -File .\build.ps1
ctest --test-dir build --output-on-failure

git add <changed-files>
git commit -m "feat: describe the change"
git push -u origin feature/example
```

## Continuous integration

[`windows-build.yml`](../.github/workflows/windows-build.yml) runs on every pull request and every push to `main`. On a GitHub-hosted Windows runner it:

1. Installs Qt 6.8.3 for MSVC 2022, a compatibility baseline older than the Qt version used for local MinGW builds.
2. Configures and builds the app and tests with CMake.
3. Builds the temperature helper and runs its sensor-selection self-test.
4. Runs every CTest suite, including a `--smoke-test` launch of the app.

Third-party actions are pinned to full commit SHAs, and the workflow has read-only repository permissions.

## Conventions

- Keep `main` buildable and runnable. All changes go through pull requests.
- Prefer comments that explain intent or non-obvious behavior.
- `.editorconfig` describes formatting and `.gitattributes` keeps line endings consistent (LF for source, CRLF for PowerShell).
- `.gitignore` excludes build output, local SDKs and editor files. Never commit credentials or `.env` files.
- MinGW builds compile with `-Wall -Wextra -Wpedantic`.
