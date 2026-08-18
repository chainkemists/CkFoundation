#pragma once

// Out-of-line bodies for FCk_PersistenceHandlerRegistry's template registration methods — kept out of the registry
// header so they instantiate in the feature registrar .cpp, where T_RepData is a complete type. Include both there.

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"

#include "CkCore/Ensure/CkEnsure.h"

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
// declaration order: NetApply, NetRemove, HydrationApply, Produce, Posture). Omitted slots become empty
// TFunctions. Each shape also ensures the DECLARED posture is one the shape can actually deliver: only the
// registration site knows the intent, and a shape that contradicts it is an authoring error, not a preference.

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetOnly(FArgs_NetOnly InArgs)
    -> void
{
    const auto PostureIsLegal = InArgs.Posture.Value == ECk_Snapshot_Posture::Session;
    CK_ENSURE_IF_NOT(PostureIsLegal,
        TEXT("Wire-only registration for [{}] declares posture [{}]. A payload that never enters the save file "
            "is Session by construction — there is nothing for a load to restore."),
        T_RepData::StaticStruct(), InArgs.Posture.Value)
    { return; }

    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply  = MoveTemp(InArgs.NetApply.Value),
        .NetRemove = MoveTemp(InArgs.NetRemove),
        .Posture   = InArgs.Posture.Value});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_SaveOnly(FArgs_SaveOnly InArgs)
    -> void
{
    const auto PostureIsLegal = InArgs.Posture.Value == ECk_Snapshot_Posture::Durable;
    CK_ENSURE_IF_NOT(PostureIsLegal,
        TEXT("Save-participating registration for [{}] declares posture [{}]. A payload with a Produce and a "
            "HydrationApply IS part of the saved world — declare it Durable, or stop registering it."),
        T_RepData::StaticStruct(), InArgs.Posture.Value)
    { return; }

    RegisterLazyTyped<T_RepData>(FHandler{
        .HydrationApply = MoveTemp(InArgs.HydrationApply.Value),
        .Produce        = MoveTemp(InArgs.Produce.Value),
        .Posture        = InArgs.Posture.Value});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetAndSave_SharedApply(FArgs_NetAndSave_SharedApply InArgs)
    -> void
{
    const auto PostureIsLegal = InArgs.Posture.Value == ECk_Snapshot_Posture::Durable;
    CK_ENSURE_IF_NOT(PostureIsLegal,
        TEXT("Save-participating registration for [{}] declares posture [{}]. A payload with a Produce and a "
            "HydrationApply IS part of the saved world — declare it Durable, or stop registering it."),
        T_RepData::StaticStruct(), InArgs.Posture.Value)
    { return; }

    // COPY the shared lambda into NetApply (initialized first, declaration order), then MOVE it into HydrationApply.
    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply       = InArgs.SharedApply.Value,
        .NetRemove      = MoveTemp(InArgs.NetRemove),
        .HydrationApply = MoveTemp(InArgs.SharedApply.Value),
        .Produce        = MoveTemp(InArgs.Produce.Value),
        .Posture        = InArgs.Posture.Value});
}

template <typename T_RepData>
auto
    FCk_PersistenceHandlerRegistry::
    Register_NetAndSave_SplitApply(FArgs_NetAndSave_SplitApply InArgs)
    -> void
{
    const auto PostureIsLegal = InArgs.Posture.Value == ECk_Snapshot_Posture::Durable;
    CK_ENSURE_IF_NOT(PostureIsLegal,
        TEXT("Save-participating registration for [{}] declares posture [{}]. A payload with a Produce and a "
            "HydrationApply IS part of the saved world — declare it Durable, or stop registering it."),
        T_RepData::StaticStruct(), InArgs.Posture.Value)
    { return; }

    RegisterLazyTyped<T_RepData>(FHandler{
        .NetApply       = MoveTemp(InArgs.NetApply.Value),
        .NetRemove      = MoveTemp(InArgs.NetRemove),
        .HydrationApply = MoveTemp(InArgs.HydrationApply.Value),
        .Produce        = MoveTemp(InArgs.Produce.Value),
        .Posture        = InArgs.Posture.Value});
}
