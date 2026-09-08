# Gate 0: Native Yoga integration

Written: 2026-09-07. Status and evidence: [../PROGRESS.md](../PROGRESS.md).

## Entry

- Inspect and preserve unrelated checkout changes.
- Fetch and rebase the four Ck plugin campaign branches onto `origin/dev`.
- Verify supplied native source and license against upstream v3.2.1.

## Work and verification

1. Import all 19 native C++ translation units and 55 headers plus license unchanged; record upstream identity and hashes. Verify every copied file against the supplied source and official Git blob identity.
2. Add a standalone Runtime CkYoga module with C++20, isolated compiler settings, and public native headers. Verify registration, platform declarations, and export behavior through a separate consumer module.
3. Add focused CkTests native API cases for layout, resize/invalidation, and measured leaves. Tests own nodes/configuration and release them in the correct order. No adapter or reflection API is introduced.
4. Generate project files and incrementally build/test through the selected host's UnrealToolbox. Use one test lane for this small group and inspect process exit, full test summary, and fresh startup diagnostics. Toolbox may launch additional discovery processes even with `--parallel 1`; do not describe a single invocation as a single editor boot. Actual runs and the corrected initial estimate are recorded in PROGRESS.md.

## Expected branches

- Missing import symbols: inspect module-local export flags and consumer link command, preserving upstream headers.
- Vendor compile failure: inspect the first compiler error against Yoga's own build policy; fix module settings before considering a source patch.
- Startup or unrelated compile failure after upstream update: report the first causal error and establish ownership before claiming it is inherited. Preserve unrelated work.

## Exit

Record copied-file count/hash result, current plugin baselines, build configuration and exit, test names/counts, fresh diagnostic scan, and remaining platform/UI limitations. Present Gate 1 for review; do not implement it in Gate 0.
