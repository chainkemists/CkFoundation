#pragma once

// Out-of-line body for FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<T>. Kept out-of-line (rather than in
// the registry header) so it instantiates in the feature registrar .cpp where T_RepData is a complete type. It only
// resolves the payload type lazily via T_RepData::StaticStruct() and forwards to RegisterLazy.

template <typename T_RepData>
auto
    FCk_ReplicatedFragmentHandlerRegistry::
    RegisterLazyTyped(
        FHandler InHandler)
    -> void
{
    RegisterLazy(
        []() -> UScriptStruct* { return T_RepData::StaticStruct(); },
        MoveTemp(InHandler));
}
