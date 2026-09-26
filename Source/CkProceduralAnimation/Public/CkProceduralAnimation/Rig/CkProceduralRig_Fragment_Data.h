#pragma once

#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralRig_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_ProceduralRig : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProceduralRig);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProceduralRig);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProceduralRig_Failure : uint8
{
    None,
    InvalidRootScale,
    MissingPart
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralRig_Failure);

// --------------------------------------------------------------------------------------------------------------------

// How the rig poses its chain. Auto aims a single segment, uses two-bone IK for two segments and the curve for three or
// more; an explicit Fabrik or Curve applies to any chain of two or more and cannot pose a single segment.
UENUM(BlueprintType)
enum class ECk_ProceduralRig_ChainSolver : uint8
{
    Auto,
    Fabrik,
    Curve
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralRig_ChainSolver);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralRig_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralRig_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_Transform> _Segments;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Foot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProceduralRig_ChainSolver _Solver = ECk_ProceduralRig_ChainSolver::Auto;

public:
    CK_PROPERTY(_Segments);
    CK_PROPERTY(_Foot);
    CK_PROPERTY(_Solver);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralRig_Spec, _Segments);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ProceduralRig_Spec, IsValid_Policy_Default,
[=](const FCk_ProceduralRig_Spec& InParams)
{
    const auto& Segments = InParams.Get_Segments();
    if (Segments.Num() < 1 || Segments.Num() > 8)
    { return false; }

    if (Segments.Num() == 1 && InParams.Get_Solver() != ECk_ProceduralRig_ChainSolver::Auto)
    { return false; }

    auto Parts = TSet<FCk_Handle>{};
    for (const auto& Segment : Segments)
    {
        if (ck::Is_NOT_Valid(Segment) || Parts.Contains(Segment))
        { return false; }

        Parts.Add(Segment);
    }

    const auto HasFoot = InParams.Get_Foot() != FCk_Handle_Transform{};
    return NOT HasFoot || (ck::IsValid(InParams.Get_Foot()) && NOT Parts.Contains(InParams.Get_Foot()));
});

// --------------------------------------------------------------------------------------------------------------------

// Leg layout for one creature. No runtime handles.
UCLASS(BlueprintType)
class CKPROCEDURALANIMATION_API UCk_ProceduralRig_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ProceduralRig_Data);

protected:
#if WITH_EDITOR
    auto
    IsDataValid(class FDataValidationContext& InContext) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Id"))
    TArray<FCk_ProceduralLeg_Spec> _Legs;

public:
    CK_PROPERTY(_Legs);
};

// --------------------------------------------------------------------------------------------------------------------
