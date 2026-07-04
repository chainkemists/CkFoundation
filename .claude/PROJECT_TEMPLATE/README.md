# PROJECT_TEMPLATE — the per-project residue home

`CLAUDE.md.template` is the fill-in-the-blanks project CLAUDE.md for a new game built ON
CkFoundation. Everything generic lives in the **ck-game-\*** skill set and the framework
CLAUDE.md chain; the template holds ONLY what is genuinely per-project. (Authored 2026-07-03.)

## Why it is deliberately short

Long project docs rot into active misinformation. Both real consumers prove it:

- **BusterBlock's root `CLAUDE.md`** (verified stale 2026-07-03) still describes a
  `BbGameMode`/`BbGameState`/`BbPlayerController`/`BbGameInstance`/`BbGameEngine` C++
  family that does not exist in `Source/` (the default GameMode is a Blueprint; the store
  and AI C++ it describes was rewritten in AngelScript), and points tests at `Script/Tests/`
  when they moved to `Plugins/BusterBlockTests/Script/`.
- **Venus's `Script/Claude.md`** still teaches `ck::SelfEntity(this)` — an API removed from
  the framework (DECISIONS §3: replaced by `ck::ToEntity`).

Both files restated doctrine and enumerated structure; neither had a staleness convention.
The template resists this three ways: it links doctrine instead of restating it, every
volatile section carries a `(verified <date>)` stamp, and the Provenance section names the
re-verify commands and the rule — **at every milestone, re-stamp or tombstone**. Keep it
short so it stays true.

## How to instantiate

1. Copy `CLAUDE.md.template` to `<your-game-repo>/CLAUDE.md` (repo root — Claude Code only
   auto-loads the session project root's CLAUDE.md).
2. Fill every `<angle-bracket>` slot; delete every guidance comment. If a section has no
   content yet (e.g. no divergences), keep the heading with its empty table — an empty
   table is information.
3. Run `Plugins/CkFoundation/.claude/scripts/sync-skills.ps1` from the repo root to
   junction the ck-game-\* (and framework) skills into `.claude/skills/`. Re-run it after
   every framework-submodule update.
4. **Restart the Claude session once** if this was the first-ever creation of
   `.claude/skills/` — junctions made mid-session are not discovered.
5. Commit the filled CLAUDE.md (and `.claude/skills/` junctions per your repo's policy).

## The local-skill policy

A consuming project may create a local `.claude/skills/<name>` for exactly two things:

- **(a) Third-party SDK integration** — Steamworks/EOS/Sentry/console SDK specifics: keys,
  entitlement quirks, upload tooling. Inherently non-generalizable.
- **(b) Project packaging/distribution specifics** — this game's store pipeline, cert
  checklist, build-farm wiring, deploy scripts.

**Everything else must be generalizable.** If a gameplay/testing/debugging procedure feels
like it needs a local skill, that is a **framework gap**, not a local skill to write:
report it via the gap-report ritual in **ck-game-framework-boundary** so it lands in the
shared library (or the framework itself) and every game benefits. Local copies of
generalizable knowledge fork the doctrine and rot on their own schedule — the exact
failure mode documented above.

### The 4-question self-test (all four must pass to justify a local skill)

1. **Would a second game copy this skill verbatim?** If yes → it's generic → framework gap,
   not a local skill.
2. **Does it exist only because of a third-party contract or a distribution channel this
   project signed up for?** If no → framework gap.
3. **Would it be wrong, not just irrelevant, in another CkFoundation game?** If it would
   merely be irrelevant, it's probably generic-with-parameters → framework gap.
4. **Is it a procedure (a skill), not a fact?** Facts (map names, pins, gates, glossary)
   belong in the project CLAUDE.md, not in any skill.

## Routing table — where does this content go?

| Content | Home |
|---|---|
| Project facts: name, prefix, engine pin, submodule pins, platforms, SDK list | project CLAUDE.md (Identity) |
| Design ambiguities / open questions | the living tracker the CLAUDE.md names |
| Extra "done" requirements (CI gates, cert, per-platform checks) | project CLAUDE.md (Gates) |
| Game-domain vocabulary | project CLAUDE.md (Glossary) |
| Map/GameMode entry points | project CLAUDE.md (Maps) |
| A sanctioned deviation from the standard | project CLAUDE.md (Divergences, dated + justified) |
| SDK integration procedure (Steam upload, EOS auth quirks, Sentry symbols) | **local skill** (policy a) |
| This game's packaging/deploy/cert runbook | **local skill** (policy b) |
| How to build a feature, compose entities, replicate, test, debug, cook | **ck-game-\* skill** — never local |
| A generalizable procedure no ck-game skill covers | **gap report** → ck-game-framework-boundary ritual |
| Framework behavior questions (why the ECS works this way, macros, AS interop) | framework CLAUDE.md chain + framework ck-\* skills |
| Framework bug or missing capability | gap report / framework issue — never a local workaround skill |

## Provenance and maintenance

Authored 2026-07-03 against: BusterBlock @ `52a75e13d` and Venus @ CkFoundation pin
`3e27a245a` as the two-consumer corpus; staleness evidence verified by direct read of
`D:/Repos/BusterBlock/CLAUDE.md` vs `Source/` contents, and Venus `Script/Claude.md` vs
DECISIONS §3. Re-verify the ck-game-\* skill list in the template's "The standard" section
against `Plugins/CkFoundation/.claude/skills/` (`Get-ChildItem` — the Grep/Glob tools can
be blind here; use `rg --no-ignore --files` on zero-match) whenever skills are added or
renamed, and update the template's list in the same commit.
