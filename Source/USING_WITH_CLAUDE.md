# Using CkFoundation with Claude Code / Claude Cowork

This guide documents what the documentation structure does, where it is, and how to work with it effectively. Written for both developers onboarding to CkFoundation and for Claude itself as a reference when reasoning about the codebase.

---

## What was built

137 documentation files were added to the repository:

| Location | Count | Purpose |
|---|---|---|
| `/Source/CLAUDE.md` | 1 (updated) | Root: code style + new module index + decision tree + cross-module patterns |
| `/Source/<Module>/Claude.md` | 88 | Per-module purpose, key API, patterns, anti-patterns, cross-references |
| `/Source/CkCore/Public/CkCore/<Folder>/README.md` | 48 | Per-subfolder API details (15 full, 33 stubs) |
| `/Source/CkCore/Claude.md` | 1 | CkCore use-case lookup table for all 48 subfolders |
| `/Source/EDITOR_MODULES.md` | 1 | Combined reference for all editor modules |

---

## Expected behavior after this structure

When Claude encounters a task on this codebase, it should now:

1. **Find utilities without redirection.** "Where's the string fuzzy match?" → CLAUDE.md decision tree → CkCore/Claude.md → String/README.md. No manual "check CkCore."
2. **Find module APIs without reading full headers.** "How do I add an audio track?" → CkAudio/Claude.md has the exact call and pattern.
3. **Avoid duplicating existing utilities.** The anti-patterns sections in CkCore/Claude.md and each module file say explicitly what not to reimplement.
4. **Correctly model dependencies.** Before adding a new dependency to a feature module, Claude can check the Tier table in CLAUDE.md section 3 to see whether the dependency is valid given the module's tier.

---

## How to read the docs efficiently

For a **new feature task:**

1. Read `/Source/CLAUDE.md` section 3 (Module Index / Decision Tree) — find the right module(s).
2. Read `<Module>/Claude.md` — understand purpose, key API, and anti-patterns.
3. Look at `CkCore/Claude.md` for any utility operations needed (validation, formatting, time, etc.).
4. Drill into `CkCore/Public/CkCore/<Folder>/README.md` only if you need the full API signature.

For a **bug fix or existing feature:**

1. Identify the module from the file path.
2. Read that module's `Claude.md` to understand intended behavior and common pitfalls.
3. Check the anti-patterns section first — the bug may be a known pitfall.

For an **editor feature:**

1. Read `/Source/EDITOR_MODULES.md` for the combined editor module reference.
2. Pair the `*Editor` module with its runtime twin's `Claude.md`.

---

## Known limitations / things to verify before trusting

| What | Status | Note |
|---|---|---|
| All 15 full CkCore READMEs | Verified from headers | Method names confirmed against source |
| All 33 stub CkCore READMEs | Inferred from filenames + brief reads | May not capture full API — verify before authoring |
| Tier A module Claude.md (12 files) | Headers read directly | `ck::SelfEntity(this)` from root CLAUDE.md not found in accessible headers — documented as "per CLAUDE.md; verify" |
| Tier B module Claude.md (56 files) | Templated from header scans + build.cs deps | Key API verified for ~20 modules; remainder inferred from naming conventions |
| Tier C editor modules | Stubs only | Point to EDITOR_MODULES.md |
| Dependency table in CLAUDE.md section 3 | Generated from build.cs files | Exact; will drift if build.cs files change |
| `Is_PlaceholderClass` method name | Verified against `CkReflection_Utils.h:81` | Was `Get_IsReinstancingPlaceholder` in an earlier draft — corrected |

---

## Maintenance

**When you add a new module:**
1. Create `<NewModule>/Claude.md` using the template in any Tier B file.
2. Add a row to CLAUDE.md section 3 module tier table.
3. Add to CLAUDE.md decision tree if the module owns a new use-case verb.

**When you rename a module:**
1. Update its Claude.md.
2. Update the tier table in CLAUDE.md.
3. Update any "See also" references in related module Claude.md files.
4. Update build.cs accordingly (the tier table was generated from build.cs).

**When a module's API changes significantly:**
1. Update that module's Claude.md.
2. If it changes CkCore behavior, update the relevant CkCore subfolder README.

**When the build.cs dependency changes:**
1. Update the tier table entry in CLAUDE.md section 3.
2. If a tier changes (e.g., new dep on a higher-tier module), note the new tier.

---

## When to intervene manually

Claude should find the right module without redirection in ~95% of cases after reading these docs. Intervene when:

- Claude proposes a module architecture that contradicts a tier constraint (e.g., CkCore depending on CkEcs — not allowed).
- Claude tries to implement something that already exists in CkCore and the anti-patterns section should have caught it.
- A stub README (33 of the 48 CkCore subfolders) turns out to be inaccurate for the task at hand — expand it at that point.
- The Tier B module Claude.md has a method name that doesn't compile — those were inferred; verify against the header and update the doc.

---

## File locations quick reference

```
/Source/
├── CLAUDE.md              ← Start here for any task
├── EDITOR_MODULES.md      ← All editor modules
├── USING_WITH_CLAUDE.md   ← This file
│
├── CkCore/
│   ├── Claude.md          ← CkCore use-case lookup table
│   └── Public/CkCore/
│       └── <Folder>/
│           └── README.md  ← Per-folder API details
│
└── <Module>/
    └── Claude.md          ← Per-module docs
```
