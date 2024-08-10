#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRelationship/Player/CkPlayer_Fragment_Data.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkPlayer_Fragment.generated.h"

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

    private:
        ECk_Player_ID _PlayerID;

    public:
        CK_PROPERTY_GET(_PlayerID);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_PlayerInfo, _PlayerID);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRELATIONSHIP_API, PlayerChanged, FCk_Delegate_PlayerChanged_MC,
        FCk_Handle_Player, ECk_Player_ID, ECk_Player_ID);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKRELATIONSHIP_API UCk_Fragment_Player_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_Player_Rep);
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Player_Rep);

public:
    auto
    Broadcast_Assign(
        ECk_Player_ID InPlayerID) -> void;

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
    ECk_Player_ID _PlayerID = ECk_Player_ID::Unassigned;
};

// --------------------------------------------------------------------------------------------------------------------
