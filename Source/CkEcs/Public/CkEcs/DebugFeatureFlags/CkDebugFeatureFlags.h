#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Debug feature-flag cache (ECS debugger redesign, CkGameplayDebugger docs/specs/2026-07-10 §5).
//
// A per-registry bit table — one uint64 row per entity index — maintained by EnTT
// on_construct/on_destroy sinks on each registered feature's MARKER fragment (the stable
// Params/Current fragment, never request/transient tags). Rows self-correct on entity
// destruction because each feature's on_destroy fires for fragment removal AND entity
// destruction alike.
//
// Zero cost until Enable() connects the sinks (the debugger opening); consumers get O(1)
// per-entity feature queries, and archetype matching compiles to
// (bits & required) == required.
//
// Feature→fragment registration is intentionally NOT in this module: CkEcs is
// feature-agnostic and must not see T4 feature modules. Consumers that link the fragment
// types (the ECS debugger links every feature module) call
// RegisterFlag<TFragment>(FeatureId) at startup, BEFORE Enable().
// --------------------------------------------------------------------------------------------------------------------

namespace ck::debug_feature_flags
{
    static constexpr int32 MaxFlags = 64;

    // Sink payload. EnTT sinks cannot take capturing lambdas — each registered flag gets
    // one heap-stable listener instance (owned by the registry's ctx payload) whose bit
    // index routes the callback to the right column.
    struct CKECS_API FBitListener
    {
        int32 _Bit = INDEX_NONE;

        auto OnAdded(registry_table::EnttRegistryType& InRegistry, FCk_Entity::IdType InEntity) -> void;
        auto OnRemoved(registry_table::EnttRegistryType& InRegistry, FCk_Entity::IdType InEntity) -> void;
    };

    // Type-erased per-flag hookup, captured at RegisterFlag<T> instantiation.
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

    // Disconnect sinks and drop the bit table. No-op when not enabled.
    CKECS_API auto Disable(const FCk_Registry& InRegistry) -> void;

    CKECS_API auto Get_IsEnabled(const FCk_Registry& InRegistry) -> bool;

    // All-zero when disabled or the entity has no registered features.
    CKECS_API auto Get_Flags(const FCk_Registry& InRegistry, FCk_Entity InEntity) -> uint64;

    // Monotonic change counter, bumped by every sink fire (marker fragment added or
    // removed — which includes entity spawn/destroy for flagged features). Consumers
    // poll it O(1) to detect churn without any O(n) scan: unchanged revision at steady
    // state = provably no membership change since the last poll. 0 when disabled.
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
    // Registry-ctx payload (see FCtx_TransientEntity for the pattern). entt::any (the
    // ctx backing store) requires copy-constructible payloads, so the move-only innards
    // (listeners are live sink payloads, connections are scoped) sit behind a shared ptr.
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
