#include "CkJoltCollisionLayer_Utils.h"

#include <Engine/CollisionProfile.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    auto
        TryDerive_SignatureFromProfile(
            FName InProfileName,
            ECk_Jolt_BodyDomain InDomain)
        -> TOptional<FCk_Jolt_CollisionSignature>
    {
        const auto* CollisionProfiles = UCollisionProfile::Get();
        if (CollisionProfiles == nullptr)
        { return {}; }

        auto ProfileTemplate = FCollisionResponseTemplate{};
        if (NOT CollisionProfiles->GetProfileTemplate(InProfileName, ProfileTemplate))
        { return {}; }

        auto Signature = FCk_Jolt_CollisionSignature{};
        Signature.Set_ObjectChannel(ProfileTemplate.ObjectType);
        Signature.Set_CollisionEnabled(ProfileTemplate.CollisionEnabled);

        for (auto ChannelIndex = 0; ChannelIndex < 32; ++ChannelIndex)
        {
            const auto Channel = static_cast<ECollisionChannel>(ChannelIndex);
            const auto Response = ProfileTemplate.ResponseToChannels.GetResponse(Channel);

            Signature.Set_ResponseToChannel(Channel,
                Response == ECR_Block ? ECk_Jolt_PairInteraction::Block :
                Response == ECR_Overlap ? ECk_Jolt_PairInteraction::Overlap :
                ECk_Jolt_PairInteraction::Ignore);
        }

        Signature.Set_Domain(InDomain);
        return Signature;
    }
}

// --------------------------------------------------------------------------------------------------------------------
