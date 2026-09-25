#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralRig_Fragment_Data.generated.h"

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_ProceduralRig : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProceduralRig);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProceduralRig);

UENUM(BlueprintType)
enum class ECk_ProceduralRig_Failure : uint8
{
    None,
    InvalidRootScale,
    MissingPart
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProceduralRig_Failure);

// Geometry must use local +X as its length axis and be centered on its segment. Mesh scale is
// authored by the caller and is retained on every pose update. Parts remain caller-owned.
USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralRig_Leg
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_ProceduralRig_Leg);

private:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Upper;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Lower;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Foot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _UpperLength = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _LowerLength = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _PoleLocal = FVector{0.0, 0.0, 100.0};
public:
    CK_PROPERTY(_Id);
    CK_PROPERTY(_Upper);
    CK_PROPERTY(_Lower);
    CK_PROPERTY(_Foot);
    CK_PROPERTY(_UpperLength);
    CK_PROPERTY(_LowerLength);
    CK_PROPERTY(_PoleLocal);
};

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Fragment_ProceduralRig_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_ProceduralRig_ParamsData);

private:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, TitleProperty = "_Id"))
    TArray<FCk_ProceduralRig_Leg> _Legs;
public:
    CK_PROPERTY(_Legs);
};
