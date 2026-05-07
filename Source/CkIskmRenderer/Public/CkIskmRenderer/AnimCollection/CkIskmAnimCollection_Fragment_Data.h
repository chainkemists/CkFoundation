#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcsExt/EntityScript/CkEntityScript_Utils.h"

#include "CkIskmAnimCollection_Fragment_Data.generated.h"

UENUM(BlueprintType)
enum class ECk_IskmAnimCollection_ValidationResult : uint8
{
    Valid,
    SkeletonMissing,
    SequenceSkeletonMismatch,
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmAnimCollection_SequenceDef
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IskmAnimCollection_SequenceDef);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimSequenceBase> _Sequence;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _Name;

public:
    CK_PROPERTY_GET(_Sequence);
    CK_PROPERTY_GET(_Name);
};
