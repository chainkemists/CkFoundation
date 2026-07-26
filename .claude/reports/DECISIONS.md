# DECISIONS.md — doctrine-vs-code triage record

Campaign: principal handoff documentation (CLAUDE.md rewrites + skill library), started 2026-07-02.
Scope: CkFoundation, CkGameplayDebugger, CkTests submodules.
Rule applied: uniform code practice beats stale doctrine; doctrine beats scattered violations;
genuine principal-level forks escalate to [ADJUDICATIONS.md](ADJUDICATIONS.md).
Format per item: **pattern → winner → rationale → confidence**. Evidence details live in the
discovery reports (session scratchpad) and the skills that cite them.
Numbering note: entries are append-ordered; numbers are stable IDs, not a reading sequence —
34–35 live under "Meta" at the end of the file.

## Version / identity facts

1. **Engine version statement** → pin "UnrealEngine-Angelscript 5.7.x (5.7.4 verified)" → docs said 5.5 (root), 5.6 (Source:244), "5.5+" (Script); disk Build.version = 5.7.4 at D:/Repos/UnrealEngineAngelscript, checked 2026-07-02 → confidence high. [DOCTRINE UPDATED 2026-07-02]
2. **EnTT version** → 3.16.0 → docs claimed 3.15.0 twice; vendored source `CkThirdParty/.../entt-3.16.0/src/entt/config/version.h` says 3.16.0 → high. [DOCTRINE UPDATED 2026-07-02]
3. **`ck::SelfEntity(this)`** → replaced by `ck::ToEntity` in all docs, no compatibility alias suggested → renamed in commit f72a93416; zero in-tree uses of old name; docs taught non-compiling code → high. [DOCTRINE UPDATED 2026-07-02]

## Style rules (code practice vs written rule)

4. **Fragment naming** → teach the two-tier scheme: reflected `FCk_Fragment_[Feature]_ParamsData` (USTRUCT) vs runtime `ck::FFragment_[Feature]_[Type]` (plain C++ in ck namespace) → written single-tier rule matches neither count (124 reflected vs 338 runtime); code is uniform on the richer scheme → high. [DOCTRINE UPDATED 2026-07-02]
5. **UObject refs in fragments** → teach ownership split: `TStrongObjectPtr` when the entity owns the object's lifetime (13 uses), `TWeakObjectPtr` for non-owning references (35 uses); both sanctioned, same feature may use both → written rule ("TStrongObjectPtr in fragments") describes only half the convention → high. [DOCTRINE UPDATED 2026-07-02]
6. **UFUNCTION overload-suffix vocabulary** → teach observed vocabulary `_ByName` / `_ByTag` / `_Simple` / `AddOrReplace` / `INTERNAL__` → `_Advanced` (named in prior doctrine) has no significant presence; `_ByName` alone has 48 uses → high. [DOCTRINE UPDATED 2026-07-02]
7. **Processor suffix vocabulary** → teach observed set (Setup 68, HandleRequests 64, EndPlay 26, Replicate 14, Update 13, ReplicateOnRestore 11, FireSignals 8, …) → `Teardown` is effectively absent from 369 processors; prior example vocab was stale → high. [DOCTRINE UPDATED 2026-07-02]
8. **Processor registration** → `CK_REGISTER_PROCESSOR` (+ ProcessorScript subsystem) is the current mechanism → `ProcessorInjector` retired: zero files at HEAD, still present in root CLAUDE.md layout diagram → high. [DOCTRINE UPDATED 2026-07-02]
9. **"Dependency Injection" naming** → architecture docs name the real mechanisms: CkProvider + ContextOwner → no module or type named DI exists; teaching a phantom name sends readers hunting → high.
10. **b-prefix on bools** → rule stands (no `b` prefix) → violations are ~10%: engine-forced Iris trait fields (exempt, engine contract) and CkCamera's `bInEnabled`-style API (listed as framework debt) → high.
11. **Enum-over-bool options** → rule stands (dominant 4.5:1; `ECk_EnableDisable`/`ECk_SucceededFailed` family + `ExpandEnumAsExecs`) → CkCamera again the main counterexample; debt, not doctrine change → high.
12. **Editor-module style relaxation** → documented as observed reality (runtime modules strict; editor/UncookedOnly modules visibly relaxed: bare `IsValid`, `if (!`, single-line defs) — not presented as license to relax new code → medium.
13. **Comment doctrine** → refined to "no what-comments; yes why-comments": processors 0–2.8% comment lines, but Utils public headers carry 111 `/**` contract blocks and fragments carry rationale comments → the flat "no unnecessary comments" rule under-described real practice → high. [DOCTRINE UPDATED 2026-07-02]

## Scope and authoring calls

14. **Nested CLAUDE.md files** → root + Source/CLAUDE.md + Script/CLAUDE.md rewritten (they are the plugin's doctrine surface); CkGameplayDebugger's `Source/CkDebuggerCommon/CLAUDE.md` kept as-is (spot-verified accurate 2026-07-02) and pointed at → medium.
15. **Per-module doc network (89 `Claude.md` + 48 READMEs in CkFoundation)** → pointed at, not rewritten in this campaign; per-module staleness (7 modules undocumented; CkCrowd/CkNavigation docs claim "skeleton" for implemented modules; CkScripts doc wrong; Ensure README wrong twice; CkTimer doc teaches `OnFinished`/`OnTick` but real signals are `OnTimerDone`/`OnTimerUpdate` — CkTimer_Fragment.h:96-103) recorded as framework debt → medium (scope call; a per-module sweep is a separate campaign).
16. **CkTests documentation stance** → host-agnostic (plugin truth) with BusterBlock as the worked example → the plugin is dual-hosted (CkPlugins + BusterBlock) and ships zero host runner scripts → high.
17. **CkTests stale campaign docs** (3 CONTINUATION_PROMPT files + Progress.md referencing the CkPlugins superproject) → flagged as misleading debt with a deletion recommendation; NOT deleted (outside this campaign's write scope) → high.
18. **Legacy CkGameplayDebugger generation** (IGameplayDebugger category + DebugProfiles) → documented as maintenance-only (frozen since early 2024 by commit history); not proclaimed deprecated — that is the maintainer's call → medium.
19. **Campaign-skill target selection** → entity-teardown / signal-unbind lifecycle correctness, anchored on the live `CkInteractTarget_Processor.cpp:222`-area defect (verbatim comment: "This processor doesn't get called, can cause issues if teardown is mid interaction!!!"). Campaign authoring (2026-07-02) sharpened the working hypotheses: the EndPlay processors likely DO fire — the live gap is the Failed-end request being unconsumable (`CK_IGNORE_PENDING_KILL` on the request handler, CkInteraction_Processor.h:18, + empty `FProcessor_Interaction_EndPlay`, .cpp:72-80) plus an independent world-teardown no-pump path (CkEcsWorld_Subsystem.cpp:156-182); corroborating anchor: `CkAutoTest_Interaction_TimedInterruptedByCancel.as:13-16` explicitly dodges the broken path ("target destruction would leak the interaction"). Hypotheses, not conclusions — Phase 1 of the campaign proves them; adjacent evidence: 3× TeamListener unbind TODOs, signal `in_place_delete` workaround (2c8319c1c) → maintainer had no named "hardest problem"; selected from Phase-0 evidence per campaign spec; robustness-first per maintainer's stated values → medium.
20. **Cross-drive worktree trap** → NOT documented as an incident story → zero trace in any repo/doc AND unknown to the maintainer when asked (2026-07-02); playbook carries only verifiable worktree/multi-session mechanics, with operator-session experience labeled as such → high (inventing it would violate ground-truth-only).
21. **Packaged-GC-crash narrative** → told as git records it: diagnostic d77810096 → root-cause pre-GC rooting pass feb08ee94 → tripwire ensure a8a93baac (tripwire = detection, not the fix) → campaign brief called the ensure a symptom patch; maintainer did not contradict the git version when it was surfaced → medium.
22. **Silent-error mandate** → encoded as a top-tier non-negotiable: fire `CK_ENSURE_IF_NOT` instead of log-and-continue; never stock `ensure`/`ensureMsgf` → maintainer's direct statement (2026-07-02): logs get ignored, ensures don't; matches observed failure mode of incoming engineers/models → high.
23. **Research-first mandate** → encoded as doctrine (read neighboring feature implementations before writing; cite what you read) → maintainer's direct statement: incoming sessions "don't research the codebase enough" → high.
24. **Unwritten-norm protocol** → "ask the maintainer; never invent policy" encoded in doctrine; ADJUDICATIONS.md is the standing ask-list → maintainer's direct statement → high.
25. **Feature-frontier constraints** → stalled/outdated branches listed as do-not-resurrect; candidates ranked robustness > performance > debug tooling (tooling usually lands in CkGameplayDebugger) → maintainer's direct statement → high.

## Macro/API documentation calls

26. **`CK_DISABLE_ENSURE_CHECKS` via BuildConfiguration.Profile** → documented with a [possibly-vestigial] label → only reachable through a profile override (CkBuildConfig.Build.cs:47); no evidence of use; design intent unconfirmed → medium.
27. **Request-struct vtable variance across configs** (`CK_DISABLE_ECS_HANDLE_DEBUGGING` adds/removes a virtual, CkRequest_Data.h:95-103) → documented as a binary-compat constraint without claiming it is intentional design → medium.
28. **`CK_GENERATED_BODY_HANDLE_DERIVED`** (handle-to-handle inheritance, 7 uses + the macro definition) → documented as rare/advanced, not core teaching → medium.
29. **UObjects × `CK_DEFINE_CONSTRUCTORS` incompatibility** → mechanical reason documented from expansion (AS-branch placement-new is struct-only, CkMacros_AngelScript.h:230); the UHT default-ctor-collision half is labeled inferred → 541 uses, zero on U/A types confirms the rule empirically → high on the rule, medium on the second mechanism.

30. **Anonymous namespaces / file-local `static` helpers** → banned; use a filename-derived named
    namespace (`namespace ck_timer_processor`) → old root doc said "use `static` or internal
    classes", but unity builds concatenate TUs and collide file-local symbols; maintainer feedback
    (recorded 2026-06) mandates the named-namespace form → high. [DOCTRINE UPDATED 2026-07-02]
31. **`ck::SelfEntity` correction detail** → the old doc's C++ handle-pattern line described an
    AS-side global; corrected split: C++ actor→entity = `UCk_Utils_OwningActor_UE` family; AS =
    `ck::ToEntity(AActor)` / `ck::ToEntity(UCk_EntityScript_UE)` (Script/CkUtils_Common.as:5,10) → high.
32. **Tier-table regeneration (Source/CLAUDE.md)** → all 73 non-editor rows re-derived from Build.cs
    2026-07-02 → beyond the 3 known drifts, 6 more found and fixed (CkTween +Spline; CkRaySense
    +IsmRenderer; CkCamera +Attribute; CkChaos −Attribute; CkIskmRenderer −AnimGraphRuntime
    +IskmRendererVF; CkGrid→CkEntitySpawner annotated editor-only); 3 UncookedOnly modules moved to
    T5 → high. Debt noted: `CkScripts/CkEcsTemplateReplacer.ps1` scaffolds from the deleted
    CkEcsTemplate; CkGoap's per-module doc is uppercase `CLAUDE.md` while siblings use `Claude.md`
    (case-sensitivity hazard for non-Windows tooling).
33. **Gym console commands** → docs teach exec names `Ck_Gym_Prev` / `Ck_Gym_GoTo` / `Ck_Gym_List`
    → earlier drafts (and the gym spec §9) used DisplayName-style names; real exec names verified at
    `CkGym_Base_PlayerController.as:255-287` → high.

36. **CkTimer `BindTo_*` wrappers silently drop `InPostFireBehavior`** (framework debt, found
    2026-07-02) → documented as divergence; docs teach `CkEntityCollection_Utils.cpp:207-215` as
    the canonical bind-wrapper shape → CkTimer_Utils.cpp:432-444 et al. accept the parameter but
    call `Utils::Bind` without it (0 `CK_SIGNAL_BIND`/`_PostFireUnbind` hits in that file) — callers
    requesting `PostFireBehavior::Unbind` on Timer signals are silently not unbound; adjacent to the
    lifecycle campaign's unbind-debt cluster → high (fact), fix is maintainer's call.

37. **Slate debugger tab menu location** → docs say Tools main menu → Developer Tools category →
    the "Window → Developer Tools" claim (discovery + first CkGPD/CLAUDE.md draft) is stale vs
    engine 5.7.4 (`WorkspaceMenuStructureModule.cpp:184-185`, RegisterToolsMenu) → high. [DOCTRINE UPDATED 2026-07-02]
38. **Stale in-code comment: `CkDynamicHandleSubsystem.h:17-23`** still teaches "3. Restart editor",
    contradicting the no-restart `ForceRefreshDynamicHandleBindings` contract documented in the
    module's Claude.md → recorded as framework debt (code comment fix is outside this campaign's
    write scope); the interop skill teaches the no-restart path → high (fact).
39. **Process rule taught by ck-methodology** → living campaign docs update in the same commit that
    lands the gate ("doc updates weld into the gate's landing commit — a rule written inside the doc
    is not enforcement") → derived from PLAN.md:89's own update rule being skipped for 67 commits
    while work continued → medium (new rule, distilled from observed failure, not maintainer-ratified).

40. **Net-stub timeout drift (CkTests)** → documented as-is: net autotest stubs run with a
    hard-coded 30s TimeLimit → `CkAutoTestNetStubGenerator.h:44` claims `_TimeoutSeconds` is read
    from the CDO, but the generator emits `30.0f` for every stub (`.cpp:266`; all 38 on-disk stubs
    confirm) — propagate-the-CDO vs fix-the-comment is the maintainer's call → high (fact).

41. **`CK_DISABLE_ABILITY_SCRIPT_DEBUGGING` is vestigial** → confirmed (zero consumers outside
    CkBuildConfig.Build.cs; Ability modules removed in 93996692c) → documented as a cleanup
    candidate, not removed (write scope) → high.

42. **`CK_ENABLE_MEMORY_TRACKING=1` cannot compile** (framework debt, found 2026-07-02) → documented
    as broken/vestigial in ck-performance-and-analysis → `FProcessor_Memory_Stats` reads
    `ck::detail::BytesAllocated` (CkMemory_Processor.cpp:23) which is defined nowhere; the allocator
    is a TODO stub (CkMemoryAllocator.h:14); `CkMemory/Claude.md` oversells "allocation counting" →
    high (fact).
43. **Scheduler debug-timing comment drift** → `CkProcessorScheduler.cpp` Dispatch comment says
    debug timing is "compiled out in Test/Shipping" but the guard is `#if !UE_BUILD_SHIPPING`
    (present in Test) → docs teach the guard, not the comment → high (fact).

44. **Global tombstone-mode storage documented in doctrine** → root CLAUDE.md now states the
    `CkHandle.h:71-77` global `in_place_delete` specialization (every pool pointer-stable; owning
    groups unavailable; EnTT swap-and-pop default inverted) → previously documented NOWHERE despite
    being load-bearing for iteration semantics and a standing perf ceiling; independently confirmed
    by three Phase-2 agents; design intent resolved — see §45 → high (fact).

45. **Global pointer stability is deliberate design** (resolves the A3 intent question) →
    `06938bba3` (2026-02-17, "feat: fragments are always pointer stable") deliberately removed the
    debug gate around the global `entt::component_traits<Type>` specialization → chain re-derived
    2026-07-02 via `git log -G component_traits -- Source/CkEcs/Public/CkEcs/Handle/CkHandle.h` +
    diffs: introduced gated in `745507381` (2024-03-07; message and code agree on debug scope) →
    relocated, still gated, in `6b54d2e384` (2024-04-12, DEBUG_NAME storage fix) → ungated on
    purpose in `06938bba3`. Consequences stand: every fragment pool tombstone-mode in every config;
    EnTT owning groups statically unavailable (`group.hpp:697`); the per-signal `in_place_delete`
    in-class opt-ins (`2c8319c1c`) shadowed. ADJUDICATIONS A3 demoted per the campaign's own triage
    rule (git answered the escalated question); the remaining owning-groups perf ceiling is a
    prioritization item — `ck-feature-frontier` candidate 5 → high.

## Meta

34. **Skill-testing methodology** → the campaign's Phase-3 review gate (FACTUAL re-verification + DOCTRINE consistency + USABILITY cold-execution by a smaller model) serves as the test-before-deploy gate for these reference/runbook skills → the superpowers TDD-for-skills Iron Law targets discipline skills in a personal library; its reference-skill test (retrieval + application) is exactly what Phase 3 implements → medium.
35. **Skill frontmatter** → `name` + trigger-only `description` ("Use when …", symptoms/error strings, no workflow summary) → per agentskills spec + Claude Code docs (checked 2026-07-02): description drives model loading decisions; workflow summaries cause agents to skip skill bodies → high.

---

# Consumer campaign (2026-07-03) — ck-game-* skill library

Campaign: consumer-layer skills (building GAMES on CkFoundation) + PROJECT_TEMPLATE, started
2026-07-03. Corpus: BusterBlock (primary) + Venus (`D:\Repos\Venus`, maintainer-named second
consumer; framework pin 2026-03-25 — mostly current-era, its Actor-tick processors and free Actor
usage are pre-doctrine). Same triage rule as above. `[PROMOTED FROM CORPUS 2026-07-03]` = uniform
consumer practice adopted as the standard where framework doctrine was silent. Maintainer rulings
of 2026-07-03 (AS-first; driver MVC endorsed; global access + band-aids = worst debt; manual
bootstrap; discovery-timing = #1 failure mode) are cited as [MAINTAINER-RULED]. New forks: A5–A7.

## Division of labor / architecture

46. **C++/AS/BP split** → AS is the standard for ALL consumer gameplay incl. fragments/processors; C++ "rarely, unless it's _very_ clear it's going to be a perf concern"; BPs are asset shells → [MAINTAINER-RULED] + census (185,725 game-AS lines vs 2,188 C++; 135 AS processors, 592 entity scripts, 0 game C++ fragments; last 200 commits 2,536 .as vs 22 .cpp) → high.
47. **Drop-to-C++ threshold** → no numeric bar; requires MEASURED evidence (ck-performance-and-analysis discipline) + maintainer ask on ambiguity — decided below escalation bar (maintainer just ruled "rarely"; a number now would be invented policy) → medium.
48. **Driver (controller) MVC architecture** → standard for all consumer games; Driver=Controller, subordinates own mechanism, never a global service locator → [MAINTAINER-RULED 2026-07-03 "the one I am most proud of"] despite BB-only corpus → high. `[PROMOTED FROM CORPUS 2026-07-03 — maintainer-endorsed]`
49. **Driver lookup** → Acquire ticket + Promise_OnReady (guaranteed) / descendant-scoped TryFind (opportunistic) / Params injection (strongest); NOT `ck::Ctx` walking (all corpus ck::Ctx sites are SM nodes resolving their own subject) and never a global accessor → high.
50. **No custom C++ engine framework classes** → GameMode is BP/AS; zero engine-class overrides needed to run CkFoundation (BB's own root doc describing BbGameEngine-family is stale — classes don't exist) → high. `[PROMOTED FROM CORPUS 2026-07-03]`
51. **No-Actors doctrine for consumers** → EntityScripts are the placeable unit; sanctioned Actor exceptions ≈ player pawn, physics props, engine-boundary bridges (`UCk_EntityScript_WithActor_UE`) → maintainer-standing + Venus free-Actor usage ruled old-era → high.

## Feature anatomy / recipe

52. **Feature dir layout** → full quartet incl. EntityScript colocated under `Script/ECS/<Feature>/` (Venus splits EntityScripts by topic; colocation keeps the placeable unit next to its contract) → high. `[PROMOTED FROM CORPUS 2026-07-03]`
53. **AS processor form** → `UCk_Processor_Script_Base_UE` subclasses, one concern per file, `_MarkedDirtyBy`-driven; Venus Actor-tick + prerequisite chains = old-era, do not teach → high. `[PROMOTED FROM CORPUS 2026-07-03]`
54. **Tests land same-commit as behavior; three-slice shape** (unit-on-logic-namespace + compose + driver-routed e2e, collapsible for small features) → uniform in BB commit log; Venus has zero tests (ruled non-exemplar here) → high. `[PROMOTED FROM CORPUS 2026-07-03]`
55. **Spawn-vehicle split** → visuals/assets live on the EntityScript; the gameplay feature stays asset-free (enables asset-free tests; stated in-source, Trashcan header) → high.
56. **Discovery-tag semantics** → tag = promise the feature contract holds; stamp LAST after composition; CK_ENSURE on tag→cast; scope all scans → fix commits `1ca589b0d`/`287ee6601` → high.
57. **Request-processor re-entrancy** → clear the request fragment BEFORE broadcasting results → in-source rule (Trashcan) → high.
58. **Signal authoring shape** → delegate + `_MC` event pair in a lazy `_Signals` fragment with BindTo_/UnbindFrom_ mixins → identical in BB and Venus → high. `[PROMOTED FROM CORPUS 2026-07-03]`
59. **ExposeOnSpawn lists are frozen positional APIs** → append-only; prefer auto-discover for non-caller fields → incidents `01a39b58f` (724 self-heal recoveries/launch), `8c7cc4f07` → high. `[PROMOTED FROM CORPUS 2026-07-03]`
60. **Attribute mutation reads** → values are float32; read via OnValueChanged payload / settle a tick, never same-frame Get → high.
61. **`k_` constants in `constants_<feature>` namespaces** → uniform BB; unconfirmed in Venus → medium.

## Composition / lifetime

62. **Self-overriding features live on their own child node** → any feature that `Request_OverrideToSelf()`s (Interactable, FlyerRecipient) composes on a dedicated scene/probe-node child, never a shared entity → NPC FlyerRecipient incident ("AtStore never fires") → high. `[PROMOTED FROM CORPUS 2026-07-03]`
63. **Interactable topology** → feature on a child probe node; consumers use `Get_Interactable()`; resolve overlap owners back via context → only corpus form, encoded in utils → high. `[PROMOTED FROM CORPUS 2026-07-03]`
64. **DoConstruct discipline** → `Request_OverrideToSelf()` before creating children on behavior-hosting entities; arm async promises LAST (inline-resolution) → StoreDriver INVARIANT block + NPC cached-self bug → high. `[PROMOTED FROM CORPUS 2026-07-03]`
65. **Discovery scoping ladder** → explicit refs > lifetime-descendant filter > interior-anchored radius prune (fail open) > per-scope tags; never raw global tag scans → `287ee6601` + [MAINTAINER-RULED: global access = worst debt] → high.
66. **Persistent query handlers delta-gate** → on `_Added`/`_Removed` after a guaranteed first reconcile (bind-time replay pass carries the full set with empty deltas) → `531b0c956` (~250ms/frame incident) → high.
67. **Two-axis discovery semantics** → axis 1 = always-on wall-clock window with re-sweep at close; axis 2 = optional minimums contract via EntityTagQuery; unmet minimums at close → warn + diagnostic tag + still stamp Ready (never hang consumers) → high.
68. **Subordinate coupling** → inject specific dependencies via Params; no driver-handle backlinks; upward flow via signals the driver binds → consistent corpus convention, never stated in-code → medium.

## Replication

69. **Replicated spawns anchor to an ActorRelay channel entity as lifetime owner** → never `ck::TransientEntity()`/non-replicating owner (framework guard enforces); Venus `AVns_EcsReplicatedAnchor` precursor superseded → incident `de3099e1c` + archaeology §7 → high. `[PROMOTED FROM CORPUS 2026-07-03]`
70. **Client→server mutation** → routes through a Reliable cue owned by the replicated entity, body authority-gated, carrying desired-state not toggle → `a421b2ce2`/`e140915f7` → high. `[PROMOTED FROM CORPUS 2026-07-03]`
71. **Replicated scripts return Finished from DoConstruct on the server path**; async readiness via a separate Ready contract (Continue past the Replicate window = never replicates) → `1b0665a3f` → high. `[PROMOTED FROM CORPUS 2026-07-03]`
72. **Rep-notify consumers bind then reconcile-once** (corpus form); replay-capable binding policy taught as valid alternative → `d6f9785a4` → medium.
73. **High-count agents** → DoesNotReplicate simulate-everywhere with authority-gated decision tier → single corpus family + open designer review; taught as considered trade-off, not doctrine → medium.

## Build / cook / bootstrap

74. **AS staging** → `DirectoriesToAlwaysStageAsNonUFS` one line per AS-bearing enabled plugin (fork stages only project Script/ by default; omission = packaged AS error wall / no boot) → `b8da4ad3b`/`e0de34899` → high. `[PROMOTED FROM CORPUS 2026-07-03]`
75. **Test exclusion triple** → NeverCook gym/autotest content + `DisablePlugins("CkTests")` in Shipping/Test targets + test AS in a `<Game>Tests` editor-only plugin (EnabledByDefault:false, TargetAllowList Editor) → Shipping incident `e0de34899` (431 AS errors, no boot) → high. `[PROMOTED FROM CORPUS 2026-07-03]`
76. **Fresh-workspace cook is inherently two-pass** (AS self-heal stubs); retries stay in ONE uncleaned workspace; never commit generated `*_EntitySpawnParams.as` (permanent cook wedge `b9b8a2214`) → high.
77. **Cook-time third-party error handling** → `-DisablePlugins=` via AdditionalCookerOptions over `-IgnoreCookErrors` (plugin names = project residue) → medium.
78. **Packaged/Test boot gate** → part of definition-of-done: launch `-log`, grep `Angelscript: Error` → composes existing doctrine → medium.
79. **Engine association** → teach both HKCU-GUID (per-machine!) and relative-path EngineAssociation; neither forced as default → high.
80. **Game Build.cs Ck modules** → start minimal (CkCore/CkEcs/CkLog + AngelscriptCode), add à-la-carte as C++ includes them; BB's 44-module list ruled historical accretion → high.
81. **Collision config** → author only the CkSensor/CkMarker channel lines; profiles self-register (CkOverlapBodyEditor module) — BB's serialized profile blocks are editor output, not required input → high.
82. **`SpawnTransform` property name on placeable EntityScripts** → leverages CkEntitySpawner auto-bind fallback (CkEntitySpawner_Actor.cpp:53-56) → high.
83. **Stale `_ProcessorInjectors` ini key** → matches no C++ member in any plugin (rg sweep 2026-07-03); silently ignored; never copy to new projects; doctrine (CK_REGISTER_PROCESSOR + script-processor base) stands → high (fact).

## Debugging / testing discipline

84. **Consumer triage ordering** → composition/discovery timing FIRST (before signals/replication/handles) → [MAINTAINER-RULED: #1 failure mode of lesser models] + corpus fix-commit frequency → high. `[PROMOTED FROM CORPUS 2026-07-03]`
85. **Cross-entity BindTo_ requires matching UnbindFrom_ in DoEndPlay** → dead-script invoke incident `05d2dc527` → high. `[PROMOTED FROM CORPUS 2026-07-03]`
86. **A timer/delay is never the fix for a discovery race** → readiness promise / delta-gated rescan instead → [MAINTAINER-RULED: band-aids = worst debt] → high.
87. **Breadcrumb constants** (`constants_<feature>::k_Breadcrumb_*`) shared between production emit and test/log watchers → medium. `[PROMOTED FROM CORPUS 2026-07-03]`
88. **Production `Debug_Force*` bypass requests** → sanctioned diagnostic surface, not test pollution → medium. `[PROMOTED FROM CORPUS 2026-07-03]`
89. **`_TimeoutSeconds` override with justifying comment** → the norm, not exception (155/216 corpus tests) → high.
90. **ActorRelay channel form in tests**; `_NonReplicating` subclass family = deprecated do-not-extend (exit ramp named in its own header) → high.
91. **Warnings-fail opt-out** → per-test `Get_ExpectedLogErrors`, narrow patterns only (CkAutoTestRunner.cpp:400-460) → high.

## Boundary / process

92. **Boundary contract = 9 may-nevers** grounded 1:1 in root non-negotiables + tier rules + 2026-07-03 rulings; game-side request fragments drained by game AS processors = sanctioned composition, not a breach → high. Breach sweep of BB game code: zero violations found (fragments/processors/`_Requests` writes/raw registry/friends/stock ensures — all 0).
93. **Gap-filing channel** → maintainer ask + ADJUDICATIONS ask-list (forks) / frontier-shaped write-up (capability gaps); no dedicated GAPS.md invented — decided below escalation bar (root #6 already names the mechanism) → medium on frontier-as-consumer-intake (it self-describes as portfolio, not inbox — flagged inside ck-game-framework-boundary).
94. **Promotion end-state** → framework-owns-mechanism / game-owns-registration (CkTests Common harness + RegisterProjectGym split as model) → high.
95. **Per-project residue home** → fill-in `PROJECT_TEMPLATE/CLAUDE.md.template`, facts-only, doctrine-by-link, `(verified <date>)` stamps + milestone re-stamp-or-tombstone + dated divergence ledger → BB/Venus project-doc rot as evidence → high.
96. **Local-skill policy** → consuming projects may author local skills ONLY for third-party SDK integration and packaging/distribution; everything else generalizable-or-gap → campaign charter (given) → high.
97. **Consumer reports appended to framework-campaign files** → DECISIONS/ADJUDICATIONS continue here (stable append IDs); CONFORMANCE_BACKLOG.md new → medium (session decision, revert hook offered).

## Object pooling (campaign 2026-07-11 — pivot from the `feature/pool-module` branch)

98. **Object pooling is a CkCore intrinsic, not a module** → hoisted into `UCk_Utils_Object_UE::Request_CreateNewObject` (pooled overload); lives in `CkCore/ObjectPooling`, not a standalone `CkPool` module → [MAINTAINER-DIRECTED] whole-framework promotion. The `feature/pool-module` CkPool module (EntityPool + ObjectPool + Poolable interface) is the port source, never merged. Full campaign: `docs/campaigns/object-pooling-core/`.
99. **Pin-everything ownership model** → the subsystem pins EVERY instance it hands out (poolable AND force-new); fragments go uniformly `TWeakObjectPtr`. Rationale: UE GC does not trace fragment members, so a weak-only ref to a force-new instance would be collected mid-life (outer does not root inner) → high. Killed the original spec's "outer keeps non-pooled alive" assumption.
100. **EntityPool (construct-once) DROPPED** → object-level pooling re-runs Construct/BeginPlay per acquire; construct-once entity pooling contradicts the pivot → [MAINTAINER-RULED]. Revive from the branch only if profiling shows feature-composition cost dominates.
101. **Actor pooling EXCLUDED** → the subsystem hands out plain UObjects only; actors go through SpawnActor. Actor-class acquire ensures → high (scoping call).
102. **No per-instance recycle veto** → the branch's `_CanBePooled`/`ICk_ObjectPool_Poolable` are gone; "never recycle" = the `DestroyOnRelease`/`InstancedPerEntity` policy, per-use safety = the participant's `OnReleasedToPool` quiescence → [MAINTAINER-RULED redundant]. `TryReleaseToPool` is a benign no-op (returns Failed, no ensure) for untracked objects, so teardown paths call it ungated.
103. **Participant opt-in mirrors ContextReceiver** → `FCk_Handle_ObjectPoolingParticipant` (reflection-scanned property, IncludeSuper), NOT a UInterface (AS can't implement one). Payload-free hooks; idempotent binds so re-running Construct on a recycled instance can't double-fire; the recycle reset SKIPS this property so binds survive → high.
104. **Sweep of `TStrongObjectPtr` fragment holders → subsystem-pinned weak** → 6 members (CkUnrealComponent, CkPmg ×2, CkAudio, CkUI ×2) route through DestroyOnRelease vends; teardown = unpin (release) then the real UE teardown (DestroyComponent). Must-stay-strong: engine-factory objects (Niagara/render-target/transient-texture) and caller-supplied asset/archetype pins → high. Sweep audit + rationale in the campaign PROGRESS.md.
105. **`Get_ScriptInstance` deliberately NOT exposed** → reaching the raw EntityScript instance is an encapsulation breach; the recycle contract is provable through pool stats + the direct plain-object API → [MAINTAINER-RULED]. Tests observe accordingly.

## Framework-side nominations surfaced (frontier-shaped candidates, not adjudications)

N1. Generic acquire-ticket feature ("CkAcquire") — three near-identical driver implementations (StoreDriver/DayCycle/MissionDriver).
N2. Ergonomic one-call replicated-lifetime-owner helper (Venus had `TransientEntity_Replicated()`; kills the most common replication footgun).
N3. Generic discovery-binder helper (~300 lines of per-driver Bind_All/dedupe boilerplate).
N4. Generic cook-with-retry script in CkAuto (mechanism is framework; only BB's copy exists, in .runreal).
N5. CkTimer `BindTo_*` PostFireBehavior drop (§36) — framework fix or generated-wrapper warning.
N6. Maintainer confirm: always-cook EntitySpawnParams dirs rationale (cook dependency walker blindness — inferred).

## Lifecycle teardown (campaign resumption 2026-07-26 — anchors #1/#2 closed)

106. **Destroy-mid-interaction defect CONFIRMED and fixed (H1)** → red repro `CkAutoTest_Interaction_DestroyTargetMidInteraction_SourceHearsFailed.as` (CkTests) timed out at HEAD exactly per the §19 hypothesis chain: the target's EndPlay queues `Request_EndInteraction(Failed)` on the pending-kill interaction entity, `FProcessor_Interaction_HandleRequests` (`CK_IGNORE_PENDING_KILL`) never consumes it, and `FProcessor_Interaction_EndPlay` was empty. Fix = campaign option A1: the interaction's own EndPlay processor broadcasts `OnInteractionFinished(Failed)`, guarded by the new `FTag_Interaction_FinishedBroadcastSent` (stamped by the normal request-path broadcast) so a normally-finished interaction destroyed in the same frame never double-broadcasts. Handles stay `IsValid` through the EndPlay window (validity flips at Teardown, `CkHandle.cpp:208-210`), so the existing listener-callback ensures do not fire. LEFT OPEN: anchor #4 TeamListener unbind debt (option B-team) and H2 world-teardown no-pump (fenced — needs maintainer call); the stale-sounding TODOs in InteractTarget/InteractSource EndPlay processors were left in place pending proof they are false → high.
