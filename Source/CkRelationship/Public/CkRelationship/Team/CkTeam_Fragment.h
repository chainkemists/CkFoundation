#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRelationship/Team/CkTeam_Fragment_Data.h"

#include "CkSignal/CkSignal_Macros.h"

#include <GenericTeamAgentInterface.h>

#include "CkTeam_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FGenericTeamId, [&]()
{
    return ck::Format(TEXT("{}"), InObj.GetId());
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

    private:
        ECk_Team_ID _TeamID = ECk_Team_ID::Unassigned;

    public:
        CK_PROPERTY_GET(_TeamID);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_TeamInfo, _TeamID);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, TeamChanged, FCk_Delegate_TeamChanged_MC,
        FCk_Handle_Team, ECk_Team_ID, ECk_Team_ID);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKRELATIONSHIP_API UCk_Fragment_Team_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Team_Rep);
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Team_Rep);

public:
    auto
    Broadcast_Assign(
        ECk_Team_ID InTeamID) -> void;

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    UFUNCTION()
    void
    OnRep_Updated();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    ECk_Team_ID _TeamID = ECk_Team_ID::Unassigned;
};

// --------------------------------------------------------------------------------------------------------------------
