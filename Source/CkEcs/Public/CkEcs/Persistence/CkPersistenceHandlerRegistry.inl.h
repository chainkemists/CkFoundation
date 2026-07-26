#pragma once

// Out-of-line bodies for FCk_PersistenceHandlerRegistry's template registration methods — kept out of the registry
// header so they instantiate in the feature registrar .cpp, where T_RepData is a complete type. Include both there.

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

// Each named shape unwraps its designated-init args into an FHandler (an aggregate — designators must follow
// declaration order: NetApply, NetRemove, HydrationApply, Produce). Omitted slots become empty TFunctions.

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
    // COPY the shared lambda into NetApply (initialized first, declaration order), then MOVE it into HydrationApply.
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
