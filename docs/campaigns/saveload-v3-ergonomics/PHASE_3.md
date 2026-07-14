# PHASE 3 — Produce symmetry, class (a): wire builders consume the registered Produce (build 3)

Goal: for the 7 trivially-symmetric features, the hand-built wire payload is replaced by the feature's own
registered `Produce` — one projection function per feature, consumed by both the wire and the save file.
Wire/save divergence for these features becomes inexpressible.

**Behavioral bar: wire CONTENT must be byte-identical before/after** (the projections were verified
identical on 2026-07-14 — this phase deletes one of the two copies). The Net suite is the gate.

## Entry criteria

Phase 2 committed; trees clean; gates at the PROGRESS-recorded baseline (counts AND names). Registry is
now `FCk_PersistenceHandlerRegistry`; slots are `NetApply`/`NetRemove`/`HydrationApply`/`Produce`.

## Step 1 — The `TryProduce<T>` helper (CkEcs)

Add to `UCk_Utils_Net_UE` in `CkNet_Utils.h`, beside `TryUpdateContainerFragment` (~L329), body in the same
file's template-definition region (~L484):

```cpp
    // Resolve the feature's registered Produce and return the typed payload. UNSET when the feature is
    // absent on this entity (Produce's own contract). Ensures loudly if no Produce is registered for
    // TDataStruct — calling this for a type without save/wire projection is a programmer error.
    template <typename TDataStruct>
    static auto
    TryProduce(
        FCk_Handle& InHandle)
        -> TOptional<TDataStruct>;
```

Body:
```cpp
    const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(TDataStruct::StaticStruct());
    CK_ENSURE_IF_NOT(Handler != nullptr && static_cast<bool>(Handler->Produce),
        TEXT("No registered Produce for [{}]"), TDataStruct::StaticStruct()->GetName())
    { return {}; }

    const auto Produced = Handler->Produce(InHandle);
    if (NOT Produced.IsSet())
    { return {}; }

    return Produced->template Get<TDataStruct>();
```

## Step 2 — Convert the 7 features (exact sites; one commit per repo-area is overkill — one commit total)

For each: replace ONLY the payload construction; keep the site's container-write mechanics, dirty-marking,
and `Remove<MarkedDirtyBy>` behavior untouched.

1. **Velocity** — `CkVelocity_Processor.cpp:320-331`: the value-overload call becomes
   `if (const auto Produced = UCk_Utils_Net_UE::TryProduce<FCk_RepData_Velocity>(InHandle))`
   `{ UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Velocity>(InHandle, *Produced); }`.
2. **Acceleration** — `CkAcceleration_Processor.cpp:253-264`: same shape, `FCk_RepData_Acceleration`.
3. **TagSet** — `CkTagSet_Processor.cpp:116-131`: replace the mutator-overload lambda with the value
   overload fed by `TryProduce<FCk_RepData_TagSet>(InHandle)`. (The value overload's `SetFragmentData`
   find-or-adds the entry — same net effect as the mutator here; the entry always exists post-Add anyway.)
   Delete the now-redundant "payload shape matches" comment in `CkTagSet_Fragment.cpp:40-41`.
4. **MontagePlayer** — `CkMontagePlayer_Processor.cpp:419-433`: KEEP the driver resolution via
   `FFragment_ContainerRef_MontagePlayer` (do NOT switch to `TryUpdateContainerFragment` — different driver
   lookup path); only the argument changes:
   `Driver->SetFragmentData<FCk_RepData_MontagePlayer>(*Produced)` guarded on `TryProduce` returning set.
5. **Grid Occupancy** — `Ck2dGridOccupancy_Processor.cpp:87-116`: replace the record-walk payload build
   with `TryProduce<FCk_RepData_2dGridPlacements>(InHandle)` (the registered Produce at
   `Ck2dGridOccupancy_Fragment.cpp:71-92` does the identical walk); write via the existing container call.
6. **Team** — `CkTeam_Utils.cpp:55` (Add-time seed) and `:102-103` (`Request_ChangeTeam`): both become
   `TryProduce<FCk_RepData_Team>` + value write. ORDER CHECK (do this before editing): the fragment write
   (`_TeamID` set) must precede the container update at both sites — read the surrounding function; if the
   container write happens BEFORE the fragment carries the new value, Produce would emit the STALE value →
   STOP + Blocker (planner believes order is write-then-replicate; verify).
7. **Player** — `CkPlayer_Utils.cpp:51,107-108`: same as Team, `FCk_RepData_Player`, same order check.

## Step 3 — Build + gate (the phase's ONLY build)

1. Build → exit 0.
2. `--test --test-pattern "Ck.Net"` → **delta-zero vs the recorded baseline**. This is the load-bearing
   gate: every converted feature has Net specs. Any red among Velocity/Acceleration/TagSet/MontagePlayer/
   Grid/Team/Player Net tests → the projection or the write-order differs; STOP, revert the single
   feature's hunk, record which.
3. `--test --test-pattern "Ck.Snapshot"` → delta-zero vs the recorded baseline.
4. Exit greps — the deleted duplication is gone:
   `rg --no-ignore -n "FCk_RepData_Velocity\{|FCk_RepData_Acceleration\{|FCk_RepData_Team\{|FCk_RepData_Player\{|FCk_RepData_MontagePlayer\{" Source --glob '!**/*_Fragment.cpp'`
   → 0 (the only remaining constructors of these payloads live in the registrars' Produce lambdas).

## Commit

`refactor(persistence): class-(a) wire builders consume the registered Produce (Velocity/Acceleration/TagSet/MontagePlayer/GridOccupancy/Team/Player) — one projection per feature`

## Fences

- **Do NOT touch StateMachine or RenderTarget** (class (c) — deliberately asymmetric; kill reasons in
  PROMPT.md). Do not "notice" their divergence and fix it.
- **Do NOT touch Attributes / EntityCollection / Inventory** — Phase 4 (they need the fold, not this shape).
- Do NOT change any `NetApply`/`HydrationApply`/`Produce` body in this phase — consumers only.
- Do NOT convert the initial-value seed sites into something conditional — replicated containers must be
  seeded with REAL data at construction (recorded incident class: clients drop the initial value
  otherwise). `TryProduce` at the seed site reads the just-composed fragment — equivalent, keep the seed.
- The Produce contract is READ-ONLY on the entity; if any converted site relied on mutating inside the old
  build lambda (none found 2026-07-14) → STOP + Blocker.
