#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

TMap<const UScriptStruct*, FCk_LiveTuneHandlerRegistry::FHandler>
    FCk_LiveTuneHandlerRegistry::_Handlers;

TArray<FCk_LiveTuneHandlerRegistry::FLazyEntry>
    FCk_LiveTuneHandlerRegistry::_PendingHandlers;

auto
    FCk_LiveTuneHandlerRegistry::
    RegisterLazy(
        FTypeResolver InTypeResolver,
        FHandler InHandler)
    -> void
{
    _PendingHandlers.Add({MoveTemp(InTypeResolver), MoveTemp(InHandler)});
}

auto
    FCk_LiveTuneHandlerRegistry::
    ResolvePending()
    -> void
{
    auto Pending = MoveTemp(_PendingHandlers);
    _PendingHandlers.Empty();

    for (auto& Entry : Pending)
    {
        if (const auto* Type = Entry.TypeResolver())
        {
            _Handlers.Add(Type, MoveTemp(Entry.Handler));
        }
    }
}

auto
    FCk_LiveTuneHandlerRegistry::
    Find(
        const UScriptStruct* InType)
    -> const FHandler*
{
    if (_PendingHandlers.Num() > 0)
    { ResolvePending(); }

    return _Handlers.Find(InType);
}

#endif

// --------------------------------------------------------------------------------------------------------------------
