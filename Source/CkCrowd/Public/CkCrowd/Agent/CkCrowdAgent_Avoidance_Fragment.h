#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkCrowdAgent_Avoidance_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Per-agent avoidance-policy override + zone-tag opt-in/out for sampling.
// --------------------------------------------------------------------------------------------------------------------

// Designer-side opt-in: any agent (or any entity in the agent's lifetime-owner chain) carrying
// this tag forces sampling on regardless of NeighborCache size.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_AlwaysSample);

// Designer-side opt-out: forces force-only behaviour even when neighbor count would normally
// trigger sampling. Use case: tutorial NPCs, scripted set-pieces.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_NeverSample);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AvoidancePolicy : uint8
{
    UseProjectDefault,  // honour _AvoidanceSampleTrigger + tags
    ForceOnly,          // never sample (overrides project + zone tag)
    SamplingAlways,     // always sample (overrides everything)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_AvoidancePolicy
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_AvoidancePolicy);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_AvoidancePolicy _Policy = ECk_AvoidancePolicy::UseProjectDefault;

public:
    CK_PROPERTY(_Policy);
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_AvoidancePolicy, _Policy);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Alias so processors include this fragment alongside the others on an agent.
    using FFragment_CrowdAgent_AvoidancePolicy = FCk_Fragment_CrowdAgent_AvoidancePolicy;
}
