#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Per-registry feature bit table; zero cost until Enable() connects the EnTT sinks. Registration is NOT in this
// module (CkEcs is feature-agnostic): consumers that link the fragment types call RegisterFlag<TFragment>(FeatureId)
// at startup, BEFORE Enable(). Design notes in CkEcs/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::debug_feature_flags
{
    static constexpr int32 MaxFlags = 64;

    // EnTT sinks cannot take capturing lambdas — each registered flag gets one heap-stable listener
    // instance (owned by the registry's ctx payload) whose bit index routes the callback.
    struct CKECS_API FBitListener
    {
        int32 _Bit = INDEX_NONE;

        auto OnAdded(registry_table::EnttRegistryType& InRegistry, FCk_Entity::IdType InEntity) -> void;
        auto OnRemoved(registry_table::EnttRegistryType& InRegistry, FCk_Entity::IdType InEntity) -> void;
    };

    struct FConnector
    {
        FName _FeatureId;
        TFunction<void(registry_table::EnttRegistryType&, FBitListener&, TArray<entt::scoped_connection>&)> _Connect;
        TFunction<void(registry_table::EnttRegistryType&, FBitListener&)> _Seed;
    };

    // Registers a flag and returns its bit index. Idempotent per FeatureId (re-registration
    // returns the existing bit). Ensures on MaxFlags overflow. Flags registered after a
    // registry was enabled only take effect on that registry's next Enable().
    CKECS_API auto DoRegister(FConnector InConnector) -> int32;

    CKECS_API auto Get_BitIndex(FName InFeatureId) -> int32;
    CKECS_API auto Get_RegisteredFeatureIds() -> TArray<FName>;

    // Connect sinks + one O(n) seed scan per registered flag. No-op when already enabled.
    CKECS_API auto Enable(const FCk_Registry& InRegistry) -> void;

    // No-op when not enabled.
    CKECS_API auto Disable(const FCk_Registry& InRegistry) -> void;

    CKECS_API auto Get_IsEnabled(const FCk_Registry& InRegistry) -> bool;

    // All-zero when disabled or the entity has no registered features.
    CKECS_API auto Get_Flags(const FCk_Registry& InRegistry, FCk_Entity InEntity) -> uint64;

    // Monotonic change counter bumped by every sink fire: an unchanged revision at steady state is
    // provably no membership change since the last poll (O(1), no scan). 0 when disabled.
    CKECS_API auto Get_Revision(const FCk_Registry& InRegistry) -> uint64;

    template <typename T_Fragment>
    auto RegisterFlag(FName InFeatureId) -> int32
    {
        return DoRegister(FConnector{
            InFeatureId,
            [](registry_table::EnttRegistryType& InRegistry, FBitListener& InListener, TArray<entt::scoped_connection>& OutConnections)
            {
                OutConnections.Emplace(InRegistry.template on_construct<T_Fragment>().template connect<&FBitListener::OnAdded>(InListener));
                OutConnections.Emplace(InRegistry.template on_destroy<T_Fragment>().template connect<&FBitListener::OnRemoved>(InListener));
            },
            [](registry_table::EnttRegistryType& InRegistry, FBitListener& InListener)
            {
                for (const auto Entity : InRegistry.template view<T_Fragment>())
                {
                    InListener.OnAdded(InRegistry, Entity);
                }
            }});
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // entt::any (the ctx backing store) requires copy-constructible payloads, so the move-only innards
    // (live sink payloads, scoped connections) sit behind a shared ptr.
    struct FCtx_DebugFeatureFlags
    {
        struct FImpl
        {
            TArray<uint64> _Rows;
            TArray<TUniquePtr<debug_feature_flags::FBitListener>> _Listeners;
            TArray<entt::scoped_connection> _Connections;
            uint64 _Revision = 0;
        };

        TSharedPtr<FImpl> _Impl;
    };
}
