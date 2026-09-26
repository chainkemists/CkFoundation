#include "CkProceduralAnimation/CkProceduralAnimation_Utils.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_animation_utils
{
    auto
        Get_IsBodyAdmissible(
            const FCk_Handle_Transform& InBody)
        -> bool
    {
        return ck::IsValid(InBody)
            && NOT InBody.Has<ck::FTag_DestroyEntity_Initiate>()
            && UCk_Utils_Transform_UE::Has(InBody)
            && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InBody)
            && NOT UCk_Utils_ProceduralGait_UE::Has(InBody)
            && UCk_Utils_ProceduralLeg_UE::Get_Legs(InBody).IsEmpty();
    }

    auto
        Get_AreAssetsAdmissible(
            const UCk_ProceduralRig_Data* InRig,
            const UCk_ProceduralGait_Data* InGait)
        -> bool
    {
        if (ck::Is_NOT_Valid(InRig) || ck::Is_NOT_Valid(InGait))
        { return false; }

        const auto& Legs = InRig->Get_Legs();
        auto Ids = TSet<FName>{};
        for (const auto& Leg : Legs)
        {
            if (ck::Is_NOT_Valid(Leg) || Ids.Contains(Leg.Get_Id()))
            { return false; }

            Ids.Add(Leg.Get_Id());
        }

        return Legs.Num() >= 2 && Legs.Num() <= 64
            && ck::IsValid(InGait->Get_Timing())
            && ck::IsValid(InGait->Get_Step())
            && ck::IsValid(InGait->Get_Probe())
            && InGait->Get_Timing().Get_MaxSimultaneousSwings() <= Legs.Num()
            && ck::algo::AllOf(Legs, [&](const FCk_ProceduralLeg_Spec& InLeg) -> bool
            {
                return ck_procedural_gait_utils::Get_IsRestWithinReach(InLeg.Get_Placement(), InLeg.Get_Chain(), InGait->Get_Step());
            });
    }

    auto
        Get_AreChainsAdmissible(
            const FCk_Handle_Transform& InBody,
            const UCk_ProceduralRig_Data& InRig,
            const TArray<FCk_ProceduralWalker_LegChain>& InChains)
        -> bool
    {
        auto BoundLegIds = TSet<FName>{};
        auto BoundParts = TSet<FCk_Handle>{};
        const auto IsPartAdmissible = [&](const FCk_Handle_Transform& InPart) -> bool
        {
            const auto Admissible = ck::IsValid(InPart)
                && NOT InPart.Has<ck::FTag_DestroyEntity_Initiate>()
                && UCk_Utils_Transform_UE::Has(InPart)
                && InPart != InBody
                && UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InPart) == InBody.ConvertToHandle()
                && NOT BoundParts.Contains(InPart);

            BoundParts.Add(InPart);
            return Admissible;
        };

        for (const auto& Chain : InChains)
        {
            const auto LegIndex = InRig.Get_Legs().IndexOfByPredicate([&](const FCk_ProceduralLeg_Spec& InLeg) -> bool
            {
                return InLeg.Get_Id() == Chain.Get_LegId();
            });

            if (LegIndex == INDEX_NONE || BoundLegIds.Contains(Chain.Get_LegId()) || ck::Is_NOT_Valid(Chain.Get_Rig()))
            { return false; }

            BoundLegIds.Add(Chain.Get_LegId());

            const auto& Rig = Chain.Get_Rig();
            if (Rig.Get_Segments().Num() != InRig.Get_Legs()[LegIndex].Get_Chain().Get_SegmentLengths().Num())
            { return false; }

            const auto HasFoot = Rig.Get_Foot() != FCk_Handle_Transform{};
            const auto PartsAdmissible = ck::algo::AllOf(Rig.Get_Segments(), IsPartAdmissible)
                && (NOT HasFoot || IsPartAdmissible(Rig.Get_Foot()));

            if (NOT PartsAdmissible)
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralAnimation_UE::
    Add_Walker(
        FCk_Handle_Transform& InBody,
        const UCk_ProceduralRig_Data* InRig,
        const UCk_ProceduralGait_Data* InGait,
        const TArray<FCk_ProceduralWalker_LegChain>& InChains)
    -> FCk_ProceduralWalker
{
    const auto Valid = ck_procedural_animation_utils::Get_IsBodyAdmissible(InBody)
        && ck_procedural_animation_utils::Get_AreAssetsAdmissible(InRig, InGait)
        && ck_procedural_animation_utils::Get_AreChainsAdmissible(InBody, *InRig, InChains);
    CK_ENSURE_IF_NOT(Valid,
        TEXT("Procedural animation Add_Walker rejected body [{}]. It needs a live transform body that can own children with "
             "no gait and no legs; a rig layout of 2..64 valid legs with unique Ids; a valid gait preset whose "
             "MaxSimultaneousSwings fits the leg count and whose TargetReachFraction of each leg's chain length reaches its "
             "rest foot; and chains that each name a distinct rig leg, match its segment "
             "count and bind unique live transform parts that are direct lifetime children of the body."),
        InBody)
    { return {}; }

    const auto Legs = ck::algo::Transform<TArray<FCk_Handle_ProceduralLeg>>(InRig->Get_Legs(),
    [&](const FCk_ProceduralLeg_Spec& InLegParams)
    {
        return UCk_Utils_ProceduralLeg_UE::Create(InBody, InLegParams);
    });

    const auto Gait = UCk_Utils_ProceduralGait_UE::Add(InBody, InGait);

    for (const auto& Chain : InChains)
    {
        auto Leg = UCk_Utils_ProceduralLeg_UE::TryGet_Leg(InBody, Chain.Get_LegId());
        UCk_Utils_ProceduralRig_UE::Add(Leg, Chain.Get_Rig());
    }

    return FCk_ProceduralWalker{Gait, Legs};
}

// --------------------------------------------------------------------------------------------------------------------
