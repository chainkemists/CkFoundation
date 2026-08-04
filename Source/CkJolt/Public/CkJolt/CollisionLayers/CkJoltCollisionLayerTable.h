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
    /// One Jolt ObjectLayer per unique collision signature, seeded from UCollisionProfile at subsystem init
    /// and grown on demand. Index == JPH::ObjectLayer.
    ///
    /// THREAD CONTRACT: registration is GAME THREAD only; Jolt's filter callbacks read from worker threads
    /// DURING PhysicsSystem::Update. The signature array is reserved to fixed capacity up front (never
    /// reallocates) and the visible count is published through an atomic, so readers are lock-free.
    class CKJOLT_API FCk_Jolt_CollisionLayerTable
    {
    public:
        CK_GENERATED_BODY(FCk_Jolt_CollisionLayerTable);

        static constexpr int32 MaxLayers = 1024;

    public:
        FCk_Jolt_CollisionLayerTable();

        /// Seeds (profile x {Static, Dynamic}) signatures in deterministic profile order, then appends the
        /// fixed probe signature. Game thread, ONCE, before the PhysicsSystem exists.
        auto Build_FromCollisionProfiles() -> void;

        /// Resolve-or-register (game thread only). Returns JPH::cObjectLayerInvalid (and ensures)
        /// only on capacity exhaustion.
        auto Get_OrRegisterLayer(
            const FCk_Jolt_CollisionSignature& InSignature) -> uint16;

        /// The one layer every Probe body lives on: WorldDynamic, Overlap ONLY vs WorldDynamic, Domain
        /// Dynamic — so probes pair with probes and NEVER with the static world.
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
            // Statics never pair with the static tree (statics don't move), and the static world is
            // query-only for probes — overlap semantics vs statics is a deliberate future opt-in.
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

    /// A body passes when its authored response to the query channel is at least InMinResponse (Block for
    /// trace-like queries, Overlap for overlap-like ones) — matching UKismetSystemLibrary trace semantics.
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

    /// Object filter for occupancy queries, fixed to the Static domain — baked level geometry AND
    /// Static-motion-type JoltBodies. Both are immovable world obstacles, which is what an occupancy or
    /// navigation bake wants. Fixed rather than parameterized (unlike FCk_Jolt_DomainQueryFilter) so a
    /// per-cell query in a tight loop cannot be handed the wrong domain.
    class CKJOLT_API FCk_Jolt_StaticOccupancyFilter final : public JPH::ObjectLayerFilter
    {
    public:
        explicit FCk_Jolt_StaticOccupancyFilter(
            const FCk_Jolt_CollisionLayerTable& InTable)
            : _Table(InTable) {}

        auto ShouldCollide(JPH::ObjectLayer inLayer) const -> bool override
        {
            return _Table.Get_Domain(inLayer) == ECk_Jolt_BodyDomain::Static;
        }

    private:
        const FCk_Jolt_CollisionLayerTable& _Table;
    };

    /// Broadphase companion to FCk_Jolt_StaticOccupancyFilter: the dynamic tree is never descended at all.
    /// The scene-query wrappers pass an accept-all JPH::BroadPhaseLayerFilter, which is the right default
    /// for a handful of gameplay traces and the wrong one for grid-scale query counts.
    class CKJOLT_API FCk_Jolt_StaticBroadPhaseQueryFilter final : public JPH::BroadPhaseLayerFilter
    {
    public:
        auto ShouldCollide(JPH::BroadPhaseLayer inLayer) const -> bool override
        {
            return inLayer == broadphase_layers::Static;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /// Published into the ECS registry context alongside the PhysicsSystem. Non-const because body spawn is a
    /// sanctioned (game-thread) registration site; Probe setup only reads _ProbeLayer.
    struct CKJOLT_API FCk_Jolt_LayerContext
    {
        FCk_Jolt_CollisionLayerTable* _Table = nullptr;
        uint16 _ProbeLayer = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
