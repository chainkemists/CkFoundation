# CkCore / Validation

The `ck::IsValid` system. Extensible, policy-based validity checking that understands custom types, handles, pointers, and containers. This is the correct way to check "is this usable" for any CkFoundation type.

**Key files:** `CkIsValid.h`, `CkIsValid_Defaults.h`, `CkIsValid_AngelScript.h`

## Public API

```cpp
namespace ck
{
    template <typename T>                          auto IsValid(T&& InObj) -> bool;
    template <typename T, typename T_Policy>       auto IsValid(T&& InObj, T_Policy InPolicy) -> bool;
    template <typename T>                          auto Is_NOT_Valid(T&& InObj) -> bool;
    template <typename T, typename T_Policy>       auto Is_NOT_Valid(T&& InObj, T_Policy InPolicy) -> bool;
}
```

Functors for `std::*`/ranges APIs:

```cpp
namespace ck::algo { struct IsValid { ... }; struct Is_NOT_Valid { ... }; }
```

## Rule: use `ck::IsValid`, not `::IsValid`

UE's `IsValid(UObject*)` only handles `UObject`. It **cannot** validate:

- CkFoundation handles (`FCk_Handle_*`)
- Custom structs with a custom validator
- `TSharedPtr`, `TWeakObjectPtr`, `TStrongObjectPtr` wrappers that ship custom checks
- Collections (arrays / sets / maps with custom `IsValid` policies)

`ck::IsValid` dispatches to `IsValid_Executor<T, T_Policy>` and walks base-class validators via `IsValid_Executor_IsBaseOf`. Result: one call site, correct answer for any registered type.

## Policies

A policy is just a tag type inheriting `ck::IsValid_Policy`. Default is `ck::IsValid_Policy_Default`.

Common project policies (discover by grepping `CK_DEFINE_CUSTOM_IS_VALID_POLICY`):

- `IsValid_Policy_Default` — canonical "is this usable" check.
- `IsValid_Policy_NullptrOnly{}` — raw-pointer style, skips deeper checks. Useful when you only want to guard against `nullptr` (e.g., on a `UEdGraphPin*`).

Call site:

```cpp
if (ck::IsValid(Pin, ck::IsValid_Policy_NullptrOnly{})) { /* only nullptr check */ }
if (ck::IsValid(Handle))                               { /* full default validator */ }
```

## Defining a custom validator

Four macros, pick based on scope:

| Macro | Use when |
|---|---|
| `CK_DEFINE_CUSTOM_IS_VALID_INLINE(_type_, _policy_, _lambda_)` | Validator lives entirely in a header. |
| `CK_DECLARE_CUSTOM_IS_VALID(_api_name_, _type_, _policy_)` + `CK_DEFINE_CUSTOM_IS_VALID(_type_, _policy_, _lambda_)` | Validator split across .h / .cpp. Prefer this when body is non-trivial. |
| `CK_DECLARE_CUSTOM_IS_VALID_PTR(_api_name_, _type_, _policy_)` + `CK_DEFINE_CUSTOM_IS_VALID_PTR(_type_, _policy_, _lambda_)` | Validator for `T*`. |
| `CK_DECLARE_CUSTOM_IS_VALID_NAMESPACE(_api_name_, _namespace_, _type_, _policy_)` | Validator for a namespaced type. |

Template-friendly variants: `CK_DEFINE_CUSTOM_IS_VALID_T`, `CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T`.

Opt-out: `CK_DELETE_CUSTOM_IS_VALID(_type_)` emits a static-assert if anyone tries to validate that type — use it for "this type should never go through IsValid."

Example (from `CkIsValid_Defaults.*`):

```cpp
CK_DEFINE_CUSTOM_IS_VALID_INLINE(FSoftObjectPath, IsValid_Policy_Default,
    [](const FSoftObjectPath& InPath) { return NOT InPath.IsNull(); });
```

## Custom-validator access to private state

The `CK_GENERATED_BODY(MyType)` macro (see `Macros/README.md`) expands to include:

```cpp
template <typename T, typename T_Policy, typename> friend class ck::IsValid_Executor;
```

So a validator defined alongside the type can read private members. The standalone macro `CK_ENABLE_CUSTOM_VALIDATION()` provides the same friendship if you don't want the rest of `CK_GENERATED_BODY`.

## AngelScript

`CK_DEFINE_CUSTOM_IS_VALID` also emits AS bindings (`CK_DEFINE_ANGELSCRIPT_IS_VALID`). Only the default-policy overload is exported to AS — AS doesn't see named policies.

## Pitfalls

1. Calling `ck::IsValid` on a bare `UObject*` falls back to a default executor; if you want UE's `UObject` liveness semantics, make sure `CkIsValid_Defaults.cpp` has the right specialization (it does for `UObject*`).
2. Policies are **tag types**, not enums. Pass them as instances: `ck::IsValid(X, MyPolicy{})`, not `ck::IsValid(X, MyPolicy)`.
3. `CK_DELETE_CUSTOM_IS_VALID` triggers a `static_assert` at the call site of `ck::IsValid<ThatType>`. This is intentional — read the error carefully; it says "IsValid for X is explicitly deleted."

## Depends on
`Macros/`, `TypeTraits/`. No further Ck deps.

## Used by
Every module. `Ensure/CkEnsure.h` builds `CK_ENSURE_VALID_IF_NOT` on top of `ck::IsValid`. ECS `Utils` files validate handles on every entry point.

## See also
- `Ensure/README.md` — pairs validity checks with diagnostic early-outs.
- `CkIsValid_Defaults.h/.cpp` — the canonical list of built-in validators. Read before defining new ones; the built-in may already cover you.
