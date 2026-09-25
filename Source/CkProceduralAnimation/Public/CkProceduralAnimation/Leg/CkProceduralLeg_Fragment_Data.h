#pragma once

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralLeg_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_ProceduralLeg : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProceduralLeg);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProceduralLeg);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralLeg_Placement
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralLeg_Placement);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _HipLocal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _RestFootLocal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 1))
    float _PhaseOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _StepThresholdScale = 1.0f;

public:
    CK_PROPERTY(_HipLocal);
    CK_PROPERTY(_RestFootLocal);
    CK_PROPERTY(_PhaseOffset);
    CK_PROPERTY(_StepThresholdScale);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralLeg_Placement, _HipLocal, _RestFootLocal);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralLeg_Placement, IsValid_Policy_Default,
[=](const FCk_ProceduralLeg_Placement& InPlacement)
{
    return NOT InPlacement.Get_HipLocal().ContainsNaN()
        && NOT InPlacement.Get_RestFootLocal().ContainsNaN()
        && FMath::IsFinite(InPlacement.Get_PhaseOffset())
        && InPlacement.Get_PhaseOffset() >= 0.0f && InPlacement.Get_PhaseOffset() < 1.0f
        && FMath::IsFinite(InPlacement.Get_StepThresholdScale())
        && InPlacement.Get_StepThresholdScale() > 0.0f;
});

// --------------------------------------------------------------------------------------------------------------------

// Rigid chain geometry consumed by the rig. Segment lengths are hip-first in centimetres; the pole is body-local
// and bends the chain. Segment geometry must be centred on its transform origin with length along local +X.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralLeg_ChainGeometry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralLeg_ChainGeometry);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<float> _SegmentLengths;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _PoleLocal = FVector{0.0, 0.0, 100.0};

public:
    CK_PROPERTY(_SegmentLengths);
    CK_PROPERTY(_PoleLocal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralLeg_ChainGeometry, _SegmentLengths);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralLeg_ChainGeometry, IsValid_Policy_Default,
[=](const FCk_ProceduralLeg_ChainGeometry& InChain)
{
    return InChain.Get_SegmentLengths().Num() >= 1
        && InChain.Get_SegmentLengths().Num() <= 8
        && NOT InChain.Get_PoleLocal().ContainsNaN()
        && ck::algo::AllOf(InChain.Get_SegmentLengths(), [](float InLength) -> bool
        {
            return FMath::IsFinite(InLength) && InLength > 0.0f;
        });
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralLeg_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralLeg_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralLeg_Placement _Placement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralLeg_ChainGeometry _Chain;

public:
    CK_PROPERTY_GET(_Id);
    CK_PROPERTY(_Placement);
    CK_PROPERTY(_Chain);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralLeg_Spec, _Id, _Placement, _Chain);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralLeg_Spec, IsValid_Policy_Default,
[=](const FCk_ProceduralLeg_Spec& InParams)
{
    return NOT InParams.Get_Id().IsNone()
        && ck::IsValid(InParams.Get_Placement())
        && ck::IsValid(InParams.Get_Chain());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralLeg_Foot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralLeg_Foot);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _Position = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _Normal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FQuat _Rotation = FQuat::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _SwingAlpha = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _Planted = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _ContactTrusted = false;

public:
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_SwingAlpha);
    CK_PROPERTY(_Planted);
    CK_PROPERTY(_ContactTrusted);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Request_ProceduralLeg_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ProceduralLeg_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ProceduralLeg_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ProceduralLeg_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralLeg_ReleasedPartsOwnership : uint8
{
    KeepBodyOwned    UMETA(DisplayName = "Keep Body Owned (parts die with the body)"),
    TransferToWorld  UMETA(DisplayName = "Transfer To World (parts outlive the body)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralLeg_ReleasedPartsOwnership);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralLeg_ReleasedParts
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralLeg_ReleasedParts);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_Transform> _Parts;

public:
    CK_PROPERTY(_Parts);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Request_ProceduralLeg_Detach : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ProceduralLeg_Detach);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ProceduralLeg_Detach);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProceduralLeg_ReleasedPartsOwnership _PartsOwnership = ECk_ProceduralLeg_ReleasedPartsOwnership::KeepBodyOwned;

public:
    CK_PROPERTY_GET(_PartsOwnership);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ProceduralLeg_Detach, _PartsOwnership);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ProceduralLeg_OnDetached,
    FCk_Handle_ProceduralLeg, InLeg,
    FCk_ProceduralLeg_ReleasedParts, InReleasedParts);

// --------------------------------------------------------------------------------------------------------------------
