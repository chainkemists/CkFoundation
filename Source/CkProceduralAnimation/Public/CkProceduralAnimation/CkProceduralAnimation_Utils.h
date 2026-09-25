#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProceduralAnimation_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralWalker_LegChain
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralWalker_LegChain);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _LegId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ProceduralRig_Spec _Rig;

public:
    CK_PROPERTY_GET(_LegId);
    CK_PROPERTY_GET(_Rig);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralWalker_LegChain, _LegId, _Rig);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_ProceduralWalker
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProceduralWalker);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_ProceduralGait _Gait;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_ProceduralLeg> _Legs;

public:
    CK_PROPERTY_GET(_Gait);
    CK_PROPERTY_GET(_Legs);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProceduralWalker, _Gait, _Legs);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralAnimation_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralAnimation_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralAnimation",
              DisplayName="[Ck][ProceduralAnimation] Add Walker")
    static FCk_ProceduralWalker
    Add_Walker(
        UPARAM(ref) FCk_Handle_Transform& InBody,
        const UCk_ProceduralRig_Data* InRig,
        const UCk_ProceduralGait_Data* InGait,
        const TArray<FCk_ProceduralWalker_LegChain>& InChains);
};

// --------------------------------------------------------------------------------------------------------------------
