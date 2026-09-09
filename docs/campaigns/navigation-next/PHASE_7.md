# PHASE 7 — Persistence, cook, and streaming

> **DEFERRABLE PHASE.** This phase may be **deferred by the CTO** — it is a Tier-2 (retirement)
> phase, not a Tier-1 (promotion) one, so promotion can be reached without it. **Its entry criteria
> are deliberately independent of PHASE_8**: nothing here waits on parity measurement, on promotion,
> or on retirement, and PHASE_8's promotion gate (Tier B) does not consume anything this phase
> produces. If the CTO defers P7, PHASE_8's **Tier B** work proceeds; **Tier C (retirement) does
> not**, because C-tier retirement removes Recast's cook-time bake and this phase is what replaces
> it. That dependency is stated in PHASE_8 §2 and is not re-litigated here.
>
> **Freshness:** authored 2026-08-31 during the P4 planning package (documentation-only session,
> [NN-D3]). **Status of record: [PROGRESS.md](PROGRESS.md).**
> **Authoritative upstream:** [PROMPT.md](PROMPT.md), [REPRESENTATION.md](REPRESENTATION.md)
> [NN-D7], [MIGRATION_SEAM.md](MIGRATION_SEAM.md) [NN-D8], [FEATURE_MATRIX.md](FEATURE_MATRIX.md)
> (Tier-2 rows F2.14–F2.19), [VALIDATION.md](VALIDATION.md) (gate **A9**).
> Line references are a snapshot @ CkFoundation `a25ec9539` — re-verify before editing.

---

## 1. Goal and feature ids

Recast supplies persistence implicitly today — nav data is baked level content, and CkNavigation
persists nothing (R3 §C.11). Removing Recast therefore means **we** must supply what it was giving
us for free. This phase does that, and nothing else.

| Sub-phase | Features | Goal |
|---|---|---|
| **7A** | **F2.14** | **Serialize the ground field as values** with stable integer identity, at three granularities (whole field / spatial subset / per tile), version-gated. |
| **7B** | **F2.15** | **Cooked bake path** — fields bake offline and ship in the package; **runtime bake remains the fallback and the test path**, never deleted. |
| **7C** | **F2.16** | **Streaming chunk lifecycle** — merge-on-load, remove-on-unload, disable/re-enable without a rebuild. |
| **7D** | **F2.17** | **Build invokers** — inner/outer-radius hysteresis driving build scope. |
| **7E** | **F2.18** | **World Partition / data-layer awareness** in geometry collection and in cooked chunk assets. |
| **7F** | **F2.19** | **Field merge** at an offset + 90°-multiple rotation with a total tag remap. |

Ordering is load-bearing: **7A before everything** (cook and streaming both serialize); 7B before 7C
(a streamed chunk is a serialized chunk); 7C before 7D (invokers drive the lifecycle 7C implements);
7E and 7F after 7C (both are variants of collection and merge).

VALIDATION gate served: **A9 — Persistence and cook (P7)**, plus VALIDATION §4.4 `[EDITOR-VERIFY]`.

---

## 2. Entry criteria — confirm all before writing a line

**None of these reference PHASE_8.** This phase is entered from P6's close (or from an explicit CTO
resumption after a deferral), never from a parity result.

- [ ] **P6 closed** in PROGRESS.md with A8 evidence recorded, **or** — if P7 was deferred and is now
      resumed — the resumption is recorded as a numbered decision and the repo state at resumption is
      re-validated from scratch.
- [ ] **F1.8 immutable publish + epochs**, **F1.7 portal extraction (including cross-tile portals)**,
      **F1.11 content-hash dirty check**, **F1.34 path invalidation**, and **F1.43 hermetic math-core
      boundary** are landed. This phase serializes and schedules them; it builds none of them.
- [ ] **Session-start ritual** run verbatim (PROMPT.md §7) with two "Done"-claim spot-checks.
- [ ] **Phase-entry baseline captured this session** — full toolbox `--build --test`, `--parallel 1`,
      `--no-nullrhi`; totals **and failing-test names** and the artifact identity recorded, with the
      inherited `Nav.Filter.Customer` mapping failure recorded **by name**.
- [ ] **`[MEASURE]` budgets for A9 filled at entry**, measured and recorded in PROGRESS.md:
      (a) serialized field size per tile on the reference scene, (b) load time per tile from a cooked
      blob, (c) the current cold-start cost of a **runtime** bake of the same scene (this is the
      number the cook has to beat), (d) streaming merge time at a seam.
- [ ] **A build-machine cook slot is arranged** for 7B/7E's packaged verification. Local cooking is
      forbidden (§8) — if no slot is available, plan 7A/7C/7D/7F and defer the packaged legs rather
      than cooking locally.

**Verify commands:**

```powershell
Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test --parallel 1 --no-nullrhi `
    --output=Saved/Logs/P7-Baseline.log --project="D:\Repos\CkPlugins_3"

# The persistence contract this phase must NOT violate — the process-relative pending clock
rg --no-ignore -n '_PendingSinceSeconds' `
    Plugins/CkFoundation/Source/CkNavigation/Public/CkNavigation/Nav/CkNav_Fragment_Data.h

# Identity discipline: no pointer/engine-object identity anywhere in the field value types
rg --no-ignore -n 'TObjectPtr|TWeakObjectPtr|TSoftObjectPtr|UObject\*|\*\s*_' `
    Plugins/CkFoundation/Source/CkGroundNav/Public/CkGroundNav/Field
```

---

## Standing decision gate (applies to EVERY `-> verify:` line in this file)

Read this once; it is the default branch set for every verification step below. Steps that need
extra branches carry their own **Gate** block.

- Observation matches the stated expectation → continue to the next step.
- Build fails to compile → fix mechanically (missing include, moved symbol, UHT complaint) and
  re-verify. **Two failed attempts at the same step → STOP.**
- A test that was red in the P7 phase-entry baseline is still red with the same name → not yours;
  continue, and carry it forward in the delta-zero comparison.
- A test that was green in the baseline is now red → **STOP**, restore the known-good state (revert
  your last step), then diagnose before re-applying.
- A test that was red in the baseline is now green → **STOP**. An unexplained improvement is as much
  a defect as a regression — explain it or record it (VALIDATION.md A1).
- **Any observation not enumerated above → STOP, record it in PROGRESS.md § Blockers with verbatim
  evidence, end session.**

---

## 3. Sub-phase 7A — field serialization as values (F2.14)

1. **Serialize the field's flat arrays and stable integer ids**, at three granularities: whole field,
   a **spatial subset** (boundary-crossing portals and links dropped cleanly, never dangling), and
   **per tile**.
   → *verify:* Layer 1 — round-trip a baked field and **byte-compare**; round-trip a spatial subset
   and assert boundary portals and links are **dropped**, not dangling; per-tile round-trip equals the
   corresponding slice of the whole-field round-trip.

2. **Version-gate the stream.** A mismatched version is a **clean rejection with a status** — never a
   partial load, never a best-effort read.
   → *verify:* Layer 1 — a version-mismatched blob is rejected with a status and mutates nothing
   (no partial field, no epoch bump, previously published field pointer unchanged).

3. **Exclude what must never persist.** The **process-relative pending clock** stays excluded (it is
   already contractually non-persistable and non-replicable: restored into another process it reads
   as ancient and trips the timeout instantly). Runtime **overlay** state — markup overlays, link
   enable/cost overlays — is **re-applied after load, never serialized**.
   → *verify:* Layer 1 — a reflection/grep assertion that no wall-time or process-relative value
   appears in the serialized set; a load followed by overlay re-application reproduces the pre-save
   query results, and a load **without** re-application reproduces the **baked** results (proving the
   overlay was not smuggled in).

4. **Stable integer identity survives the round trip**, with **no pointer identity anywhere**.
   → *verify:* Layer 1 — tile/layer/plate/portal/link ids compare equal across the round trip; the
   identity grep of A3 re-run against the serialized types.

### Gate 7A → 7B

- All four steps green → **proceed**.
- A field member cannot be expressed as a value without a pointer or an engine-object reference →
  **STOP**, record in PROGRESS.md § Blockers, end session. The representation forbids it ([NN-D7]).
- The byte-compare is unstable across runs → **STOP**: that is a determinism defect in the **bake**
  (P1), not a serialization tolerance to widen. Record and end session.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 4. Sub-phase 7B — cooked bake with runtime bake as fallback and test path (F2.15)

The rule that shapes this sub-phase: **the cook is an optimization, not the only path.** The runtime
bake remains, because it is (a) the fallback when a cook is missing or stale and (b) the path every
Layer-1 and Layer-2 test exercises.

1. **Cook-time build entry point** over the **same math core** (F1.43) — one implementation, two
   drivers. Writes serialized tiles (7A) into cooked assets.
   → *verify:* Layer 1 — a cooked field loaded at runtime is **byte-identical** to a runtime bake of
   the same world (A9's stated criterion).

2. **Needs-resave signal** by content-hash comparison (F1.11): source geometry changed after the last
   bake ⇒ the asset reports stale.
   → *verify:* Layer 1 — perturbing any enumerated hash input flips the signal; perturbing nothing
   does not.

3. **A stale or missing cook is a status, not a crash.** The field reports its state
   (`MissingCook` / `StaleCook`) and the provider fails paths **cleanly**, exactly as an unbuilt
   region does — same fail reasons, same signals, so consumer retry machinery is provider-blind.
   → *verify:* Layer 1 — one test per state; a path request into a missing-cook region returns the
   unbuilt-class failure and **never** an empty-but-successful result.

4. **Runtime bake remains reachable** as the fallback and as the test path, selected by state, not by
   a build-configuration `#if`.
   → *verify:* Layer 1 — with the cook absent, the runtime bake produces the field and the status
   reports `RuntimeOnly`; the full Layer-1 bake family still runs headless with no cook present.

5. **`[EDITOR-VERIFY]` VALIDATION §4.4 step 5** — a packaged **Development** build loads cooked
   fields, paths match the editor bake on the same scene, and **no runtime bake occurred** (the
   status row / log says loaded-from-cook, not built-at-runtime).
   → *verify:* the recorded human result with verifier and artifact. **This runs on the build
   machine** (§8).

### Gate 7B → 7C

- Byte-identical cooked-vs-runtime field, all statuses correct, §4.4 step 5 recorded → **proceed**.
- Cooked and runtime fields differ → **STOP**, record the first differing tile and byte offset, end
  session. Do not add a tolerance; a divergence here means the two drivers are not running the same
  math core.
- A packaged verification cannot be scheduled on the build machine → **defer the packaged legs**,
  record the deferral, and proceed with 7C only if the deferred items are listed as open. Never cook
  locally to unblock yourself.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 5. Sub-phase 7C — streaming chunk lifecycle (F2.16)

Semantics are fixed by FEATURE_MATRIX and MIGRATION_SEAM; this sub-phase implements them and does
not choose them.

1. **Merge on load** = **portal re-derivation at the seam only** — the cross-tile portal extraction
   from F1.7 run for the newly adjacent boundary. It is bounded and already tested; it is **not** a
   partial rebuild and **never** a patch of a published field.
   → *verify:* Layer 1 — load A, load adjacent B, assert cross-tile portals **identical to a single
   bake of A∪B**.

2. **Remove on unload** — the chunk is removed cleanly and paths crossing it are invalidated through
   **F1.34**, never left dangling.
   → *verify:* Layer 1 — unload B and assert A's field is **exactly A-alone's field**; Layer 2 — an
   agent whose installed path crossed B fails with the correct reason (not a crash, not an ensure,
   not a silent stall).

3. **Disable / re-enable without a rebuild** for a streamed-out-but-not-unloaded chunk.
   → *verify:* Layer 1 — a disable/re-enable round-trips **exactly** (byte-compare against the field
   before the disable), with zero geometry probes consumed.

4. **Chunk identity follows the proven precedent** — stable integer ids (volume id + lattice index),
   portals plus an adjacency table, and **partitioning decided at composition, not inside a
   processor**.
   → *verify:* Layer 1 — identity is reproducible across load orders; a grep confirms no partitioning
   decision lives inside a processor body.

5. **Streaming during an active episode.**
   → *verify:* Layer 2 — streaming a level in and out during an active crowd episode produces path
   failures **with correct reasons**, not crashes or ensures.

### Gate 7C → 7D

- All five green → **proceed**.
- Merged-on-load portals differ from a single bake of the union → **STOP**, record the differing
  portal ids, end session. That is a seam-derivation defect, not a streaming tolerance.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 6. Sub-phase 7D — build invokers (F2.17)

1. **An ECS-native invoker**: a fragment on any entity (inner generation radius, outer removal
   radius), aggregated per world into a **required-tile set** each tick, differenced against the
   built set, feeding the build scheduler (F1.10) in a **deterministic order**. Invoker volumes as a
   second source. Illustrative house shape — *executor refines mechanically, does not redesign*:

   ```cpp
   namespace ck
   {
       struct CKGROUNDNAV_API FFragment_GroundNav_BuildInvoker
       {
           CK_GENERATED_BODY(FFragment_GroundNav_BuildInvoker);

       private:
           float _InnerGenerationRadiusUu = 0.0f;
           float _OuterRemovalRadiusUu     = 0.0f;

       public:
           CK_PROPERTY_GET(_InnerGenerationRadiusUu);
           CK_PROPERTY_GET(_OuterRemovalRadiusUu);
       };
   }
   ```
   *(Radii are plain `float` scalars in unreal units — the precedent is `CkNav_ProjectSettings`,
   which expresses projection extents as floats. The "never bare floats" doctrine governs authored
   gameplay **shape** assets, not config scalars. Any budget or interval is still `FCk_Time`.)*
   → *verify:* Layer 1 — the required set is a **pure function** of invoker positions + radii
   (same inputs, same set, any tick order).

2. **Hysteresis** — inner radius generates, outer radius purges, so an invoker oscillating across the
   boundary does not thrash.
   → *verify:* Layer 1 — a bounded build count over N oscillations across the boundary (assert an
   exact bound, not "few").

3. **Purge correctness.**
   → *verify:* Layer 1 — purge removes exactly the tiles outside the outer radius and no others.

### Gate 7D → 7E

- Green → **proceed**. A hysteresis band that cannot bound the oscillation count → **STOP**, record,
  end session. Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 7. Sub-phases 7E and 7F

### 7E — World Partition / data-layer awareness (F2.18)

1. **A data-layer selector on the build request** feeding F1.1's collection filter; geometry from
   excluded layers is **not collected**.
   → *verify:* Layer 1 — collection with a layer filter returns exactly the expected geometry subset.
2. **The layer selector participates in the content hash** (F1.11) — otherwise a layer change is
   silently skipped by the dirty check, which is the permissive failure F1.11 explicitly forbids.
   → *verify:* Layer 1 — perturbing the selector forces a rebuild.
3. **Cooked chunks are World-Partition-aware assets** carrying bounds, integer chunk coordinates, and
   their content hash.
   → *verify:* Layer 1 on the asset contents; `[EDITOR-VERIFY]` — a data-layer-varying level bakes
   distinct fields.

### 7F — Field merge (F2.19)

1. **A pure value transform over the serialized field** — positions offset and rotated, ids rebased,
   tags remapped through a table — followed by **seam portal re-derivation (F1.7)**.
   → *verify:* Layer 1 — merging a field at offset O **equals** baking the same geometry at offset O
   (byte-compare after seam re-derivation).
2. **Rotation is restricted to axis-aligned 90° multiples** in generation 1. Arbitrary rotation
   breaks the axis-aligned plate invariant and is a Tier-3 question, not an executor's.
   → *verify:* a non-90°-multiple rotation is rejected at admission with `CK_ENSURE_IF_NOT` and no
   partial state — never silently snapped.
3. **The tag remap is total** — an unmapped tag is a **rejection**, not a silent drop.
   → *verify:* Layer 1 — an unmapped tag rejects the merge and mutates nothing.

### Gate 7E/7F → phase exit

- Green → **phase exits**. An arbitrary-rotation requirement surfacing from content → **STOP**,
  record, end session (Tier-3 fork, CTO's call). Anything else → **STOP, record in PROGRESS.md
  § Blockers, end session.**

---

## 8. Fences (each with its reason)

- **Cook and packaging are build-machine-only. Do not cook or package locally, in this phase above
  all.** This is the phase most tempted to do it. A local cook races the running editor for
  `Saved/`/`Intermediate/`, can corrupt DDC and hot-reload state, and produces an artifact that no
  gate in VALIDATION.md is defined against — so its result is not evidence even when it is green.
  Request the cook from the build machine and record the artifact identity it produced.
- **Shipping-configuration builds require explicit maintainer approval**, requested and granted in
  chat. A9 and §4.4 need **Development** and **Test**; nothing here needs Shipping, and a Shipping
  build is never started to "check something".
- **Module tier.** Serialization, cook loading, streaming, and invokers are all **Runtime**-tier —
  they must work in a packaged build. Cook-time-only entry points live in an **UncookedOnly/Editor**
  module and are never referenced from runtime code. This inversion passes every PIE test and fails
  only when packaged.
- **No raw pointers, `TObjectPtr`, `TWeakObjectPtr`, or engine-object references inside the field, a
  serialized blob, or a debug snapshot** — stable integer ids only. A serialized pointer is
  meaningless on load, and a pointer inside a snapshot outlives its producer.
- **Nothing process-relative is persisted or replicated.** The pending clock is a
  `FPlatformTime::Seconds()` absolute: restored into another process it reads as ancient and trips
  the timeout instantly. The same rule binds any wall-time value this phase might be tempted to store.
- **Never hide a new failure behind the known inherited `Nav.Filter.Customer` baseline failure**
  (`CkCrowdDebugger/CLAUDE.md:29-30`). It is named in the entry baseline for exactly this reason.
- **Never patch a published field in place** — merge, unload, disable, and re-enable all derive a new
  field (or tile) and swap ([NN-D7] fence). Patching makes corruption representable.
- **The runtime bake is never deleted "because the cook works".** It is the fallback for a missing or
  stale cook and the path every hermetic test runs on.
- **Never edit source, `Script/`, or config while a build or test run is in flight.**
- **Commits, pushes, submodule pointer bumps, and PRs only when the maintainer asks.** Stage only
  paths you authored.

---

## 9. Exit criteria (measurable)

- [ ] **Targeted `Ck.GroundNav.Serialization.*` and the bake/portal families green** with
      `--parallel 1`.
- [ ] **Full suite delta-zero** vs the phase-entry baseline, on the **final** artifact of the phase.
- [ ] **Zero new ensures, zero new warnings**; fresh editor startup log inspected.
- [ ] **A9 evidence complete**: baked tiles serialize as values and round-trip byte-identically with
      stable integer ids and no pointer identity; a cooked build loads baked tiles and produces paths
      identical to the editor bake on the same scene; a stale or missing cook is a **status**, not a
      crash, and the provider fails paths cleanly as an unbuilt region does; nothing process-relative
      is persisted.
- [ ] **Streaming semantics evidenced**: merge-on-load == single bake of the union at the seam;
      unload leaves the remainder exactly as it was; disable/re-enable round-trips exactly; a
      streamed unload during a live episode yields correct failure reasons.
- [ ] **Invoker hysteresis bounded** by an asserted count over N oscillations.
- [ ] **`[EDITOR-VERIFY]` §4.4** recorded with verifier and artifact (Development **and** Test
      builds), produced on the **build machine**.
- [ ] **Three environments** for every public API added; **Provenance cells** non-empty for every
      feature shipped.
- [ ] **Comment audit** run; PROGRESS.md updated; PHASE_8 re-verified at the boundary.

---

## 10. [P7] Done means

**[P7] is done when**, and only when:

1. `VALIDATION.md` **A9** is fully checked with evidence recorded in PROGRESS.md — value-only
   serialization with byte-identical round-trip and surviving stable integer ids; a cooked build that
   loads baked tiles and paths identically to the editor bake; stale/missing cook as a status rather
   than a crash; nothing process-relative persisted.
2. `VALIDATION.md` **§4.4** is recorded for both packaged Development and packaged Test, with
   verifier and artifact, and confirms **loaded-from-cook, not built-at-runtime**.
3. Every **§0 standing gate** is green for this phase: entry baseline, phase-exit full-suite
   delta-zero on the final artifact, zero new ensures/warnings, clean editor boot with regenerated AS
   bindings, three environments, Provenance rows complete, attribution obligations discharged, no
   forbidden-source language.
4. If the CTO **deferred** this phase instead: the deferral is recorded as a numbered decision in
   PROGRESS.md, and PHASE_8's **Tier C** is explicitly blocked on this phase's later completion while
   **Tier B** proceeds. A deferral that is not written down is not a deferral — it is a gap that will
   be discovered at the retirement gate.
