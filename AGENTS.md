# AGENTS.md

## Purpose
This repository builds a runtime plugin framework for Super Smash Bros. Brawl (Wii), where C/C++ is compiled with MWCC (MetroWerks C++ Compiler for Nintendo Wii) into .rel modules. These modules are the Wii's equivalent of relocatable elf files.

The primary engineering goal is to minimize memory usage and code size while preserving runtime stability inside the game process.

## Project Context
- Plugins are loaded at runtime and can hook/patch game code.
- The framework provides a Core API, hook machinery, plugin lifecycle management, and event dispatch.
- Runtime constraints are strict: low available memory and high sensitivity to crashes.

## Optimization Priorities
When implementing changes, prioritize in this order:
1. Correctness and crash prevention in hook/lifecycle paths.
2. Lower heap churn and fragmentation.
3. Smaller code footprint and reduced branching on hot paths.
4. Predictable behavior during scene/module transitions.

## Required Workflow
These are mandatory moving forward:
1. After every code change, run a full verification build:
   - make clean && make
2. Commit every change or small set of related changes in logical groups.
3. Use informative commit messages that describe intent and risk.
4. If a fix addresses a bug or potential bug, state that explicitly in the commit message/body.
5. After grouped commits, report:
   - Number of commits created
   - Exact command to reset HEAD back to before those commits without losing the changes (for example, git reset HEAD~n)

## Current Commit Style
Use concise titles with direct impact statements, for example:
- Fix hook removal loop skipping entries for same owner
- Fix OPT_NO_RETURN being overwritten during hook payload build
- Harden plugin lifecycle against null and duplicate-load edge cases

Recommended format:
- Subject: action + component + impact
- Body line 1: what changed
- Body line 2: why it matters (bug avoided, memory reduced, behavior stabilized)

## Guardrails
- Do not revert unrelated changes in the working tree.
- Treat submodule changes as independent unless explicitly requested.
- Keep patches minimal and localized.
- Prefer low-risk, measurable optimizations in core/runtime paths unless requested by the user.