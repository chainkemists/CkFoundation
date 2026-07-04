# sync-skills — surface Ck plugin skills in a consuming superproject

## Problem

Ck skills live in the plugin repo that owns them (`<PluginRepo>/.claude/skills/<name>/SKILL.md`) so
they version with the code — CkFoundation carries the framework skills, CkTests the testing skill,
CkGameplayDebugger the debugger skill. Claude Code, however, only auto-discovers skills at the
session's project root (plus the user's `~/.claude/skills` and `--add-dir` roots). A session rooted
at a superproject (e.g. BusterBlock) does not see skills inside submodules.

## Solution

`sync-skills.ps1` creates NTFS **directory junctions** from `<superproject>/.claude/skills/<name>`
to each plugin's skill directory. Run it from the superproject root:

```powershell
pwsh Plugins/CkFoundation/.claude/scripts/sync-skills.ps1
```

Options: `-SuperprojectRoot <path>`, `-PluginSkillDirs <paths>` (skip auto-discovery),
`-Prune` (remove junctions whose targets vanished), `-DryRun`.

Idempotent — safe to re-run after every `git submodule update`. It never writes into the plugin
repos and never deletes real directories (a non-junction directory with a colliding name is warned
about and left alone). Junctions were chosen over symlinks because creating them requires no admin
rights or Developer Mode on Windows.

**First run only:** if `<superproject>/.claude/skills` did not exist before, restart any open
Claude Code session afterward — a newly created top-level skills directory is only discovered at
session start (the script prints the same warning at the moment it creates the directory).

## Claude Code discovery behavior (verified against docs 2026-07-02)

Source: `https://code.claude.com/docs/en/skills.md` (doc build 2026-06-30). Re-verify at that URL
when this file is older than a few months.

| Fact | Status |
|---|---|
| Project-root `.claude/skills/<name>/SKILL.md` is auto-discovered | Documented |
| Submodule `.claude/skills` are NOT discovered from a superproject session | Inferred from docs (discovery climbs parents "up to the repository root"; nothing documents descending into submodules) — consistent with observed behavior |
| A `<skill-name>` entry may be a **symlink**; Claude Code follows it | Documented |
| NTFS **junctions** specifically | NOT named in docs. Junction mechanics (create, read-through, idempotent re-run, prune) validated locally 2026-07-02 on Windows 11 / PowerShell 7. Junctions are resolved by the filesystem layer, below the process — expected equivalent to symlinks. Live-session skill loading through a junction: verify once in your own session (run the script, restart the session, check the skill list) |
| Edits to skills in an existing watched dir take effect live, no restart | Documented |
| Creating `.claude/skills` for the FIRST time requires a session restart | Documented — the script prints a warning when it creates the directory |
| Frontmatter: no required fields; `description` recommended; `description` + `when_to_use` combined ≤ 1,536 chars; the skill's command name comes from the DIRECTORY name, frontmatter `name` is a display label | Documented |

## Alternatives considered

- **`--add-dir <plugin>`** — documented to auto-load that dir's `.claude/skills`. Works per-session
  without any filesystem changes, but must be passed on every launch; junctions are set-and-forget.
- **Claude Code plugins / marketplaces** — heavier machinery; wrong shape for skills that must
  version in lockstep with plugin code.
- **Copying skills into the superproject** — rejected: copies rot; junctions always serve the
  checked-out submodule revision.

## Validation record (2026-07-02)

Tested against a fake superproject (scratchpad): junction creation, `SKILL.md` read-through,
idempotent second run (0 changes), duplicate-skill-name collision skip, real-directory clobber
protection, `-Prune` of a dead-target junction, and source-directory integrity after junction
deletion. Not yet exercised: live Claude Code session loading a skill through a junction (one-time
user check, see table above).
