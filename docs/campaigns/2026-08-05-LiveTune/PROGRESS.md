# PROGRESS — LiveTune campaign

**Living doc.** Updated before ending any session. Design of record:
[../../specs/2026-08-05-LiveTune-design.md](../../specs/2026-08-05-LiveTune-design.md) (GREEN-LIT);
CTO review: [../../reviews/2026-08-05-LiveTune-CTO-review.md](../../reviews/2026-08-05-LiveTune-CTO-review.md).

**Host project for this campaign: CkPlugins_Other** (`D:\Repositories\CkRepos\CkPlugins_Other`) — the
isolated plugin-dev host. Code lands in its `Plugins/CkFoundation` / `Plugins/CkTests` checkouts;
builds + tests run through its toolbox against `CkPlugins.uproject`. (Adam's call, 2026-08-05 —
the campaign docs speak of "BusterBlock repo" because the design session ran there; same submodule
repos, different superproject checkout.)

**Branch discipline (Adam's call, 2026-08-05):** all campaign work is committed progressively on a
local branch `livetune/phase-0` in EACH touched submodule (CkFoundation off `dev` @ `870ad0172`,
CkTests off `dev` @ `0ea0d6a2`). Never pushed; merge/rebase onto `dev` is Adam's post-audit step.

## Phase status

| Phase | Status |
|---|---|
| 0 — Spine | IN PROGRESS (this session, 2026-08-05) |
| 1 — Pilots | not started (blocked on Phase 0 gate + audit) |
| 2 — AS path | not started |
| 3 — Rollout | not started |

## Baseline (recorded BEFORE first edit)

- 2026-08-05: single-shot toolbox `--build --test` (full suite, fresh boot) on CkPlugins_Other,
  editor confirmed closed (log-lock probe: free). Results pending — recorded here verbatim
  (totals + failing names) before any source lands.
- Repo ground at start: `Plugins/CkFoundation` on `dev` @ `870ad0172`, tree clean.
  `Plugins/CkTests` on `dev` @ `0ea0d6a2`, tree clean — one commit behind the BusterBlock
  checkout's `403d887` (inventory-test commit; no overlap with this campaign; deliberately NOT
  pulled — sibling sessions own the pointer sync). Superproject dirty paths NOT ours, left for
  their owning session: `CkAuto`, `CkPlugins.uproject`, `Config/DefaultGameplayTags.ini`,
  submodule pointers.
- An earlier baseline run was mistakenly launched on BusterBlock and stopped mid-build before
  its test phase; no artifacts of it are used.

## Fork-source verifications (Phase 0 gate item: undo/redo event shape)

- **Undo/redo → `OnObjectPropertyChanged` arrives with NULL property, ChangeType `Unspecified`.**
  Chain (read on the 5.7 fork, `D:\Repositories\UnrealEngine-Angelscript`, this session):
  `FTransaction::Apply` → `Editor/UnrealEd/Private/EditorTransaction.cpp:953,957` `PostEditUndo()` →
  `CoreUObject/Private/UObject/Obj.cpp:847-852` `PostEditChange()` → `Obj.cpp:513-514` builds
  `FPropertyChangedEvent(NULL)` → `Obj.cpp:520` broadcast. Default ChangeType is `Unspecified`
  (`UnrealType.h:6874`). **Consequence encoded in the subsystem:** an event with no MemberProperty
  is treated as "any linked member of this asset may have changed" — every stamped member of that
  asset is re-diffed; the value-diff gate suppresses the non-changes. Undo/redo therefore
  dispatches the undone member only, at final-commit (all-tiers) policy.
- **Interactive vs ValueSet** re-confirmed at `PropertyHandleImpl.cpp:370-372` (matches CTO review).
- `FInstancedStruct::operator==` compares type + `Identical(PPF_None)` (`InstancedStruct.h:235-237`)
  — used as the diff-gate comparison.

## Decisions made this session (executor-level; design not relitigated)

1. **`Link` is a plain static C++ function in Phase 0, not a UFUNCTION.** The design's "empty
   inline outside the editor" is only literally achievable for a non-UFUNCTION (UHT thunks are
   never inline, and a UFUNCTION must exist in cooked builds for BP/AS callers). The tri-env
   surface (`utils_live_tune`, BP node) is Phase 2's deliverable per design §9; AS AutoTests reach
   Link through a CkTests shim meanwhile. Revert hook: wrap Link as UFUNCTION in Phase 2.
2. **Stamp fragment holds an ARRAY of (asset, member) entries**, not a single pair — two features
   added directly on the same entity (no child) can each Link; single-slot would silently drop
   one. Cleanup removes all entries for the dying entity. Named `ck::FFragment_LiveTune_Stamp`
   (design's `FFragment_DevTuningSource` renamed to the house `FFragment_[Feature]_[Type]` shape;
   spec header says all names are proposals).
3. **Registry + subsystem compile in all builds; functionality is `WITH_EDITOR`-gated internally**
   (mirrors `ACk_EditorSelectionProxyHost_Actor_UE` / in-module precedent — a fully `#if`-gated
   UCLASS in a Runtime module is not house practice). Non-editor: `Link` is an empty inline,
   `Register_*` are empty inlines, the subsystem never instantiates (`ShouldCreateSubsystem`
   false). The stamp fragment itself IS fully `#if WITH_EDITOR` (precedent
   `FFragment_EditorSelectionOwner`).
4. **ViaRebuild dispatch is NOT reachable in Phase 0.** `Register_ViaRebuild` stores config
   (Scope/ReAdd/Produce/Hydrate slots, compile-enforced ReAdd) but the subsystem's dispatch fires
   `CK_TRIGGER_ENSURE` naming the type if a rebuild-tier handler is ever hit — the rebuild driver
   is the Phase 1 pilot deliverable (its gate tests — round-trip, record-disconnect sequencing —
   pin it). No production registrations exist until Phase 1, so the path is unreachable outside
   tests, and no Phase 0 test dispatches it.
5. **Link-time deep validation is tier-aware.** "Struct type matches the handle's feature params
   type" is enforced via the handler's synthesized `Has<T>` check — ViaReplace tier only
   (ViaRequest/ViaRebuild features may keep no params fragment; FloatAttribute keeps none).
   Unregistered types stamp fine (fork call 2: dispatch logs Display "no handler registered").
6. **Diff cache is seeded at Link time** with the asset's current value — the first full-heal
   after linking with no actual change is suppressed, not just later ones.
7. **Authority gate reads the ENTITY, not the handler registration:** skip when
   `Get_IsEntityNetMode_Client(E)` && `Get_EntityReplication(E) == Replicates` (both on
   `UCk_Utils_Net_UE`). No per-handler replication flag needed.
8. **Stamp cleanup rides `on_destroy<FFragment_LiveTune_Stamp>`** on the EcsWorld registry's EnTT
   sink (the CkDebugFeatureFlags listener pattern), connected in the subsystem's Initialize with
   `InitializeDependency<UCk_EcsWorld_Subsystem_UE>` guaranteeing registry lifetime brackets the
   connection. Dispatch additionally validity-checks and iterates a copy (risk §10.5 "impossible,
   not merely rare").
9. **Test seams are plain C++ on the subsystem** (`Test_SimulatePropertyChange`,
   `Test_Get_LinkCount`), wrapped as UFUNCTIONs by a CkTests BPFL shim for AS — keeps the
   framework's reflected surface free of test-only API (CTO suggestion 4 asked for the seam, not
   its reflection). The simulate seam BROADCASTS a hand-built `FPropertyChangedEvent` through the
   real `FCoreUObjectDelegates` path, so AutoTests cover subscription → extraction → gates →
   dispatch end to end.

## File list (Phase 0)

Drafted in session scratchpad; landing into the CkPlugins_Other checkouts after the baseline run
completes (no source edits while a toolbox run is in flight):

- CkFoundation `Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_Fragment.h` — stamp fragment
- CkFoundation `Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h/.inl.h/.cpp` — registry
- CkFoundation `Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_Utils.h/.cpp` — `Link`
- CkFoundation `Source/CkEcs/Public/CkEcs/Subsystem/CkLiveTune_Subsystem.h/.cpp` — change listener
  (beside the existing editor subsystem, per fork call 3's layout)
- CkTests `Source/CkTests/Public/CkLiveTune_AutoTest_Utils.h` + `Private/CkLiveTune_AutoTest_Utils.cpp`
  — AS-facing shim: test params structs (Replace/Request + Spec*), tuning asset, Link/simulate
  wrappers, invocation counters, static-init handler registrations
- CkTests `Source/CkTests/Private/UnitTests/CkEcs/Test_LiveTune_Registry.cpp` — C++ specs
  (`Ck.LiveTune.Registry.*`, `Ck.LiveTune.LinkValidation.*`)
- CkTests `Script/CkEcs/CkAutoTest_LiveTune_DispatchByType.as` / `_DiffGate.as` /
  `_InteractivePolicy.as` / `_StampCleanup.as` — PIE autotests

## Gate status (Phase 0)

| Gate item | Status |
|---|---|
| Link validation rejects wrong-type/missing property loudly, zero partial state | tests drafted, pending run |
| Registry dispatch by type | tests drafted (spec + AS full-pipeline), pending run |
| Stamp cleanup on entity destroy | test drafted, pending run |
| Diff gate suppresses no-op + full-heal dispatch | test drafted, pending run |
| Undo/redo event shape verified on fork | **DONE** (see verifications; handled in dispatch) |

## Known limitations / notes for the auditor

- `Ck.LiveTune.LinkValidation.MissingProperty` pins loudness with expected-error occurrences=0
  (require ≥1). If the ensure display policy registers per-site ignores (LogOnly/MessageLog), a
  warm-server RE-run of the same process could see the site suppressed — the fresh-boot gate of
  record is unaffected. The other validation tests use tolerant `-1` + structural asserts.
- Test counters are process-global statics; every AS test asserts DELTAS against its own
  fresh asset instance, so parallel lanes (separate processes) and sequential same-world runs
  are both safe.

## [EDITOR-VERIFY] items (for Adam)

- (accumulating; none yet — the real details-panel mid-PIE edit lands with Phase 1 pilots)
