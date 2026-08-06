---
name: ck-macros-and-codegen
description: "Use when reading or writing CK_ macros or adding Ck fragments, processors, requests, signals, tags, snapshot registration, or typesafe handles; not for architecture rationale."
---

## Overview

Every CkFoundation type is assembled from `CK_` macros: they generate accessors, constructors,
typed handles, signals, and self-registration, and they encode the framework's contracts in their
expansions. This skill gives you each macro's actual expansion, the generated members, the
constraints **with the mechanical reason**, one canonical call site, and file-by-file checklists
for the six "add a new X" rituals. Style/naming rules are owned by the root doctrine
(`Plugins/CkFoundation/CLAUDE.md`) — this skill only explains the machinery behind them.

All file paths below are relative to `Plugins/CkFoundation/Source/`. All line numbers and counts
verified against source 2026-07-02 (re-verification commands at the bottom).

## When NOT to use this skill

| You actually want | Load instead |
|---|---|
| WHY the architecture is shaped this way (handles, deferral, lifecycle invariants) | `ckecs-architecture-contract` |
| How AS bindings work end-to-end, exposing/verifying script APIs, dynamic handles | `ck-angelscript-interop` |
| EnTT theory, entity↔actor lifetime, GC interaction | `ckecs-domain-reference` |
| A build/UHT/linker error that isn't macro-shaped | `ck-debugging-playbook` |
| Which module a feature belongs in | `Source/CLAUDE.md` (topology) |
| To USE an existing feature (add a Health attribute, a timer, …) — not author a new one | `Source/CLAUDE.md` decision tree (+ `Script/CLAUDE.md` §5 for AS); no skill needed |

## 1. Census and family map

**273 `#define CK_*` definition sites, 247 unique macro names** (as of 2026-07-02; the delta is
macros redefined across `#if` branches — `CK_ENSURE_IF_NOT` ×3, AS no-op twins, etc.). Verify:

```powershell
# cwd = d:\Repos\BusterBlock (PowerShell or Git Bash)
rg -c '#define CK_' Plugins/CkFoundation/Source --glob '*.{h,inl,hpp}'   # sum ≈ 273
```

Two non-`CK_` framework macros also live in `CkCore/Public/CkCore/Macros/CkMacros.h`:
**`NOT`** (= `!`, :31 — house style for all logical negation) and **`COMMA`** (= `,`, :32 — passes
commas through macro arguments, e.g. `CK_GENERATED_BODY(TFoo<A COMMA B>)`).

| Family | Representative macros | Defined in | You touch it when… |
|---|---|---|---|
| Identity / class body | `CK_GENERATED_BODY`, `CK_ENABLE_CUSTOM_VALIDATION`, `CK_ENABLE_SFINAE_THIS` | `CkCore/Public/CkCore/Macros/CkMacros.h:63-69,268` | every Ck struct/class |
| Property accessors | `CK_PROPERTY`, `CK_PROPERTY_GET` (+`_BY_COPY`,`_NON_CONST`,`_PASSTHROUGH`,`_STATIC`), `CK_PROPERTY_SET`, `CK_PROPERTY_UPDATE` | CkMacros.h:72-145 | every data member |
| Constructors | `CK_DEFINE_CONSTRUCTORS`, `CK_DEFINE_CONSTRUCTOR_1..9`, `CK_USING_BASE_CONSTRUCTORS` | CkMacros.h:160-217 | every params/request struct |
| Operators | `CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_T)`, `CK_DECL_AND_DEF_OPERATORS(_T)`, assignment-op sets | CkMacros.h:221-263 | comparable value types |
| Ensure (runtime validation) | `CK_ENSURE`, `CK_ENSURE_IF_NOT`, `CK_TRIGGER_ENSURE(_IF)`, `CK_INVALID_ENUM`, `CK_ENSURE_VALID_IF_NOT(_MSG)`, `CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT`, `CK_PURE_VIRTUAL` | `CkCore/Public/CkCore/Ensure/CkEnsure.h:36-85`; CkMacros.h:300-306 | every precondition |
| IsValid plumbing | `CK_DEFINE_CUSTOM_IS_VALID_INLINE`, `CK_DECLARE/DEFINE_CUSTOM_IS_VALID*`, `CK_DEFINE_CUSTOM_IS_VALID_POLICY`, `CK_DELETE_CUSTOM_IS_VALID` | `CkCore/Public/CkCore/Validation/CkIsValid.h:24-235` | new validatable type |
| Formatters | `CK_DEFINE_CUSTOM_FORMATTER_ENUM`, `CK_DECLARE/DEFINE_CUSTOM_FORMATTER*` | `CkCore/Public/CkCore/Format/CkFormat.h:133-302` | every UENUM; loggable types |
| Signals | `CK_DEFINE_SIGNAL_AND_UTILS(_WITH_DELEGATE)`, `CK_SIGNAL_BIND(_PROMISE/_REQUEST_FULFILLED/_WITH_CONDITION)`, `CK_SIGNAL_UNBIND` | `CkEcs/Public/CkEcs/Signal/CkSignal_Macros.h:10-64` | events |
| Requests | `CK_REQUEST_DEFINE_DEBUG_NAME` | `CkEcs/Public/CkEcs/Request/CkRequest_Data.h:111-121` | every request struct |
| Typesafe handles | `CK_GENERATED_BODY_HANDLE_TYPESAFE(_DERIVED)`, `CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE`, `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE`, `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` | `CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h:84-242` | new feature handle |
| Lifetime view filters | `CK_IGNORE_PENDING_KILL`, `CK_IF_END_PLAY`, `CK_IF_TEARING_DOWN`; `CK_IF_HANDLE_IS_PENDING_KILL` | `CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h:37-50`; `CkEcs/Public/CkEcs/Handle/CkHandle.h:395-396` | every processor declaration |
| ECS tags | `CK_DEFINE_ECS_TAG`, `_TRANSIENT`, `_COUNTED` | `CkEcs/Public/CkEcs/Tag/CkTag.h:23-40` | lifecycle markers |
| Processor registration | `CK_REGISTER_PROCESSOR(_WITH_FACTORY)`, `CK_REGISTER_GROUP` | `CkEcs/Public/CkEcs/Scheduler/CkProcessorRegistration.h:85-95` | every processor .cpp |
| Snapshot | `CK_REGISTER_SNAPSHOTABLE`, `CK_SNAPSHOT_ANNOUNCE_EXPECTED` | `CkEcs/Public/CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h:138-152`, `CkSnapshot_Audit.h` | save/load participation |
| Records / EntityHolders | `CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP/_TRANSIENT`, `CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP/_TRANSIENT` (+`_AND_UTILS`) | `CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h:102-120`; `CkEcsExt/Public/CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h:61-79` | child-entity collections |
| Logging | `CK_DEFINE_LOG_FUNCTIONS`, `CK_REGISTER_LOG_FUNCTIONS`, `CK_LOG_ERROR(_NOTIFY)_IF_NOT` | `CkLog/Public/CkLog/CkLog_Utils.h:106,338,408,412` | new module log |
| Debug callstack | `CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR`, `CK_CALLSTACK_RECORD(_MSG)`, `CK_CALLSTACK_CLEAR` | `CkEcs/Public/CkEcs/Handle/CkDebugCallstack_Macros.h:20-59` | request-origin tracing |
| Profiling | `CK_DEFINE_STAT`, `CK_DEFINE_PHASE_STAT`, `CK_STAT` | `CkProfile/Public/CkProfile/Stats/CkStats.h:70-109` | → `ck-performance-and-analysis` |
| AS bridge (~90 macros) | `CK_ANGELSCRIPT_*` | `CkCore/Public/CkCore/Macros/CkMacros_AngelScript.h` | never directly — internals of the families above; → `ck-angelscript-interop` |
| Utility | `CK_CONCAT`, `CK_UNIQUE_NAME`, `EXPAND(_ALL)`, `NARG_`, `NOT`, `COMMA`, `CK_SCOPE_CALL` | CkMacros.h:19-59,280-285 | building other macros |

Call-site frequency (grep over `Source/*.{h,cpp,inl}`, 2026-07-02) — the top ones you must know
cold: `CK_PROPERTY_GET` 1707 · `CK_GENERATED_BODY` 1592 · `CK_ENSURE_IF_NOT` 1448 · `CK_PROPERTY`
1144 · `CK_DEFINE_CONSTRUCTORS` 541 · `CK_REGISTER_PROCESSOR` 388 registrations (raw grep 390:
+1 commented example, +1 `#define`) · `CK_DEFINE_ECS_TAG` 236 · `CK_REQUEST_DEFINE_DEBUG_NAME` 173 ·
`CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` 147 · `CK_SIGNAL_BIND` 131 ·
`CK_REGISTER_SNAPSHOTABLE` 121 · `CK_GENERATED_BODY_HANDLE_TYPESAFE` 91. These are RAW line
counts — they include each macro's own `#define` and commented examples; sibling skills quote
call-site-only figures (130 `CK_SIGNAL_BIND` binds, 119 snapshot registrations, 171 request
debug-names, 90 typesafe-handle declarations) — both derivations are correct.


## 2. Reference files — load only what the task needs

Section numbers cited elsewhere in this skill (`§2.7`, `§3.4`, …) point into these files.

| You need | Read |
|---|---|
| A specific macro's actual expansion, its generated members, and the constraint **with its mechanical reason** | `references/macro-expansions.md` |
| The file-by-file ritual for adding a new fragment, processor, typesafe handle, request, signal, or snapshotable type | `references/add-a-new-x.md` |

`references/macro-expansions.md` — 2.1 `CK_GENERATED_BODY` · 2.2 `CK_PROPERTY` family · 2.3 `CK_DEFINE_CONSTRUCTORS` · 2.4 Ensure family · 2.5 Signal macros · 2.6 Request macros · 2.7 Typesafe-handle macros · 2.8 Tag macros · 2.9 Record / EntityHolder (poisoned policy-blind forms) · 2.10 `CK_REGISTER_PROCESSOR` · 2.11 `CK_REGISTER_SNAPSHOTABLE` · 2.12 `CK_DEFINE_LOG_FUNCTIONS` · 2.13 `CK_DEFINE_CUSTOM_FORMATTER_ENUM` · 2.14 `NOT` and the `ck::IsValid` machinery

`references/add-a-new-x.md` — 3.1 new fragment (Spec + runtime fragments) · 3.2 new processor · 3.3 new typesafe handle · 3.4 new request (+ Utils UFUNCTION surface) · 3.5 new signal (+ BindTo wrappers) · 3.6 make a type snapshotable

## Common mistakes

1. **Member without leading `_`** → accessors come out `GetFoo` and the AS name-strip does
   nothing. Rename the member, not the macro.
2. **`CK_PROPERTY_UPDATE`/`CK_PROPERTY` before `CK_GENERATED_BODY`** → "ThisType: undeclared
   identifier". Move `CK_GENERATED_BODY` up.
3. **`CK_DEFINE_CONSTRUCTORS` on a UCLASS/AActor** → default-ctor redeclaration errors and (in AS
   builds) placement-new codegen on a UObject. Structs only (§2.3).
4. **Recovery block with side effects after `CK_ENSURE_IF_NOT`** → behavior differs across build
   configs (§2.4). Pure bail-out only.
5. **Assuming ensures vanish in Shipping** → they don't (CHECKS=0 in Shipping); only the
   diagnostics do. Don't write recovery blocks as "unreachable".
6. **`CK_REGISTER_SNAPSHOTABLE(ck::FFoo)`** → token-paste error on `::`. File-scope alias first.
7. **Copying `CK_DEFINE_RECORD_OF_ENTITIES(...)` from an old doc** → `static_assert(false)`
   tombstone. Pick `_ROUNDTRIP` or `_TRANSIENT` (§2.9).
8. **Hand-writing `BindTo_*` that forwards to `Utils::Bind` directly** → PostFireBehavior silently
   ignored (the Unbind flavor is a different generated class). Use `CK_SIGNAL_BIND` (§2.5).
9. **Trusting a green test from a binary older than your registrar edit** → global static
   registration means the old binary registered the old set. Rebuild, re-run (§2.11).
10. **memcpy/static_assert(sizeof) on request structs** → vptr comes and goes with
    `CK_DISABLE_ECS_HANDLE_DEBUGGING` (§2.6).
11. **Adding data members to a typed handle** → `static_assert(sizeof == sizeof(FCk_Handle))`
    fires (twice, deliberately). Typed handles are views, never containers (§2.7).
12. **New UENUM without `CK_DEFINE_CUSTOM_FORMATTER_ENUM`** → first `{}`-format of it fails to
    compile. Add the macro right below the enum.
13. **Typed handle declared in `_Fragment.h`** → breaks the UHT-facing/runtime split. It goes in
    `_Fragment_Data.h` (72/74 files comply; §2.7).

## Provenance and maintenance

Campaign 2026-07-02. Everything above was read from source that day (engine:
UnrealEngine-Angelscript 5.7.4; EnTT 3.16.0 — per root CLAUDE.md). Macro definitions move;
re-verify before trusting long after that date. From `d:\Repos\BusterBlock` (PowerShell or
Git Bash; the Grep tool is fine for `Source/` but use `rg --no-ignore` under `Script/`):

- Census: `rg -c '#define CK_' Plugins/CkFoundation/Source --glob '*.{h,inl,hpp}'` (sum ≈ 273);
  unique names: `rg -o '#define (CK_[A-Za-z0-9_]+)' -r '$1' Plugins/CkFoundation/Source --glob '*.{h,inl,hpp}' --no-filename | sort -u | wc -l` (≈ 247).
- Any macro's current definition: `rg -n 'define <NAME>' Plugins/CkFoundation/Source -A5`.
- Ensure config matrix: `rg -n 'CK_DISABLE_ENSURE' Plugins/CkFoundation/Source/CkBuildConfig/CkBuildConfig.Build.cs`.
- Call-site frequencies: `rg -o '<MACRO>\(' Plugins/CkFoundation/Source --glob '*.{h,cpp,inl}' --no-filename | wc -l`.
- Typed-handle placement: `rg -ln 'CK_GENERATED_BODY_HANDLE_TYPESAFE' Plugins/CkFoundation/Source | rg -v '_Fragment_Data\.h'` (expect only the base header + CkShape_Handle.h).
- Binding-policy enumerators: `rg -n 'enum class ECk_Signal_BindingPolicy' -A10 Plugins/CkFoundation/Source/CkEcs`.
- Tombstones still poisoned: `rg -n 'static_assert\(false' Plugins/CkFoundation/Source/CkRecord Plugins/CkFoundation/Source/CkEcsExt`.
- CkTimer wrapper divergence (drop this skill's warning if fixed):
  `rg -c 'CK_SIGNAL_BIND|_PostFireUnbind' Plugins/CkFoundation/Source/CkTimer/Public/CkTimer/CkTimer_Utils.cpp` (0 hits = still divergent).
- Stale READMEs called out here: `rg -n 'ensureAlwaysMsgf|CkBuild_Macros' Plugins/CkFoundation/Source/CkCore/Public/CkCore/Ensure/README.md` (hits = still stale).
