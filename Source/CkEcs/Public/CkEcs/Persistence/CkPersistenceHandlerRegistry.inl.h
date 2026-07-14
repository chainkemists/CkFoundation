#pragma once

// Out-of-line bodies for FCk_PersistenceHandlerRegistry's template registration methods. Kept out-of-line (rather
// than in the registry header) so they instantiate in the feature registrar .cpp where T_RepData is a complete type.
// Include this alongside CkPersistenceHandlerRegistry.h from a feature's registrar .cpp.

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    RegisterLazyTyped(
        FHandler InHandler)
    -> void
{
    RegisterLazy(
        []() -> UScriptStruct* { return T_RepData::StaticStruct(); },
        MoveTemp(InHandler));
}

// ---- Named participation shapes — each unwraps its designated-init args and forwards a fully-formed FHandler to
//      RegisterLazyTyped. FHandler is an aggregate; designated initializers list members in declaration order
//      (NetApply, NetRemove, HydrationApply, Produce). Omitted slots value-initialize to an empty TFunction
//      (== "not participating on that path").

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetOnly(FArgs_NetOnly InArgs)
    -> void
{
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply  = MoveTemp(InArgs.NetApply.Value),
        .NetRemove = MoveTemp(InArgs.NetRemove)});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_SaveOnly(FArgs_SaveOnly InArgs)
    -> void
{
    RegisterLazyTyped<T_RepData>(FHandler{
        .HydrationApply = MoveTemp(InArgs.HydrationApply.Value),
        .Produce        = MoveTemp(InArgs.Produce.Value)});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetAndSave_SharedApply(FArgs_NetAndSave_SharedApply InArgs)
    -> void
{
    // One authority-safe applier serves both the net-receive path and the load-path (Team/Player/Velocity shape).
    // Copy the shared lambda into NetApply (evaluated first, declaration order), then move it into HydrationApply.
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply       = InArgs.SharedApply.Value,
        .NetRemove      = MoveTemp(InArgs.NetRemove),
        .HydrationApply = MoveTemp(InArgs.SharedApply.Value),
        .Produce        = MoveTemp(InArgs.Produce.Value)});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetAndSave_SplitApply(FArgs_NetAndSave_SplitApply InArgs)
    -> void
{
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply       = MoveTemp(InArgs.NetApply.Value),
        .NetRemove      = MoveTemp(InArgs.NetRemove),
        .HydrationApply = MoveTemp(InArgs.HydrationApply.Value),
        .Produce        = MoveTemp(InArgs.Produce.Value)});
}
