# Upstream source

- Project: Yoga
- Version: 3.2.1
- Commit: `042f5013152eb81c1552dec945b88f7b95ca350f`
- Repository: https://github.com/react/yoga
- Tag: `v3.2.1`
- Source directory: `F:/yoga-3.2.1/yoga/`
- License: MIT; copied to `Public/CkYoga/yoga-3.2.1/LICENSE`

The 74 native `.h`/`.cpp` files and `LICENSE` in this module were verified as
byte-identical to the upstream recursive tree at the commit above. This module
does not build the upstream CMake project.

Verification date: 2026-09-07. The supplied files were compared using Git blob
hashes against the official commit tree; the imported files were then compared
using SHA-256 against the supplied files. `SOURCE_HASHES.sha256` records the
imported bytes, including the license. Local `.gitattributes` disables newline
conversion for this vendor directory.

Local upstream patches: none. Unreal build rules and module glue are outside the
vendor directory. Import all 19 native translation units and 55 headers; omit
language bindings, examples, benchmarks, test generators, and upstream build files.

Required Unreal build policy: precise floating-point semantics. Yoga uses NaN
sentinels and `value != value`; Unreal's default fast math invalidates that contract.

For an upgrade, choose a pinned tag/commit, verify the replacement against that
commit, update this manifest and hashes, and run the cross-module
`Ck.Yoga.Integration` tests through the consuming host's UnrealToolbox. Recheck
export and compiler requirements against the new upstream build configuration.
