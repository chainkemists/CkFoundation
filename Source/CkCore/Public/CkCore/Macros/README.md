# CkCore / Macros

Foundation preprocessor macros used everywhere in CkFoundation. Every header and nearly every source file relies on something declared here (directly or transitively).

**Key file:** `CkMacros.h`

## Most-used macros

### Code-generation

- `CK_GENERATED_BODY(_InClass_)` — stamps `using ThisType = _InClass_;` and grants `ck::IsValid_Executor` friend access. Put at the top of every `CK_PROPERTY_GET`-using struct/class.
- `CK_PROPERTY_GET(_InVar_)` — generates `const auto& Get_InVar() const`.
- `CK_PROPERTY_GET_BY_COPY(_InVar_)` — getter returning by value. Use for primitives where indirection isn't wanted.
- `CK_PROPERTY_GET_NON_CONST(_InVar_)` — non-const overload (friend-only access pattern; use sparingly).
- `CK_PROPERTY_GET_STATIC(_InVar_)` — static getter.
- `CK_PROPERTY_SET(_InVar_)` — generates `Set_InVar(const T&) -> ThisType&`.
- `CK_PROPERTY_UPDATE(_InVar_)` — generates `Update_InVar(std::function<void(T&)>) -> ThisType&` for in-place mutation.
- `CK_PROPERTY(_InVar_)` — getter + setter + update in one.
- `CK_PROPERTY_AND_VAR(_Type_, _InVar_)` / `CK_PROPERTY_AND_VAR_GET(_Type_, _InVar_)` — declares the private member AND its accessors. Use for tiny POD wrappers where the variable and its interface are declared together.
- `CK_PROPERTY_GET_PASSTHROUGH(_InVar_, _Getter_)` — forwards a getter through a member.

### Constructors

- `CK_DEFINE_CONSTRUCTORS(_ClassType_, _1, _2, … _9)` — declares a defaulted no-arg ctor plus a positional ctor that takes N args and `std::move`-initializes members of the same name. Up to 9 args.
- `CK_DEFINE_CONSTRUCTOR(_ClassType_, _1, …)` — positional ctor only (no default ctor).
- `CK_USING_BASE_CONSTRUCTORS(_InObject_)` — `using Base::Base;`.

**Do not use** `CK_DEFINE_CONSTRUCTORS` on `UObject`-derived classes — UHT generates its own. Use it on `USTRUCT` / plain classes.

### Operators

- `CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_InObject_)` — `operator!=` from `operator==`.
- `CK_DECL_AND_DEF_OPERATORS(_InObject_)` — `operator!= > <= >=` from `operator==` + `operator<`.
- `CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(_InObject_)`, `CK_DECL_AND_DEF_MULTIPLY_DIVIDE_ASSIGNMENT_OPERATORS(_InObject_)`, `CK_DECL_AND_DEF_SHORTHAND_ASSIGNMENT_OPERATORS(_InObject_)` — compound assignments derived from the non-compound operator.
- `CK_USING_OPERATORS(_InObject_)` / `CK_USING_OPERATORS_EQUAL_NOT_EQUAL(_InObject_)` — `using Base::operator==` etc. for inheritance.
- Templated variants: `CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL_T(_Template_, _InObject_)`, `CK_DECL_AND_DEF_OPERATORS_T(...)`.

### Language sugar

- `NOT` (== `!`) — preferred spelling per project style.
- `COMMA` (== `,`) — for passing commas into macros.
- `CK_EMPTY` — empty-token placeholder.
- `EXPAND(x)`, `EXPAND_ALL(...)` — MSVC `__VA_ARGS__` workarounds.
- `CK_CONCAT(a,b)` — token paste.
- `CK_UNIQUE_NAME(prefix)` — `prefix##__LINE__`.
- `CK_ENABLE_SFINAE_THIS(_DerivedType_)` — injects a `This()` accessor for CRTP/static-polymorphism bases.
- `CK_INTENTIONALLY_EMPTY()` — marker for deliberately blank macro sites.
- `CK_SCOPE_CALL(_NestedCall_)` — lambda-wraps a statement so macros that expect an `if`/`return` context can be called inside a constructor body.

### Pure virtual

- `CK_PURE_VIRTUAL(func, ...)` — UE's `PURE_VIRTUAL` replacement. Fires `CK_TRIGGER_ENSURE` instead of crashing. Example:
  ```cpp
  virtual auto Get_Something() const -> int32 CK_PURE_VIRTUAL(Get_Something, return 0);
  ```

## Worked example

```cpp
USTRUCT(BlueprintType)
struct FCk_Request_Foo
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Request_Foo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Amount = 0;

public:
    CK_PROPERTY_GET(_Tag);       // Get_Tag() const
    CK_PROPERTY(_Amount);        // Get_Amount() + Set_Amount() + Update_Amount()

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Foo, _Tag);   // default + 1-arg ctor
};
```

## AngelScript coupling

When `WITH_ANGELSCRIPT_CK` is defined, property macros also emit AS registration (`CK_ANGELSCRIPT_PROPERTY_REGISTRATION_*`). The AS-aware variants are in `CkMacros_AngelScript.h`; they are no-ops outside AS.

## Depends on
`<functional>` (std). No Ck deps.

## Used by
Every CkFoundation module. The file `CkMacros.h` is pulled into almost every header directly or via PCH.

## See also
- `Validation/` — `CK_ENABLE_CUSTOM_VALIDATION` is re-declared here to avoid circular includes.
- `Ensure/` — `CK_ENSURE_*` macros consume `CK_TRIGGER_ENSURE` which `CK_PURE_VIRTUAL` uses.
- `/Source/CLAUDE.md` section "Function formatting standards" — formatting rules for declarations/definitions using these macros.
