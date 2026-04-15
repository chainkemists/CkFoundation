# CkCore / Ensure

Assertion / precondition macros with formatted diagnostics, per-site silencing, and a code-vs-script break policy. Use these **instead of** UE's `ensure`/`ensureMsgf`/`check` in CkFoundation code.

**Key files:** `CkEnsure.h`, `CkEnsure_Subsystem.h`, `CkEnsure_Utils.h`, `CkEnsure_Log.h`

## Public macros

### Primary

```cpp
CK_ENSURE(InExpression, InFormat, ...)            // returns bool; true if expression passed
CK_ENSURE_IF_NOT(InExpression, InFormat, ...)     // use inside control flow
```

`CK_ENSURE_IF_NOT` is the one you'll call 99% of the time. It evaluates `InExpression`; on failure it calls `ck::ensure::Ensure_Impl`, which talks to the `CkEnsure_Subsystem` to decide whether to break in code, break in script, log, or silence the site. The macro is `if (NOT CK_ENSURE(...))`, so pair it with an early-return block:

```cpp
CK_ENSURE_IF_NOT(ck::IsValid(Handle), TEXT("Invalid handle in function [{}]"), __FUNCTION__)
{ return; }
```

### Targeted triggers

- `CK_TRIGGER_ENSURE(InString, ...)` — unconditional ensure fire with a formatted message. Semantically "we reached an impossible branch."
- `CK_TRIGGER_ENSURE_IF(InExpression, InString, ...)` — fires the ensure **only** when expression is true. Use when the triggering condition isn't easy to phrase as the ensure's own predicate.
- `CK_INVALID_ENUM(InEnum)` — shorthand for "we hit a switch-case default that shouldn't be reachable."

### Shortcuts over `ck::IsValid`

- `CK_ENSURE_VALID_IF_NOT(_ToValidate_)` — equivalent to `CK_ENSURE_IF_NOT(ck::IsValid(_ToValidate_), TEXT("X is INVALID"))`.
- `CK_ENSURE_VALID_IF_NOT_MSG(_ToValidate_, InFormat, ...)` — same, with an extra message appended.
- `CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(InWorldContextObject)` — ensures the world-context object and its `UWorld` are both valid. Use at the top of any function that needs a `UWorld*`.

## Build-time control

Two compile-time switches gate the behavior (declared in `CkCore/Build/CkBuild_Macros.h`):

| Flag | Effect on `CK_ENSURE_IF_NOT` |
|---|---|
| `CK_DISABLE_ENSURE_CHECKS` = 1 | Expands to `if constexpr (false)`. Block body is compiled out. |
| `CK_DISABLE_ENSURE_DEBUGGING` = 1 | Expands to `if (NOT expr)`. Still branches on failure, but skips the subsystem call. |
| both 0 | Full behavior: subsystem + debug break. |

Set these in your target's `.Target.cs` for shipping / test builds.

## Behavior

When a `CK_ENSURE` fails:

1. Message is built via `ck::Format_UE(InString, ...)`.
2. `ck::ensure::Ensure_Impl` runs, which:
   - logs through the Ensure log category,
   - consults `UCk_Ensure_Subsystem` for per-site silencing (a site can be muted at runtime after first fire),
   - decides `ShouldBreakInCode` / `ShouldBreakInScript`.
3. If `ShouldBreakInCode`, UE's `ensureAlwaysMsgf(false, …)` fires so debuggers break at the call site.
4. If `ShouldBreakInScript` **and** we're inside an AngelScript call, `ck::ensure::Do_BreakInScript()` routes the break to the AS debugger.

Scripts that fire an ensure from AS can mark themselves with `ck::ensure::Do_Push_EnsureIsFromScript()` / `Do_Pop_EnsureIsFromScript()` so the subsystem attributes the fire correctly.

## Worked example

```cpp
auto
    UCk_Utils_Foo_UE::
    Request_DoThing(
        FCk_Handle_Foo& InHandle,
        const FCk_Request_Foo_DoThing& InRequest)
    -> FCk_Handle_Foo
{
    CK_ENSURE_VALID_IF_NOT(InHandle)
    { return InHandle; }

    CK_ENSURE_IF_NOT(InRequest.Get_Amount() > 0,
        TEXT("Request [{}] has non-positive Amount [{}]"),
        InRequest, InRequest.Get_Amount())
    { return InHandle; }

    // ... main logic ...
    return InHandle;
}
```

## Pitfalls

1. Don't call `ensure()` or `check()` directly in CkFoundation code. You lose per-site silencing, logging, and AS integration.
2. `CK_ENSURE` evaluates `InExpression` **once** (via `const auto ExpressionResult = InExpression;`), so side-effects in the expression are safe — but keep them out anyway.
3. Pass a **format string** (fmt syntax: `{}`), not `TEXT("%s")`. The macro runs `ck::Format_UE`, which is fmt-based, not printf-based.
4. `CK_ENSURE_IF_NOT` is not a statement — it's the head of an `if`. You must follow it with a `{ ... }` block.
5. The block body runs only on failure; don't put success-path code in it.

## Depends on
`Macros/`, `Format/` (for message formatting), `Build/`, `Validation/` (for the `CK_ENSURE_VALID_*` macros).

## Used by
Every module. `_Utils.h` entry points typically start with a `CK_ENSURE_VALID_IF_NOT` or `CK_ENSURE_IF_NOT` block.

## See also
- `Validation/README.md` — the `ck::IsValid` predicate most ensures run against.
- `Build/CkBuild_Macros.h` — the flags that gate this whole system.
- `CkEnsure_Subsystem.h` — the runtime subsystem that tracks per-site mute state.
