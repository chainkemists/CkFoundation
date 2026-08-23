#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"

#include <CoreMinimal.h>
#include <Templates/UniquePtr.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /** Why a cooked-world query source is unavailable. Callers must treat every state other than Ready as
     *  untrusted geometry; an editor preview must never quietly substitute live collision for cooked data. */
    enum class ECk_Jolt_CookedWorldQueryLoadStatus : uint8
    {
        NotLoaded,
        Ready,
        InvalidRequest,
        MissingIndex,
        StaleIndex,
        MissingCell,
        StaleCell,
        StaleActor,
        CorruptCell,
        BodyCapacityExceeded
    };

    /** Value-only cooked-world request. Supplying required actor runtime hashes makes the load strict: every
     *  selected cooked actor group must have a matching current hash before any query is made available. */
    struct CKJOLT_API FCk_Jolt_CookedWorldQueryLoadRequest
    {
        FString _CookedDataRootPath;
        FString _MapPackageName;
        FBox _OptionalBounds = FBox{ForceInit};
        bool _RequireCurrentActorRuntimeHashes = false;
        // Keyed by (level, actor) — a bare actor name is unique only within its level, so a flat
        // name->hash map drops all but one same-named actor and spuriously fails the rest.
        TMap<FCk_Jolt_CookedActorKey, uint64> _CurrentActorRuntimeHashes;
    };

    struct CKJOLT_API FCk_Jolt_CookedWorldQueryLoadResult
    {
        ECk_Jolt_CookedWorldQueryLoadStatus _Status = ECk_Jolt_CookedWorldQueryLoadStatus::NotLoaded;
        FString _Message;
        int32 _LoadedCellCount = 0;
        int32 _LoadedBodyCount = 0;

        auto Get_IsReady() const -> bool { return _Status == ECk_Jolt_CookedWorldQueryLoadStatus::Ready; }
    };

    /** A self-contained query-only reconstruction of selected cooked Jolt cells. It exposes no Jolt type and
     *  never observes a game world, so editor tooling can inspect packaged-equivalent collision directly.
     *  Construct, load, and query on the game thread only. */
    class CKJOLT_API FCk_Jolt_CookedWorldQuery
    {
    public:
        FCk_Jolt_CookedWorldQuery();
        ~FCk_Jolt_CookedWorldQuery();

        FCk_Jolt_CookedWorldQuery(const FCk_Jolt_CookedWorldQuery&) = delete;
        auto operator=(const FCk_Jolt_CookedWorldQuery&) -> FCk_Jolt_CookedWorldQuery& = delete;

        FCk_Jolt_CookedWorldQuery(FCk_Jolt_CookedWorldQuery&&) = delete;
        auto operator=(FCk_Jolt_CookedWorldQuery&&) -> FCk_Jolt_CookedWorldQuery& = delete;

    public:
        /** Loads every cooked cell intersecting _OptionalBounds. An invalid bounds box means the entire map.
         *  The result is all-or-nothing: a selected stale, missing, or corrupt cell leaves no queryable world. */
        auto
        Request_Load(const FCk_Jolt_CookedWorldQueryLoadRequest& InRequest) -> FCk_Jolt_CookedWorldQueryLoadResult;

        auto Get_LoadResult() const -> const FCk_Jolt_CookedWorldQueryLoadResult&;
        auto Get_IsReady() const -> bool;
        auto Get_LoadedBounds() const -> const FBox&;

        /// Static-domain exact shape overlap. Invalid/unready input reads false and never issues a Jolt query.
        auto Get_IsBoxOccupied(const FVector& InCenter, const FVector& InHalfExtents) const -> bool;

        /// Static-domain broadphase count only. This intentionally does not expose Jolt body identifiers.
        auto Get_BroadphaseBodyCount(const FBox& InWorldBounds) const -> int32;

        /// Static-domain exact segment query. Invalid/unready input reads false and never issues a Jolt query.
        auto Get_IsSegmentBlocked(const FVector& InFrom, const FVector& InTo) const -> bool;

    private:
        struct FImpl;
        TUniquePtr<FImpl> _Impl;
    };
}

// --------------------------------------------------------------------------------------------------------------------
