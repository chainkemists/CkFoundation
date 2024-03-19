#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkSubstep_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Substep_State : uint8
{
    Paused,
    Running
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Substep_State);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSUBSTEP_API FCk_Handle_Substep : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Substep); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Substep);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSUBSTEP_API FCk_Substep_ParamsData
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Substep_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FCk_Time _TickRate = FCk_Time{0.01};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_Substep_State _StartingState = ECk_Substep_State::Paused;

public:
    CK_PROPERTY_GET(_TickRate);

    CK_DEFINE_CONSTRUCTORS(FCk_Substep_ParamsData, _TickRate);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Substep_OnUpdate,
    FCk_Handle_Substep, InHandle,
    FCk_Time, InSubstepDeltaT,
    int32, InStepNumber,
    FCk_Time, InFrameDeltaT);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FCk_Delegate_Substep_OnUpdate_MC,
    FCk_Handle_Substep, InHandle,
    FCk_Time, InSubstepDeltaT,
    int32, InStepNumber,
    FCk_Time, InFrameDeltaT);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Substep_OnFrameEnd,
    FCk_Handle_Substep, InHandle,
    FCk_Time, InFrameDeltaT);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Substep_OnFrameEnd_MC,
    FCk_Handle_Substep, InHandle,
    FCk_Time, InFrameDeltaT);

// --------------------------------------------------------------------------------------------------------------------
