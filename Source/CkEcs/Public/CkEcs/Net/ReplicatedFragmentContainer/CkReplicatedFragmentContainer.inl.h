#pragma once

// Out-of-line body for FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<T>.
//
// This file has NO includes on purpose: it must be included in a feature registrar .cpp AFTER CkNet_Utils.h, where
// both FCk_ReplicatedFragmentHandlerRegistry (from CkReplicatedFragmentContainer.h) and
// UCk_Utils_Net_UE::TryAddContainerFragment are fully declared. It cannot ride at the bottom of CkNet_Utils.h
// (that is a UHT-reflected header: UHT forbids any #include after its .generated.h), and the registry header
// cannot include CkNet_Utils.h (that header includes the registry header — the reverse edge is a cycle). So each
// migrated registrar .cpp includes CkNet_Utils.h then this .inl.h.

template <typename T_RepData>
auto
    FCk_ReplicatedFragmentHandlerRegistry::
    RegisterLazyTyped(
        FHandler InHandler)
    -> void
{
    // Default typed seed: re-establish the FastArray entry + the entity-side TFragment_ContainerEntryRef<T>.
    // A registrar that needs re-arm work beyond the plain add supplies its own SeedContainer (do the typed add
    // there, then the re-arm) — honored here by only synthesizing when unset.
    if (NOT InHandler.SeedContainer)
    {
        InHandler.SeedContainer = [](FCk_Handle& InEntity, const FInstancedStruct& InData) -> ECk_AddedOrNot
        {
            return UCk_Utils_Net_UE::TryAddContainerFragment<T_RepData>(InEntity, InData.Get<T_RepData>());
        };
    }

    RegisterLazy(
        []() -> UScriptStruct* { return T_RepData::StaticStruct(); },
        MoveTemp(InHandler));
}
