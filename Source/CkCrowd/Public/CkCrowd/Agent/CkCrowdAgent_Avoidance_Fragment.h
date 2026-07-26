#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkCrowdAgent_Avoidance_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Designer opt-in, honoured on the agent or anywhere in its lifetime-owner chain: forces sampling on.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_AlwaysSample);

// Designer opt-out, same chain: forces force-only behaviour whatever the neighbor count.
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
    using FFragment_CrowdAgent_AvoidancePolicy = FCk_Fragment_CrowdAgent_AvoidancePolicy;
}
