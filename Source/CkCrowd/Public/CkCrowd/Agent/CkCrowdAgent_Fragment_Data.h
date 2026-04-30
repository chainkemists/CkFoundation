#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "CkCrowdAgent_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Forward declarations for friend declarations below — the actual processor classes live in
// namespace ck (per the codebase convention), so the friend lines need the namespace.
namespace ck
{
    class FProcessor_CrowdAgent_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCROWD_API FCk_Handle_CrowdAgent : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_CrowdAgent); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_CrowdAgent);

// --------------------------------------------------------------------------------------------------------------------

// Reflected ECS params for a crowd agent. Gate 0 carries only structural fields
// (Radius, Height, Tags). Gate 2+ adds MaxSpeed/MaxAcceleration/MaxTurnRate/etc.
// Gate 3+ adds SeparationRadius/SeparationWeight/Flags/IgnoreFlags. Gate 4+ adds
// the piercing/sleep/replan tunables. See PLAN.md for the staged-rollout list and
// CkCrowd/Claude.md for the final defaults table.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_ParamsData);

    friend class ck::FProcessor_CrowdAgent_Setup;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Radius = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _Height = 192.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FGameplayTagContainer _Tags;

public:
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Tags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_ParamsData, _Radius, _Height);
};

// --------------------------------------------------------------------------------------------------------------------
