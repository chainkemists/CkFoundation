#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Collision/GroupFilter.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::spatialquery
{
    using JPH::RTTI;

    /**
     * Immutable probe-side collision data. Entries are written on the game thread, then published for Jolt worker
     * reads. It intentionally excludes context-owner policy: that remains the game-thread overlap-router's job.
     */
    struct FCk_ProbeContactSignature
    {
        FGameplayTag                 ProbeName;
        ECk_ProbeResponse_Policy     ResponsePolicy = ECk_ProbeResponse_Policy::Notify;
        FGameplayTagContainer        Filter;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Jolt's GroupFilter for CkSpatialQuery probes. A Jolt CollisionGroup stores a signature id in its SubGroupID.
     *
     * THREAD CONTRACT: Get_OrRegisterSignature is game-thread only. CanCollide runs on Jolt workers; _Signatures
     * reserves its fixed capacity up front and _PublishedSignatureCount releases each completed entry before workers
     * may read it.
     */
    class CKSPATIALQUERY_API FCk_ProbeContactFilter final : public JPH::GroupFilter
    {
    public:
        JPH_DECLARE_RTTI_VIRTUAL(CKSPATIALQUERY_API, FCk_ProbeContactFilter)

        static constexpr int32 MaxSignatures = 1024;

    public:
        explicit FCk_ProbeContactFilter(int32 InMaxSignatures = MaxSignatures);

        /** Game thread only. Returns JPH::CollisionGroup::cInvalidSubGroup on capacity exhaustion. */
        auto
        Get_OrRegisterSignature(
            const FCk_Fragment_Probe_ParamsData& InParams) -> uint32;

        /** Lock-free Jolt-worker read. */
        auto
        CanCollide(
            const JPH::CollisionGroup& InGroupA,
            const JPH::CollisionGroup& InGroupB) const -> bool override;

    private:
        auto
        Get_IsSignaturePublished(
            uint32 InSignatureId) const -> bool;

        auto
        Get_CanReceive(
            const FCk_ProbeContactSignature& InReceiver,
            const FCk_ProbeContactSignature& InOther) const -> bool;

    private:
        TArray<FCk_ProbeContactSignature> _Signatures;
        std::atomic<uint32>                _PublishedSignatureCount{0};
        int32                              _MaxSignatures;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Registry context owner for the one per-world probe contact filter. Collision groups retain their own JPH
     * references after this context has attached the filter to a body.
     */
    struct CKSPATIALQUERY_API FCk_ProbeContactFilter_Context
    {
        JPH::Ref<FCk_ProbeContactFilter> Filter;

        FCk_ProbeContactFilter_Context();

        /** Game thread only. */
        auto
        Get_OrRegisterSignature(
            const FCk_Fragment_Probe_ParamsData& InParams) const -> uint32;
    };
}

// --------------------------------------------------------------------------------------------------------------------
