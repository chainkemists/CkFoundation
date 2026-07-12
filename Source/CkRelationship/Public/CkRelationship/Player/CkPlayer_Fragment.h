#pragma once

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRelationship/Player/CkPlayer_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <Serialization/Archive.h>

#include "CkPlayer_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FSnapshotContext; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    /*
        Notes: Tagging Entities with IDs allows us to get all Entities (optionally filtered by specific
        component types) that belong to a particular ID without iterating over _all_ the IDs
    */

    template <ECk_Player_ID T_ID>
    struct FTag_PlayerID : public ck::TTag<FTag_PlayerID<T_ID>>
    {
        constexpr static ECk_Player_ID Get_ID()
        {
            return T_ID;
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRELATIONSHIP_API FFragment_PlayerInfo
    {
    public:
        CK_GENERATED_BODY(FFragment_PlayerInfo);

    public:
        using IsSnapshotable = void;

        // Tier-C: the assigned ID is the whole persistent state. The per-ID FTag_PlayerID<> tag
        // (which Get_ID actually reads) and the replicated container entry are re-derived from
        // this value on restore by FProcessor_Player_ReplicateOnRestore.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto IDByte = static_cast<uint8>(_PlayerID);
            InAr << IDByte;
            _PlayerID = static_cast<ECk_Player_ID>(IDByte);
        }

    private:
        ECk_Player_ID _PlayerID;

    public:
        CK_PROPERTY_GET(_PlayerID);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_PlayerInfo, _PlayerID);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, PlayerChanged, FCk_Delegate_PlayerChanged,
        FCk_Handle_Player, ECk_Player_ID, ECk_Player_ID);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKRELATIONSHIP_API FCk_RepData_Player
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Player);

    UPROPERTY()
    ECk_Player_ID Value = ECk_Player_ID::Unassigned;
};

namespace ck
{
    using FFragment_ContainerRef_Player = TFragment_ContainerEntryRef<FCk_RepData_Player>;
}

// --------------------------------------------------------------------------------------------------------------------
