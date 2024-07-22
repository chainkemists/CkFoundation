#include "CkRelationship_Utils.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Relationship_UE::
    Get_AttitudeTowards(
        const FCk_Handle& InFrom,
        const FCk_Handle& InTo)
    -> ECk_RelationshipAttitude
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFrom), TEXT("Unable to Get_AttitudeTowards [{}] because [{}] is INVALID."), InTo, InFrom)
    { return ECk_RelationshipAttitude::Neutral; }

    CK_ENSURE_IF_NOT(ck::IsValid(InTo), TEXT("Unable to Get_AttitudeTowards from [{}] because [{}] INVALID."), InFrom, InTo)
    { return ECk_RelationshipAttitude::Neutral; }

    if (InFrom == InTo)
    { return ECk_RelationshipAttitude::Friendly; }

    const auto& FromTeam = UCk_Utils_Team_UE::TryGet_Entity_Team_InOwnershipChain(InFrom);

    if (ck::Is_NOT_Valid(FromTeam))
    { return ECk_RelationshipAttitude::Neutral; }

    const auto& ToTeam = UCk_Utils_Team_UE::TryGet_Entity_Team_InOwnershipChain(InTo);

    if (ck::Is_NOT_Valid(ToTeam))
    { return ECk_RelationshipAttitude::Neutral; }

    if (NOT UCk_Utils_Team_UE::Get_IsSame(FromTeam, ToTeam))
    { return ECk_RelationshipAttitude::Hostile; }

    return ECk_RelationshipAttitude::Friendly;
}

auto
    UCk_Utils_Relationship_UE::
    Get_AttitudeTowards_Exec(
        const FCk_Handle& InFrom,
        const FCk_Handle& InTo)
    -> ECk_RelationshipAttitude
{
    return Get_AttitudeTowards(InFrom, InTo);
}

// --------------------------------------------------------------------------------------------------------------------
