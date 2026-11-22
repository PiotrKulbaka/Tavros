# AGENTS.md

# Project Agent Instructions

## 1. General Rules

- Follow the user's instructions literally and prioritize them over your own assumptions.
- Work only within the scope of the explicitly requested task.
- Do not introduce unrelated changes, improvements, refactoring, or optimizations.
- Do not make architectural decisions without explicit approval when they affect existing project design.
- Preserve existing coding style, naming conventions, architecture, and established patterns.
- Before modifying code, inspect the relevant implementation and its dependencies.

## 2. Strict Scope Control

- Never modify code unrelated to the current task.
- Never fix existing bugs, compiler errors, linker errors, or warnings unless explicitly requested.
- Never attempt to make the entire project build successfully unless explicitly requested.
- If you encounter an unrelated problem, report it and leave it untouched.
- Do not expand the task scope based on your own judgment.

**Important:** Discovering a problem does not grant permission to fix it.

## 3. Build and Verification

- Build or run tests only when relevant to the requested task or explicitly requested by the user.
- Do not start a general debugging or error-fixing process after a failed build.
- If compilation fails:
  1. Record the errors.
  2. Determine whether they are related to your changes.
  3. If unrelated, do not modify anything to fix them.
  4. Report the result to the user.
- Never modify unrelated files just to achieve a successful build.

## 4. Code Changes

- Prefer minimal and localized changes.
- Do not rewrite entire files when a small modification is sufficient.
- Do not remove existing functionality unless explicitly requested.
- Do not add dependencies or third-party libraries without approval.
- Do not change public APIs or interfaces unless required by the task.
- Do not silently change behavior outside the requested functionality.

## 5. Working Process

For every task:

1. Understand the requested result.
2. Inspect the relevant project code.
3. Identify the minimum set of required changes.
4. Implement only those changes.
5. Review your own diff.
6. Report what was changed and what was not done.

If requirements are ambiguous or contradictory, ask the user before making assumptions.

## 6. Report

Always provide a concise report containing:

- What was implemented.
- Which files were modified.
- Whether the changes were verified.
- Any errors or unrelated problems discovered.
- Any incomplete parts of the requested task.

Never claim that something was tested or verified if it was not.

## 6. Project Tooling - tavros.py

`tavros.py` is the official command-line interface for managing the Tavros build workflow.

It provides commands for source formatting, source collection, CMake project generation, and building project targets.

### Available Commands

```text
tavros.py autoformat [--colored]
tavros.py collect_sources [--colored]
tavros.py cmake_gen (xcode | visual_studio | ninja | makefiles) [--tracy] [--collect_sources] [--autoformat] [--colored]
tavros.py build (sandbox) [--release | --debug] [--tracy] [--colored]
tavros.py --show_resolved_config [--colored]
tavros.py --help [--colored]
```

### Mandatory Rules

- Always use `tavros.py` for the main project build workflow.
- CMake project generation must be performed through `tavros.py cmake_gen`.
- Building project targets must be performed through `tavros.py build`.
- Source formatting and source collection must use their corresponding `tavros.py` commands.
- Never invoke CMake generation or project builds directly if the corresponding `tavros.py` command is available.
- Do not invent commands, arguments, targets, or options. Check `tavros.py --help` when necessary.
- Use the appropriate generator for the current development environment.

### Missing Functionality

If a required project operation is not supported by `tavros.py`:

1. Explain what functionality is missing.
2. Propose a new command, including its expected arguments and behavior.
3. Ask the user for approval before modifying `tavros.py`.
4. Only implement the new command after receiving explicit approval.
5. Use the newly implemented command for the requested operation.

Never modify `tavros.py` or introduce new CLI commands without explicit user approval.

### Tests

If test execution is not currently supported by `tavros.py`, do not invent a test command or silently introduce an alternative project workflow.

Propose the required functionality to the user and request approval before adding it to `tavros.py`.

### Important

`tavros.py` defines the official project workflow. Keep build and project-generation operations consistent with it.

Git operations, source-code inspection, file searching, and other general development utilities do not need to go through `tavros.py`.