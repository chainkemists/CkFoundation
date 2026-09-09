# PHASE 0 — Contract neutralization, provider-neutral facade, Recast adapter

> Freshness: authored 2026-08-31 in planning session 1, at the campaign's 0→1 boundary.
> Status of record is [PROGRESS.md](PROGRESS.md); the design of record is
> [MIGRATION_SEAM.md](MIGRATION_SEAM.md) (Steps 1–3) and [FEATURE_MATRIX.md](FEATURE_MATRIX.md)
> (F1.27, F1.38, F1.39, F1.42). Where this file and a design doc disagree, the design doc wins and
> the disagreement is a **STOP** (record it in PROGRESS.md § Blockers).
>
> **Every code block in this file is illustrative — the executor refines it mechanically, does not
> redesign it.** Names, parameter shapes, and member sets are pre-designed so an executor fills in
> bodies. A sketch that cannot be made to compile as written is a mechanical refinement (rename a
> type, split a header, add an include). A sketch whose *shape* seems wrong is a fork → STOP.

---

## 1. Goal

Move every Recast-typed navigation capability behind one provider-neutral surface **with no
behaviour change**, so that a second provider can later be added as a branch rather than as a
rewrite — landing **F1.27** (provider-neutral filters and area tags), **F1.38** (the
`UCk_Utils_NavSurface_UE` facade), **F1.39** (Recast adapter and provider selection), and **F1.42**
(the neutral test-fixture obstacle vocabulary).

Nothing in this phase makes navigation better. If a test result *changes* in either direction, that
is a defect (VALIDATION.md A1, last bullet).

---

## 2. Entry criteria

Confirm each one **now**, in this session, before editing anything. A criterion you could not
confirm is a STOP, not a footnote.

- [ ] **PROGRESS.md read top to bottom**, and two of its "Done" claims spot-checked per PROMPT.md §7.
- [ ] **Repo state clean and on a `feature/` branch** (root + the three submodules):
      ```powershell
      git -C "D:\Repos\CkPlugins_3" status --short --branch
      git -C "D:\Repos\CkPlugins_3\Plugins\CkFoundation" status --short --branch
      git -C "D:\Repos\CkPlugins_3\Plugins\CkTests" status --short --branch
      git -C "D:\Repos\CkPlugins_3\Plugins\CkGameplayDebugger" status --short --branch
      ```
      Any dirty path you did not author belongs to another session — leave it untouched and report it.
- [ ] **The two leaks are still where R3 says they are** (line numbers are a snapshot; the *symbols*
      are the contract):
      ```powershell
      rg --no-ignore -n "_ExcludedAreaClasses|_QueryFilterClassOverride" `
         "Plugins\CkFoundation\Source\CkNavigation\Public\CkNavigation\Nav\CkNav_Fragment_Data.h"
      ```
      Expect exactly two declarations. Zero hits ⇒ someone already started this ⇒ STOP.
- [ ] **The neutral install seam is still intact** (this phase must not touch it):
      ```powershell
      rg --no-ignore -n "InstallExternalPath|MarkPathPending|AbandonPath|FailPath" `
         "Plugins\CkFoundation\Source\CkNavigation\Public\CkNavigation\Nav\CkNav_Algorithm.h"
      ```
- [ ] **Phase-entry baseline captured** — full suite, on the artifact this session starts from,
      totals **and failing-test names** recorded in PROGRESS.md:
      ```powershell
      Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
        --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
      ```
      Pre-flight: the editor must be closed for this project (the toolbox exits **77** if it is
      open) — see the `build-test` skill's pre-flight table.
      (Command shape and all flag semantics: the `build-test` skill. Do not invent flags; do not
      invoke `Build.bat`/UBT/`UnrealEditor-Cmd` directly.)
- [ ] Skills loaded before the work they govern: `ck-macros-and-codegen`,
      `ckecs-architecture-contract`, `ck-tests-authoring-and-running`, `ck-angelscript-interop`,
      `build-test`, `ck-change-control`.

---

## 3. Standing decision gate (applies to EVERY `-> verify:` line in this file)

Read this once; it is the default branch set for every verification step below. Steps that need
extra branches carry their own **Gate** block.

- Observation matches the stated expectation → continue to the next step.
- Build fails to compile → fix mechanically (missing include, moved symbol, UHT complaint) and
  re-verify. **Two failed attempts at the same step → STOP.**
- A test that was red in the phase-entry baseline is still red with the same name → not yours;
  continue, and carry it forward in the delta-zero comparison.
- A test that was green in the baseline is now red → **STOP**, restore the known-good state (revert
  your last step), then diagnose before re-applying.
- A test that was red in the baseline is now green → **STOP**. In a no-behaviour-change phase an
  unexplained improvement is as much a defect as a regression (VALIDATION.md A1).
- **Anything else → STOP, record it in PROGRESS.md § Blockers with verbatim evidence, end session.**

---

## 4. Sub-phase 0A — Neutralize the two public leaks (F1.27)

**0A entry:** entry criteria §2 all green.
**0A exit:** `rg` over `CkNav_Fragment_Data.h` for Unreal-Navigation types returns zero; suite
delta-zero.

### Steps

1. Author the neutral filter-definition asset. Illustrative — executor refines mechanically, does
   not redesign:

   ```cpp
   UCLASS(BlueprintType)
   class CKNAVIGATION_API UCk_NavFilterDefinition_DataAsset : public UDataAsset
   {
       GENERATED_BODY()

   private:
       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FGameplayTagContainer _RequiredAreaTags;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FGameplayTagContainer _ExcludedAreaTags;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       TMap<FGameplayTag, float> _AreaCostMultipliers;

   public:
       CK_PROPERTY_GET(_RequiredAreaTags);
       CK_PROPERTY_GET(_ExcludedAreaTags);
       CK_PROPERTY_GET(_AreaCostMultipliers);
   };
   ```
   → verify: the asset class compiles and is visible in the content browser's asset-picker for the
   settings map (below). No behaviour is wired yet.

2. Rewrite the settings table from `TMap<FGameplayTag, TSoftClassPtr<UNavigationQueryFilter>>` to
   `TMap<FGameplayTag, TSoftObjectPtr<UCk_NavFilterDefinition_DataAsset>>` in
   `Settings/CkNav_ProjectSettings.h`.
   → verify: `rg --no-ignore -n "UNavigationQueryFilter" Plugins/CkFoundation/Source/CkNavigation/Public` → zero hits outside the adapter files created in 0B.

3. Replace the two leaked members. Illustrative:

   ```cpp
   // FCk_Nav_QueryFilterOverlay — value-only exclusion overlay, now provider-neutral
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
   TArray<FGameplayTag> _ExcludedAreaTags;

   // FCk_Request_Nav_FindPath — per-query filter override, now a tag through the settings table
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
   FGameplayTag _QueryFilterOverride;
   ```
   Delete the old members outright — **no back-compat shim, no deprecation** (CkFoundation
   doctrine; PROMPT.md §2).
   → verify:
   ```powershell
   rg --no-ignore -n "UNavArea|UNavigationQueryFilter|TSubclassOf<UNav" `
      "Plugins\CkFoundation\Source\CkNavigation\Public\CkNavigation\Nav\CkNav_Fragment_Data.h"
   ```
   → **zero hits** (VALIDATION.md A1, first bullet), and
   ```powershell
   rg --no-ignore -n "_ExcludedAreaClasses|_QueryFilterClassOverride" Plugins CkAuto
   ```
   → **zero hits repo-wide** (the removed-member sweep landed in the same change).

4. Author the two CkCrowd filter-definition assets replacing
   `UCk_NavQueryFilter_AvoidStandingCrowds` (strict) and the permissive default, and repoint
   `CkCrowdAgent_HandleRequests_Processor`'s strict/permissive selection at the two tags.
   → verify: the crowd strict/permissive re-dispatch autotests are green and unchanged in count.

   **Gate 0A-4.** After the crowd suite runs:
   - Strict and permissive tests green, same names, same count → continue.
   - A strict-phase test now takes the permissive branch (or vice versa) → the two definitions do
     not reproduce the old filter classes' semantics → **STOP**, record which test and which branch.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

5. Map the five existing framework area classes (`UCk_NavArea_Restricted`, `UCk_NavArea_CrowdAgent`,
   the three avoidance-volume areas) onto area **tags**, and keep the classes alive *only* inside the
   Recast adapter's tag→class table (0B step 3).
   → verify: `rg --no-ignore -n "UCk_NavArea_" Plugins/CkFoundation/Source` shows hits only in the
   adapter files and the area-class definitions themselves.

6. Gate 0A. Run the full suite on the final artifact of 0A.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
   ```
   → verify: delta-zero vs the §2 baseline (standing gate applies).

---

## 5. Sub-phase 0B — The provider-neutral facade + Recast adapter (F1.38, F1.39)

**0B entry:** 0A exit green, recorded in PROGRESS.md.
**0B exit:** all twelve capabilities reachable through `UCk_Utils_NavSurface_UE`; the Recast adapter
is the only implementation; suite delta-zero.

### Steps

1. Author the neutral status vocabulary. `Unbuilt`, `NoSurface`, and `Blocked` must stay
   distinguishable (FEATURE_MATRIX Part B preamble). Illustrative:

   ```cpp
   UENUM(BlueprintType)
   enum class ECk_NavSurface_QueryStatus : uint8
   {
       Success,
       NoSurface,    // nothing walkable qualified inside the search volume
       Unbuilt,      // the queried region is not built yet — NOT the same as NoSurface
       Blocked,      // walkable, but the filter or clearance rejected it
       NoProvider    // no navigation provider resolved for this world
   };
   CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_QueryStatus);
   ```
   → verify: compiles; the enum has a formatter (house rule: every UENUM does).

2. Author the query/result value types. One struct per query — the request struct **is** the
   extension point (Source/CLAUDE.md, "`Request_*` takes the request STRUCT"). Illustrative:

   ```cpp
   USTRUCT(BlueprintType)
   struct CKNAVIGATION_API FCk_NavSurface_ProjectionQuery
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_NavSurface_ProjectionQuery);

   private:
       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FVector _Location = FVector::ZeroVector;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FVector _SearchHalfExtents = FVector::ZeroVector;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       ECk_NavSurface_ProjectionMode _Mode = ECk_NavSurface_ProjectionMode::Closest;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FGameplayTag _QueryFilter;

   public:
       CK_PROPERTY_GET(_Location);
       CK_PROPERTY(_SearchHalfExtents);
       CK_PROPERTY(_Mode);
       CK_PROPERTY(_QueryFilter);

   public:
       CK_DEFINE_CONSTRUCTORS(FCk_NavSurface_ProjectionQuery, _Location);
   };

   USTRUCT(BlueprintType)
   struct CKNAVIGATION_API FCk_NavSurface_ProjectionResult
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_NavSurface_ProjectionResult);

   private:
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       ECk_NavSurface_QueryStatus _Status = ECk_NavSurface_QueryStatus::NoProvider;

       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FVector _Location = FVector::ZeroVector;

       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FVector _SurfaceNormal = FVector::UpVector;

       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FGameplayTagContainer _AreaTags;

   public:
       CK_PROPERTY_GET(_Status);
       CK_PROPERTY_GET(_Location);
       CK_PROPERTY_GET(_SurfaceNormal);
       CK_PROPERTY_GET(_AreaTags);
   };
   ```
   The remaining query pairs follow the same shape: `FCk_NavSurface_MoveAlongSurfaceQuery/Result`,
   `FCk_NavSurface_RaycastQuery/Result`, `FCk_NavSurface_BoundaryQuery/Result`,
   `FCk_NavSurface_ReachabilityQuery/Result`.
   → verify: compiles; every struct carries `CK_GENERATED_BODY`; every essential field is in the
   `CK_DEFINE_CONSTRUCTORS` list and nothing optional is.

2b. **Supporting types named above — illustrative member sketches.** These six are referenced by the
   query/result structs and the facade; sketched here so no executor invents a different shape.
   **All illustrative** — refine mechanically, do not redesign.

   ```cpp
   // Illustrative.
   UENUM(BlueprintType)
   enum class ECk_NavSurface_ProjectionMode : uint8
   {
       Down,       // project along -Z only
       Up,         // project along +Z only
       Closest     // nearest qualifying surface in the search volume
   };
   CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_ProjectionMode);

   // Illustrative.
   UENUM(BlueprintType)
   enum class ECk_NavSurface_Reachability : uint8
   {
       Reachable,
       Unreachable,
       Unknown_ProviderNotReady
   };
   CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_Reachability);

   // Illustrative.
   UENUM(BlueprintType)
   enum class ECk_NavSurface_ProviderHealth : uint8
   {
       Ready,
       Building,
       NoData,
       Error
   };
   CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NavSurface_ProviderHealth);

   // Illustrative — typesafe handle per the house pattern; declared in the *_Fragment_Data.h,
   // paired with CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE.
   USTRUCT(BlueprintType)
   struct CKNAVIGATION_API FCk_Handle_NavSurfaceMarkup : public FCk_Handle_Base
   {
       GENERATED_BODY()
       CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_NavSurfaceMarkup);
   };

   // Illustrative.
   USTRUCT(BlueprintType)
   struct CKNAVIGATION_API FCk_Request_NavSurface_AreaMarkup : public FCk_Request_Base
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_Request_NavSurface_AreaMarkup);

   private:
       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FCk_AnyShape _Shape;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FGameplayTag _AreaTag;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       ECk_EnableDisable _Enable = ECk_EnableDisable::Enable;

   public:
       CK_PROPERTY_GET(_Shape);
       CK_PROPERTY_GET(_AreaTag);
       CK_PROPERTY(_Enable);

   public:
       CK_DEFINE_CONSTRUCTORS(FCk_Request_NavSurface_AreaMarkup, _Shape, _AreaTag);
   };
   CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_NavSurface_AreaMarkup);

   // Illustrative.
   USTRUCT(BlueprintType)
   struct CKNAVIGATION_API FCk_NavSurface_BoundarySegment
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_NavSurface_BoundarySegment);

   private:
       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FVector _Start = FVector::ZeroVector;

       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FVector _End = FVector::ZeroVector;

       UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FVector _InwardNormal = FVector::ZeroVector;   // points into walkable space

   public:
       CK_PROPERTY_GET(_Start);
       CK_PROPERTY_GET(_End);
       CK_PROPERTY_GET(_InwardNormal);
   };
   ```
   → verify: compiles; every UENUM carries a formatter; the handle is declared in
   `*_Fragment_Data.h`; the request carries `CK_REQUEST_DEFINE_DEBUG_NAME`.

3. Author the facade. **Twelve capabilities, one utils class**, ECS-idiomatic — not a UObject
   interface ([NN-D8] Step 2). Illustrative:

   ```cpp
   UCLASS(NotBlueprintable)
   class CKNAVIGATION_API UCk_Utils_NavSurface_UE : public UCk_Utils_Ecs_Base_UE
   {
       GENERATED_BODY()

   public:
       UFUNCTION(BlueprintCallable,
                 Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Try Project Point")
       static FCk_NavSurface_ProjectionResult
       Try_ProjectPoint(
           const UObject* InWorldContext,
           const FCk_NavSurface_ProjectionQuery& InQuery);

       UFUNCTION(BlueprintCallable,
                 Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Try Move Along Surface")
       static FCk_NavSurface_MoveAlongSurfaceResult
       Try_MoveAlongSurface(
           const UObject* InWorldContext,
           const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery);

       UFUNCTION(BlueprintCallable,
                 Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Try Surface Raycast")
       static FCk_NavSurface_RaycastResult
       Try_SurfaceRaycast(
           const UObject* InWorldContext,
           const FCk_NavSurface_RaycastQuery& InQuery);

       // THREAD CONTRACT: callable off the game thread against an immutable field snapshot.
       // C++-ONLY BY CONTRACT: no UFUNCTION on this entry. Blueprint/AngelScript reach the
       // capability through a separate game-thread UFUNCTION wrapper.
       // Results are written into caller-provided storage; no locks, no shared-pool allocation.
       // The Recast adapter honours this with the existing stack-local-query discipline.
       static auto
       Get_BoundarySegments(
           const UObject* InWorldContext,
           const FCk_NavSurface_BoundaryQuery& InQuery,
           TArray<FCk_NavSurface_BoundarySegment>& OutSegments) -> ECk_NavSurface_QueryStatus;

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Get Is Reachable")
       static ECk_NavSurface_Reachability
       Get_IsReachable(
           const UObject* InWorldContext,
           const FCk_NavSurface_ReachabilityQuery& InQuery);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Request Area Markup")
       static FCk_Handle_NavSurfaceMarkup
       Request_AreaMarkup(
           const UObject* InWorldContext,
           const FCk_Request_NavSurface_AreaMarkup& InRequest,
           const FCk_Delegate_Request_OnCompleted& InDelegate);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Get Is Markup Live")
       static bool
       Get_IsMarkupLive(
           const FCk_Handle_NavSurfaceMarkup& InMarkup);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Get Surface Revision")
       static int64
       Get_SurfaceRevision(
           const UObject* InWorldContext);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Get Surface Bounds")
       static FBox
       Get_SurfaceBounds(
           const UObject* InWorldContext);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Get Provider Health")
       static ECk_NavSurface_ProviderHealth
       Get_ProviderHealth(
           const UObject* InWorldContext);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Get Is Build In Progress")
       static bool
       Get_IsBuildInProgress(
           const UObject* InWorldContext);

       UFUNCTION(BlueprintCallable, Category = "Ck|Utils|NavSurface",
                 DisplayName="[Ck][NavSurface] Request Surface Rebuild For Testing")
       static void
       Request_SurfaceRebuild_ForTesting(
           const UObject* InWorldContext,
           const FCk_Delegate_Request_OnCompleted& InDelegate);
   };
   ```
   Notes that are contract, not taste: the completion delegate is **always the last parameter** and
   carries `meta = (AutoCreateRefTerm = "InDelegate")` with **no C++ default**;
   `BindTo_OnSurfaceRebuilt` / `UnbindFrom_OnSurfaceRebuilt` are generated by
   `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE`, not hand-written here.
   → verify: compiles; `Script/Generated/utils_navsurface.as` (or the generator's actual filename)
   appears after an editor boot and exposes every function.

   **Gate 0B-3.** After the AngelScript generation pass:
   - Every facade function present in the generated `utils_*.as` → continue.
   - A function is missing and it takes an `FString`/UE type → it is the known `EOrder::Late`
     binding-order trap; apply `EOrder::Late` and re-verify (this is a mechanical fix, not a
     redesign).
   - The editor fails to boot after adding the bindings → also the binding-order trap; same fix,
     then re-verify.
   - A function is missing for any other reason → **STOP**, record in PROGRESS.md Blockers.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

4. Author the per-world provider state and the Recast adapter behind it. Illustrative:

   ```cpp
   namespace ck
   {
       struct CKNAVIGATION_API FFragment_NavSurface_Provider
       {
           CK_GENERATED_BODY(FFragment_NavSurface_Provider);

       private:
           ECk_NavSurface_Provider _Provider = ECk_NavSurface_Provider::Recast;
           ECk_NavSurface_ProviderHealth _Health = ECk_NavSurface_ProviderHealth::Unknown;

       public:
           CK_PROPERTY_GET(_Provider);
           CK_PROPERTY_GET(_Health);
       };
   }
   ```
   `ECk_NavSurface_Provider` contains **only the providers that exist today** (`Recast`). PHASE_3
   adds its own entry. Do not pre-add one (PROMPT.md §2, "no speculative extension points").
   → verify: compiles; the provider fragment lives on a per-world entity, and
   `rg --no-ignore -n "^\s*static\s+.*GDeferredNavRequests|G[A-Z]\w*Nav" Plugins/CkFoundation/Source/CkNavigation`
   shows no *new* process-wide state.

5. Move the deferred-request queue per world ([NN-D8b]). Preserve verbatim: latest-wins revision
   semantics over the `[1, MAX_int32]` ring, the 5s force-fail watchdog, the drain order in which
   `Request_AbandonPath` purges in-flight and deferred entries **before** completing any.
   → verify: a Layer-2 multi-PIE autotest in which two worlds each hold in-flight deferred requests
   and neither observes the other's (VALIDATION.md A2, fourth bullet).

   **Gate 0B-5.**
   - Two worlds isolated, watchdog and latest-wins intact → continue.
   - Cross-world leakage still observable → the queue is not actually per-world (a static survived);
     find it, fix it, re-verify. Two failed attempts → STOP.
   - A completion delegate re-enters `AbandonPath` and double-fires or leaks an entry → **STOP**:
     the purge-before-complete ordering was not preserved verbatim.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

6. Consolidate the two duplicate revision observers ([NN-D8a]) into
   `Get_SurfaceRevision` / `BindTo_OnSurfaceRebuilt`; delete `CkQueue`'s
   `CkQueue_NavigationRevisionSubsystem`.
   → verify: `rg --no-ignore -n "OnNavigationGenerationFinished" Plugins/CkFoundation/Source` returns
   **one** implementation site (VALIDATION.md A2, fifth bullet).

7. Gate 0B: full suite on 0B's final artifact.
   → verify: delta-zero vs the §2 baseline.

---

## 6. Sub-phase 0C — Consumer migration sweep and `[RETIRE]` deletions (F1.38 cont.)

**0C entry:** 0B exit green.
**0C exit:** every row of R3's dependency table is disposed of — migrated or deleted — with a
one-row-per-R3-row checklist in PROGRESS.md.

### Steps

1. Build the checklist first: one row per R3 dependency-table row, columns
   `file | bucket | resolution | evidence`. Paste it into PROGRESS.md **before** editing.
   → verify: the row count matches R3's table; no row is blank.

2. Migrate the `[REIMPLEMENT]`/`[ADAPTER]` consumer sites to the facade, in this order (cheapest
   blast radius first): CkEqs → CkQueue → CkPathNetwork → CkGameplayDebugger → CkCrowd. CkCrowd's
   `ConstrainToNavmesh` (the single Transform writer for grounded agents) is **last** and alone in
   its own edit batch.
   → verify: after each module's batch, run the targeted pattern, e.g.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --test-pattern Crowd --parallel 1 --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
   ```
   and confirm that module's tests are unchanged from the baseline.

3. Delete — do not migrate — the `[RETIRE]` sites: `CkCrowdAgent_DiagNavClip_Processor`,
   `CkCrowdAgent_DrawNavProjection_Processor`, the `CkCrowd_DebugSettings` projection probe, and the
   diagnostic re-probe block in `CkNav_Algorithm.cpp`.
   → verify: `rg --no-ignore -n "DiagNavClip|DrawNavProjection" Plugins` → zero hits, and the
   `.Build.cs`/module files no longer reference the deleted processors.

4. Migrate both EQS inline-projection sites to the facade and **re-measure** ([NN-D8g]).
   → verify: an EQS timing number recorded in PROGRESS.md, measured, not estimated.

   **Gate 0C-4.**
   - Facade-routed EQS cost is at or below the recorded inline cost → the "keep the two in sync"
     note dies with the duplication; delete it.
   - Facade-routed cost is materially worse → **STOP** and record the two numbers. Re-introducing an
     inline duplicate is a design decision and belongs to the orchestrator, not this session.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

5. Correct the stale `CkCrowd.Build.cs:21` comment ([NN-D8h]) — six processors use the dependency,
   not one.
   → verify: the comment names the actual consumer count.

6. Gate 0C: full suite on 0C's final artifact.
   → verify: delta-zero vs the §2 baseline.

---

## 7. Sub-phase 0D — Test-fixture vocabulary migration (F1.42)

**0D entry:** 0C exit green.
**0D exit:** every `UNavArea_Null` fixture site paints its obstacle through the neutral markup
request and is green *on the Recast provider*.

### Steps

1. Author the AngelScript fixture wrapper so a fixture reads as one call. The obstacle tag is a
   **named, well-known area tag** — not an ad-hoc literal per test. Illustrative:

   ```cpp
   UFUNCTION(BlueprintCallable,
             Category = "Ck|Utils|NavSurface",
             DisplayName="[Ck][NavSurface] Request Impassable Box")
   static FCk_Handle_NavSurfaceMarkup
   Request_ImpassableBox(
       const UObject* InWorldContext,
       const FCk_Request_NavSurface_AreaMarkup& InRequest,
       const FCk_Delegate_Request_OnCompleted& InDelegate);
   ```
   In P0 this is routed to the existing Recast markup painter (`UCk_NavAreaMarkup_UE`). Its native
   implementation arrives in PHASE_4 (F1.31); the *call sites written here do not change again*.
   → verify: the wrapper appears in the generated `utils_*.as` and compiles from AngelScript.

2. Migrate the ten-plus named fixture sites (R3 §CkTests: `NarrowGap_TraverseCalm`,
   `NarrowGap_NoRouteFailsClean`, `NarrowGap_BlockedDetours`, `Stall_RepathsAroundLateObstacle`,
   `Stall_UnreachableGoalFailsBounded`, `Steering_CornerRetirementKeepsAgentOnMesh`,
   `OffPath_TeleportRepaths`, `Facing_CalmWhilePressingBlockedGap`, `CkQueueGym_PlayerController`,
   `Queue_NavigationChangeRetriesImpossibleFormation`) plus the two `CkTestsAssets.as`
   `TSoftObjectPtr<ARecastNavMesh>` references.
   Replace every hop-counted wait around a paint with a **named condition** on
   `Get_IsMarkupLive` — never `WaitOneFrame` arithmetic (VALIDATION.md §2.2 settling discipline;
   `WaitOneFrame` is 0.05s of wall clock, not one frame).
   → verify:
   ```powershell
   rg --no-ignore -n "UNavArea_Null|ARecastNavMesh" Plugins/CkTests/Script
   ```
   → zero hits, and the named tests are green:
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --test-pattern Crowd --parallel 1 --discover-fresh --output=Saved/Logs/BuildTest.log `
     --project="D:\Repos\CkPlugins_3"
   ```
   (`--discover-fresh` because fixture files changed; a green run whose Total matches the old count
   is **stale-green** and has not run what you think it ran.)

   **Gate 0D-2.**
   - All named tests green, Total unchanged, no test silently skipped → continue.
   - A fixture test is now flaky where it was stable → the paint-then-act wait became vacuous.
     Re-read VALIDATION.md §2.2's five questions and re-derive the condition. Two attempts → STOP.
   - A fixture test needs a *new* markup capability the neutral request cannot express → **STOP**:
     that is a design fork.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Gate 0D + phase gate: full suite on the phase's **final** artifact.
   → verify: delta-zero vs the §2 baseline, plus the standing gates of VALIDATION.md §0.

---

## 8. Exit criteria (measurable; the orchestrator re-runs all of them)

- [ ] `rg --no-ignore -n "UNavArea|UNavigationQueryFilter|TSubclassOf<UNav" Plugins/CkFoundation/Source/CkNavigation/Public/CkNavigation/Nav/CkNav_Fragment_Data.h`
      → **0 hits**.
- [ ] `rg --no-ignore -n "_ExcludedAreaClasses|_QueryFilterClassOverride" Plugins` → **0 hits**.
- [ ] `rg --no-ignore -n "UNavArea_Null|ARecastNavMesh" Plugins/CkTests/Script` → **0 hits**.
- [ ] `rg --no-ignore -n "DiagNavClip|DrawNavProjection" Plugins` → **0 hits**.
- [ ] Exactly **one** navigation-revision observer implementation remains (grep evidence recorded).
- [ ] All twelve facade capabilities have at least one test exercising them **through the facade**,
      not through a provider directly.
- [ ] The R3 disposition checklist in PROGRESS.md has zero unresolved rows.
- [ ] Multi-PIE deferred-queue isolation test green.
- [ ] Three environments exercised for every facade function: C++, Blueprint, AngelScript.
      **Carve-out:** `Get_BoundarySegments` — and any other function carrying the off-thread
      thread contract — is **C++-only by contract**. Blueprint and AngelScript get a
      game-thread `UFUNCTION` wrapper; the three-environments rule applies to that wrapper.
      The off-thread C++ entry is exercised by the Layer-1 thread-contract test instead.
- [ ] **Full suite delta-zero vs the §2 phase-entry baseline**, run on the phase's final artifact,
      zero new ensures, zero new warnings, editor boots clean.
- [ ] Provenance cells complete in FEATURE_MATRIX.md for F1.27, F1.38, F1.39, F1.42.
- [ ] Comment audit run over the phase diff (root doctrine's closing step).
- [ ] PROGRESS.md updated: evidence, decisions, session log; PHASE_1 entry criteria re-verified.

---

## 9. Fences

- **No new provider.** CkGroundNav does not exist in this phase. Adding a stub module, an empty
  enum entry, or a "future provider" branch is speculative extension — forbidden (PROMPT.md §2).
- **No behaviour change.** A test result that moves in *either* direction is a defect, including
  red→green (VALIDATION.md A1).
- **No back-compat shims.** The old typed members are deleted in the same change that replaces them
  (CkFoundation doctrine). A `_DEPRECATED` member is a review rejection.
- **Do not invent a second install path.** `MarkPathPending` / `InstallExternalPath` /
  `AbandonPath` / `FailPath` + the revision ring is the seam. It is already provider-neutral and
  stays byte-compatible ([NN-D8] ground rules).
- **Do not touch the episode lifecycle.** CkCrowd's advance-revision → abandon → mark-pending →
  dispatch-exactly-one → install-only-fresh → release-terminal sequence is preserved *verbatim*.
- **Do not rewrite `CkNavmeshDebugDraw`.** It is retired at Recast retirement, not rewritten
  ([NN-D8f]). Leave it alone this phase.
- **Do not delete `Request_SetActorNavigationRegistered`** — [NN-D8c] keeps it through migration
  because the Recast adapter needs it.
- **Settle on named conditions, never hop counts.** `WaitOneFrame` is 0.05s of wall clock. Markup
  and field state are global to the shared PIE world, so a predicate must name *this test's* own
  entities (VALIDATION.md §2.2, question 2).
- **Every processor added gets `CK_REGISTER_PROCESSOR`** in its `.cpp`. An unregistered processor is
  an unscheduled no-op that fails silently.
- **`--discover-fresh` whenever an AngelScript autotest file changed**, and remember a new C++
  automation test needs touch+relink, a new *net* AS autotest needs a full C++ rebuild.
- **Stage only what you authored.** Never a blanket directory add. Commit/push only when the
  maintainer asks.

---

## 10. [P0] Done means

VALIDATION.md **A1** (contract neutralization) and **A2** (facade and adapter) are green with
evidence in PROGRESS.md, every VALIDATION.md §0 standing gate passes on the phase's final artifact,
and the F1.42 fixture vocabulary is fully served through neutral markup on the Recast provider —
which is promotion precondition **B2** banked early.
