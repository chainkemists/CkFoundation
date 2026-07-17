#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// One Jolt ObjectLayer per unique collision signature, seeded from UCollisionProfile at
    /// subsystem init (never hand-authored) and grown on demand when bodies with per-component
    /// custom responses register. Index == JPH::ObjectLayer.
    ///
    /// Thread contract: registration is GAME THREAD only (bake, cell load, body spawn); Jolt's
    /// filter callbacks read from worker threads DURING PhysicsSystem::Update. The signature
    /// array is reserved to fixed capacity up front (never reallocates) and the visible count is
    /// published through an atomic — readers are lock-free and always see fully-written entries.
    ///
    /// Pair semantics == UE's touch resolution: min(A.Response[B.Channel], B.Response[A.Channel]).
    /// Jolt's binary pair filter treats anything != Ignore as "interact"; Block-vs-Overlap is
    /// resolved at query/contact sites via the query filters below.
    class CKJOLT_API FCk_Jolt_CollisionLayerTable
    {
    public:
        CK_GENERATED_BODY(FCk_Jolt_CollisionLayerTable);

        static constexpr int32 MaxLayers = 1024;

    public:
        FCk_Jolt_CollisionLayerTable();

        /// Seeds (profile x {Static, Dynamic}) signatures for every engine + project collision
        /// profile with collision enabled, in deterministic profile order, then appends the
        /// fixed probe signature. Game thread, once, before the PhysicsSystem exists.
        auto Build_FromCollisionProfiles() -> void;

        /// Resolve-or-register (game thread only). Returns JPH::cObjectLayerInvalid (and ensures)
        /// only on capacity exhaustion.
        auto Get_OrRegisterLayer(
            const FCk_Jolt_CollisionSignature& InSignature) -> uint16;

        /// The one layer every Probe body lives on: WorldDynamic, Overlap ONLY vs WorldDynamic,
        /// Domain Dynamic. Preserves the pre-table behavior exactly — probes pair with probes
        /// (and other dynamic-domain WorldDynamic bodies) and NEVER with the static world (also
        /// enforced at the broadphase level by FObjectVsBroadPhaseLayerFilter_Table).
        auto Get_ProbeLayer() const -> uint16;

        static auto Get_ProbeSignature() -> FCk_Jolt_CollisionSignature;

        // ---- lock-free worker-thread reads ----

        auto Get_NumLayers() const -> int32;

        auto Get_Signature(
            uint16 InLayer) const -> const FCk_Jolt_CollisionSignature&;

        auto Get_PairInteraction(
            uint16 InLayerA,
            uint16 InLayerB) const -> ECk_Jolt_PairInteraction;

        /// UE trace semantics: how the body on InLayer responds to a query on InChannel.
        auto Get_ResponseOfLayerToChannel(
            uint16 InLayer,
            ECollisionChannel InChannel) const -> ECk_Jolt_PairInteraction;

        auto Get_Domain(
            uint16 InLayer) const -> ECk_Jolt_BodyDomain;

    private:
        TArray<FCk_Jolt_CollisionSignature> _Signatures;
        TMap<FCk_Jolt_CollisionSignature, uint16> _SignatureToLayer;
        std::atomic<int32> _PublishedCount{0};
        uint16 _ProbeLayer = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Filter implementations (PhysicsSystem holds references — instances must outlive it)
    // ----------------------------------------------------------------------------------------------------------------

    namespace broadphase_layers
    {
        static constexpr JPH::BroadPhaseLayer Static{0};
        static constexpr JPH::BroadPhaseLayer Dynamic{1};
        static constexpr JPH::uint Num_Layers{2};
    }

    class CKJOLT_API FCk_Jolt_BroadPhaseLayerInterface_Table final : public JPH::BroadPhaseLayerInterface
    {
    public:
        explicit FCk_Jolt_BroadPhaseLayerInterface_Table(const FCk_Jolt_CollisionLayerTable& InTable)
            : _Table(InTable) {}

        auto GetNumBroadPhaseLayers() const -> JPH::uint override
        {
            return broadphase_layers::Num_Layers;
        }

        auto GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const -> JPH::BroadPhaseLayer override
        {
            return _Table.Get_Domain(inLayer) == ECk_Jolt_BodyDomain::Static
                ? broadphase_layers::Static
                : broadphase_layers::Dynamic;
        }

    private:
        const FCk_Jolt_CollisionLayerTable& _Table;
    };

    class CKJOLT_API FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table final : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        explicit FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table(const FCk_Jolt_CollisionLayerTable& InTable)
            : _Table(InTable) {}

        auto ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const -> bool override
        {
            // Statics never pair with the static tree (statics don't move), and probes never
            // pair with it either (pre-table behavior: the static world is query-only for
            // probes — overlap semantics vs statics is a deliberate future opt-in).
            if (inLayer2 == broadphase_layers::Static)
            {
                if (_Table.Get_Domain(inLayer1) == ECk_Jolt_BodyDomain::Static)
                { return false; }

                if (inLayer1 == _Table.Get_ProbeLayer())
                { return false; }
            }

            return true;
        }

    private:
        const FCk_Jolt_CollisionLayerTable& _Table;
    };

    class CKJOLT_API FCk_Jolt_ObjectLayerPairFilter_Table final : public JPH::ObjectLayerPairFilter
    {
    public:
        explicit FCk_Jolt_ObjectLayerPairFilter_Table(const FCk_Jolt_CollisionLayerTable& InTable)
            : _Table(InTable) {}

        auto ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const -> bool override
        {
            // Static-vs-static can slip past the broadphase filter via direct pair tests.
            if (_Table.Get_Domain(inObject1) == ECk_Jolt_BodyDomain::Static &&
                _Table.Get_Domain(inObject2) == ECk_Jolt_BodyDomain::Static)
            { return false; }

            return _Table.Get_PairInteraction(inObject1, inObject2) != ECk_Jolt_PairInteraction::Ignore;
        }

    private:
        const FCk_Jolt_CollisionLayerTable& _Table;
    };

    /// Query-side filter carrying a channel: a body passes when its authored response to the
    /// query channel is at least InMinResponse (Block for trace-like queries, Overlap for
    /// overlap-like ones) — matching UKismetSystemLibrary trace semantics.
    class CKJOLT_API FCk_Jolt_ChannelQueryFilter final : public JPH::ObjectLayerFilter
    {
    public:
        FCk_Jolt_ChannelQueryFilter(
            const FCk_Jolt_CollisionLayerTable& InTable,
            ECollisionChannel InChannel,
            ECk_Jolt_PairInteraction InMinResponse)
            : _Table(InTable), _Channel(InChannel), _MinResponse(InMinResponse) {}

        auto ShouldCollide(JPH::ObjectLayer inLayer) const -> bool override
        {
            return _Table.Get_ResponseOfLayerToChannel(inLayer, _Channel) >= _MinResponse;
        }

    private:
        const FCk_Jolt_CollisionLayerTable& _Table;
        ECollisionChannel _Channel;
        ECk_Jolt_PairInteraction _MinResponse;
    };

    /// Query-side filter selecting one body domain (e.g. static-world-only introspection rays).
    class CKJOLT_API FCk_Jolt_DomainQueryFilter final : public JPH::ObjectLayerFilter
    {
    public:
        FCk_Jolt_DomainQueryFilter(
            const FCk_Jolt_CollisionLayerTable& InTable,
            ECk_Jolt_BodyDomain InDomain)
            : _Table(InTable), _Domain(InDomain) {}

        auto ShouldCollide(JPH::ObjectLayer inLayer) const -> bool override
        {
            return _Table.Get_Domain(inLayer) == _Domain;
        }

    private:
        const FCk_Jolt_CollisionLayerTable& _Table;
        ECk_Jolt_BodyDomain _Domain;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /// Published into the ECS registry context alongside the PhysicsSystem so consumers can place
    /// bodies on table layers without a subsystem ref. Non-const: registration is game-thread only
    /// and body-spawn (JoltBody setup) is a sanctioned registration site — it resolves-or-registers
    /// a body's profile-derived signature via Get_OrRegisterLayer. Probe setup only reads _ProbeLayer.
    struct CKJOLT_API FCk_Jolt_LayerContext
    {
        FCk_Jolt_CollisionLayerTable* _Table = nullptr;
        uint16 _ProbeLayer = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
