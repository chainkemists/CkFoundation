# VALIDATION — saveload-v3-ergonomics acceptance protocol

Run after the LAST executed phase (the campaign is valid stopping after any phase — validate whatever
shipped). Route the definition of done through `ck-change-control`: Phases 2–4 touch CkEcs core →
**Class-4: Adam review mandatory before any push. Nothing is pushed by the executor.**

## 1. Tree + commit state

- `git -C Plugins/CkFoundation status --short` → empty; `git -C Plugins/CkTests status --short` → empty.
- `git log --oneline` shows exactly the commits named in the executed phase docs, no others.
- CkTests commits reference framework symbols that exist in the paired CkFoundation commits (cross-repo
  rule: CkTests must never be mergeable ahead).

## 2. Full headless gates (fresh build, editor closed)

```powershell
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --build --target Editor --config Development --output CkAuto\logs\erg-final-build.log
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --discover-fresh --test-pattern "Ck.Snapshot" --output CkAuto\logs\erg-final-snapshot.log
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Net" --output CkAuto\logs\erg-final-net.log
```

Expected: build exit 0; both suites **delta-zero against the Phase-1 recorded baseline, BY NAME** (the
baseline counts live in PROGRESS.md — do not assume any literal number; the parity campaign's Phase 5
added tests beyond the older 30/90 figures). Read verdicts from the log files. Any delta → the campaign is NOT done;
identify the owning phase commit and resolve or revert before presenting to Adam.

## 3. Grep checklist (per executed phase)

| After phase | Grep | Expected |
|---|---|---|
| 1 | `rg --no-ignore -n "ck::StaticCast" Source/CkTimer Source/CkAttribute Source/CkAnimation` | 0 handler hits |
| 1 | `rg --no-ignore -c "EnqueueRoundTrip" Plugins/CkTests/Source` | ≥ 2 |
| 2 | `rg --no-ignore -n "FCk_ReplicatedFragmentHandlerRegistry\|ECk_RepFragment_ApplyResult\|FGroup_Hydration\b" Source/` | 0 |
| 2 | `rg --no-ignore -n "\.Apply = \|\.Remove = " Source/` | 0 |
| 2 | `rg --no-ignore -n "RegisterLazyTyped<" Source/ --glob '!**/CkReplicatedFragmentContainer*'` | 0 |
| 2 | parity PHASE_6 §6B enumeration grep (`FSnapshotPolicy_\|TSnapshotMarker\|WITH_POLICY`) | 0 |
| 3 | the Phase-3 payload-constructor grep | 0 outside registrars |
| 4 | `rg --no-ignore -n "Data.Attributes.Emplace\(ToReplicate\)" Source/CkAttribute` | 0 |
| 5 | `rg --no-ignore -n "ReplicatedFragmentContainer.inl" Source/ Plugins/CkTests/Source` | 0 |
| 5 | `rg --no-ignore -n "CkEcs/Net/" ` on the six save-only registrar files | 0 |

## 4. Three-environment check

- **C++** — the gates above.
- **Blueprint** — `[EDITOR-VERIFY]` (human, optional-but-recommended): open any BP that constructs
  `FCk_Request_Timer_Jump`; confirm the new `Jump Mode` pin appears, defaults to `Relative`, and existing
  BPs compile without dirty diffs. The registry/entry-point changes have NO BP surface (pure C++ —
  verified 2026-07-14).
- **AngelScript** — no regen expected: zero `Script/` references to any renamed symbol (verified; Phase 2
  re-verifies per parity PHASE_6 rule 1). `[EDITOR-VERIFY]` (human, optional): in an AS scratch, construct
  `FCk_Request_Timer_Jump` and call `Set_JumpMode(ECk_RelativeAbsolute::Absolute)` — compiles via
  reflection, no wrapper regen needed.

## 5. Documentation debt (executor updates in the final session)

- `Source/CkSnapshot/Claude.md` — registrar section: named `Register_*` forms replace the slot-literal
  example; slot names `NetApply`/`NetRemove`; note the wire now consumes `Produce` for converted features.
- `Source/CLAUDE.md` — the `RegisterLazy` cross-module pattern example gets the same update.
- Root `CLAUDE.md` macro table — no changes expected (verify `CK_DEFINE_*_WITH_POLICY` rows if 6B ran;
  parity PHASE_6 owns that edit list).
- Update BOTH campaign PROGRESS.md files (this one + the parity absorption notes).

## 6. Handoff summary for Adam (the executor writes this in PROGRESS.md)

- Phases executed + commit hashes per repo.
- Gate results vs baseline, by name.
- Deviations + Blockers (verbatim).
- The one claim most likely wrong (executor's honest pick).
- Explicit: NOTHING PUSHED; Class-4 review pending; the parity campaign's PHASE_7/VALIDATION still belongs
  to the parity executor.
