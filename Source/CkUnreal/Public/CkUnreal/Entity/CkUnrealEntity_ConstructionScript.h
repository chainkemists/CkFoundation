#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment_Params.h"

#include "CkUnrealEntity_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKUNREAL_API UCk_UnrealEntity_ConstructionScript_WithTransform_PDA : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_UnrealEntity_ConstructionScript_WithTransform_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn, AllowPrivateAccess = true))
    FTransform _EntityInitialTransform;

public:
    CK_PROPERTY(_EntityInitialTransform);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUNREAL_API UCKk_Utils_UnrealEntity_ConstructionScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCKk_Utils_UnrealEntity_ConstructionScript_UE);

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|UnrealEntity|ConstructionScript")
    static FCk_Handle
    BuildEntity(
        FCk_Handle                       InHandle,
        const UCk_UnrealEntity_Base_PDA* InUnrealEntity);
};

// --------------------------------------------------------------------------------------------------------------------
