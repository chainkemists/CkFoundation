#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"

#include "CkCore/Ensure/CkEnsure.h" // registration-time Produce-without-HydrationApply enforcement

// --------------------------------------------------------------------------------------------------------------------
// Handler Registry

TMap<const UScriptStruct*, FCk_PersistenceHandlerRegistry::FHandler>
    FCk_PersistenceHandlerRegistry::_Handlers;

TArray<FCk_PersistenceHandlerRegistry::FLazyEntry>
    FCk_PersistenceHandlerRegistry::_PendingHandlers;

TOptional<FCk_PersistenceHandlerRegistry::FHandler>
    FCk_PersistenceHandlerRegistry::_Fallback;

auto
    FCk_PersistenceHandlerRegistry::
    Register(
        const UScriptStruct* InType,
        FHandler InHandler)
    -> void
{
    CK_ENSURE_IF_NOT(NOT InHandler.Produce || static_cast<bool>(InHandler.HydrationApply),
        TEXT("Persistence handler for [{}] has Produce but no HydrationApply — its state would SAVE but never LOAD. "
             "Assign HydrationApply (reuse the NetApply lambda if the net path is authority-safe)."),
        ck::IsValid(InType) ? InType->GetName() : FString{TEXT("<null type>")})
    { /* register anyway; Get_SaveHandlerTypes excludes it, so the misconfig is loud, not corrupting */ }

    _Handlers.Add(InType, MoveTemp(InHandler));
}

auto
    FCk_PersistenceHandlerRegistry::
    RegisterLazy(
        FTypeResolver InTypeResolver,
        FHandler InHandler)
    -> void
{
    _PendingHandlers.Add({MoveTemp(InTypeResolver), MoveTemp(InHandler)});
}

auto
    FCk_PersistenceHandlerRegistry::
    ResolvePending()
    -> void
{
    auto Pending = MoveTemp(_PendingHandlers);
    _PendingHandlers.Empty();

    for (auto& Entry : Pending)
    {
        if (const auto* Type = Entry.TypeResolver())
        {
            CK_ENSURE_IF_NOT(NOT Entry.Handler.Produce || static_cast<bool>(Entry.Handler.HydrationApply),
                TEXT("Persistence handler for [{}] has Produce but no HydrationApply — its state would SAVE but never "
                     "LOAD. Assign HydrationApply (reuse the NetApply lambda if the net path is authority-safe)."),
                Type->GetName())
            { /* register anyway; Get_SaveHandlerTypes excludes it, so the misconfig is loud, not corrupting */ }

            _Handlers.Add(Type, MoveTemp(Entry.Handler));
        }
    }
}

auto
    FCk_PersistenceHandlerRegistry::
    RegisterFallback(
        FHandler InHandler)
    -> void
{
    _Fallback = MoveTemp(InHandler);
}

auto
    FCk_PersistenceHandlerRegistry::
    Get_SaveHandlerTypes()
    -> TArray<const UScriptStruct*>
{
    if (_PendingHandlers.Num() > 0)
    { ResolvePending(); }

    auto Types = TArray<const UScriptStruct*>{};
    for (const auto& Pair : _Handlers)
    {
        // Save-participating handlers only: a Produce (the payload emitter) paired with a HydrationApply (its
        // load-path applier). The v3 capture writes one payload per (entity, type) for these. A Produce with no
        // HydrationApply is excluded — the registration ensure already fired for it; excluding it keeps the save
        // free of state that could never load back.
        if (Pair.Value.Produce && Pair.Value.HydrationApply)
        { Types.Add(Pair.Key); }
    }

    // Deterministic save files + deterministic per-entity hydration order (payload apply order follows the sorted
    // type-path order). TArray::Sort dereferences the pointers before calling the predicate.
    Types.Sort([](const UScriptStruct& InA, const UScriptStruct& InB)
    { return InA.GetPathName() < InB.GetPathName(); });

    return Types;
}

auto
    FCk_PersistenceHandlerRegistry::
    Find(
        const UScriptStruct* InType)
    -> const FHandler*
{
    if (_PendingHandlers.Num() > 0)
    {
        ResolvePending();
    }

    return _Handlers.Find(InType);
}

auto
    FCk_PersistenceHandlerRegistry::
    Resolve(
        const UScriptStruct* InType)
    -> const FHandler*
{
    if (const auto* Handler = Find(InType))
    { return Handler; }

    return _Fallback.IsSet() ? &_Fallback.GetValue() : nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
