#pragma once

// Out-of-line body for FCk_PersistenceHandlerRegistry::RegisterLazyTyped<T>. Kept out-of-line (rather than in
// the registry header) so it instantiates in the feature registrar .cpp where T_RepData is a complete type. It only
// resolves the payload type lazily via T_RepData::StaticStruct() and forwards to RegisterLazy.

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

// ---- Named participation shapes — each forwards a fully-formed FHandler to RegisterLazyTyped ------------------------
// FHandler is an aggregate; designated initializers list members in declaration order (NetApply, NetRemove,
// HydrationApply, Produce). Omitted slots value-initialize to an empty TFunction (== "not participating on that path").

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetOnly(FApplyFn InNetApply, FRemoveFn InNetRemove)
    -> void
{
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply  = MoveTemp(InNetApply),
        .NetRemove = MoveTemp(InNetRemove)});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_SaveOnly(FProduceFn InProduce, FApplyFn InHydrationApply)
    -> void
{
    RegisterLazyTyped<T_RepData>(FHandler{
        .HydrationApply = MoveTemp(InHydrationApply),
        .Produce        = MoveTemp(InProduce)});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetAndSave_SharedApply(FProduceFn InProduce, FApplyFn InSharedApply, FRemoveFn InNetRemove)
    -> void
{
    // One authority-safe applier serves both the net-receive path and the load-path (Team/Player/Velocity shape).
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply       = InSharedApply,
        .NetRemove      = MoveTemp(InNetRemove),
        .HydrationApply = MoveTemp(InSharedApply),
        .Produce        = MoveTemp(InProduce)});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetAndSave_SplitApply(FProduceFn InProduce, FApplyFn InNetApply, FApplyFn InHydrationApply, FRemoveFn InNetRemove)
    -> void
{
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply       = MoveTemp(InNetApply),
        .NetRemove      = MoveTemp(InNetRemove),
        .HydrationApply = MoveTemp(InHydrationApply),
        .Produce        = MoveTemp(InProduce)});
}
