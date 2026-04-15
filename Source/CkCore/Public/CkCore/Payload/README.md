# CkCore / Payload

Variadic tuple wrapper for packing heterogeneous argument lists. Used as the transport type for signal parameters, deferred calls, and generic dispatch sites where `std::tuple` would be awkward to spell.

**Key file:** `CkPayload.h`

## Public API

```cpp
namespace ck
{
    template <typename... T_Args>
    struct TPayload
    {
        std::tuple<T_Args...> Payload;
    };

    template<typename... T_Args>
    auto MakePayload(T_Args&&... InArgs)
    {
        return TPayload<T_Args...>{ { std::forward<T_Args>(InArgs)... } };
    }
}
```

## Usage

```cpp
auto P = ck::MakePayload(42, FString(TEXT("hello")), SomeHandle);
// P is TPayload<int, FString, FCk_Handle_X>
```

Signal macros (`CK_SIGNAL_BIND` / `CK_SIGNAL_UNBIND` / `UUtils_Signal_*::Broadcast`) use `TPayload` under the hood — you won't usually write `MakePayload` directly, but knowing the type helps when reading templated signal infrastructure.

## Why not just `std::tuple`?

Named type. Makes overload resolution unambiguous when a signal system wants to distinguish "a single tuple argument" from "a payload of arguments."

## Depends on
`<tuple>`. No Ck deps.

## Used by
`CkEcs`'s signal system, any code that needs to bundle variadic args for later invocation.

## See also
- `CkEcs` signal macros (`CK_SIGNAL_BIND` and friends) — the primary consumer.
