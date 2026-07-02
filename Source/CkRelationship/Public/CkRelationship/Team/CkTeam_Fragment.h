#pragma once

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRelationship/Team/CkTeam_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <GenericTeamAgentInterface.h>
#include <NativeGameplayTags.h>
#include <Serialization/Archive.h>

#include "CkTeam_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FSnapshotContext; }

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FGenericTeamId, [](const FGenericTeamId& InObj)
{
    return ck::Format(TEXT("{}"), InObj.GetId());
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_OnTeamAssigned_Setup);

    // Per-feature restore-replication done marker. ck::FTag_Snapshot_JustRestored lives on the OWNER
    // entity and is shared by every owner-resident feature, so no single feature may remove it —
    // each feature pairs it with its own done tag instead. Never leaks across restores: a restore
    // rebuilds entities from snapshot bytes and this tag is not snapshotted.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Team_RestoreReplicated);

    // --------------------------------------------------------------------------------------------------------------------

    /*
        Notes: Tagging Entities with IDs allows us to get all Entities (optionally filtered by specific
        component types) that belong to a particular ID without iterating over _all_ the IDs
    */

    template <ECk_Team_ID T_ID>
    struct FTag_TeamID : public ck::TTag<FTag_TeamID<T_ID>>
    {
        constexpr static ECk_Team_ID Get_ID()
        {
            return T_ID;
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRELATIONSHIP_API FFragment_TeamInfo
    {
    public:
        CK_GENERATED_BODY(FFragment_TeamInfo);

    public:
        using IsSnapshotable = void;

        // Tier-C: the assigned ID is the whole persistent state. The per-ID FTag_TeamID<> tag
        // (which Get_ID actually reads) and the replicated container entry are re-derived from
        // this value on restore by FProcessor_Team_ReplicateOnRestore.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto IDByte = static_cast<uint8>(_TeamID);
            InAr << IDByte;
            _TeamID = static_cast<ECk_Team_ID>(IDByte);
        }

    private:
        ECk_Team_ID _TeamID = ECk_Team_ID::Unassigned;

    public:
        CK_PROPERTY_GET(_TeamID);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TeamInfo, _TeamID);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, TeamChanged, FCk_Delegate_TeamChanged,
        FCk_Handle_Team, ECk_Team_ID, ECk_Team_ID);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_TeamListener);
    CK_DEFINE_ECS_TAG(FTag_TeamListener_Setup);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, TeamAssigned, FCk_Delegate_TeamAssigned,
        FCk_Handle_Team, ECk_Team_ID);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, TeamAssigned_OnOpposingTeam, FCk_Delegate_TeamAssigned,
        FCk_Handle_Team, ECk_Team_ID);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, TeamAssigned_OnSameTeam, FCk_Delegate_TeamAssigned,
        FCk_Handle_Team, ECk_Team_ID);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKRELATIONSHIP_API FCk_RepData_Team
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_Team);

    UPROPERTY()
    ECk_Team_ID Value = ECk_Team_ID::Unassigned;
};

namespace ck
{
    using FFragment_ContainerRef_Team = TFragment_ContainerEntryRef<FCk_RepData_Team>;
}

// --------------------------------------------------------------------------------------------------------------------
