#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkTargetPoint_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKTARGETING_API UCk_Utils_TargetPoint_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_TargetPoint_UE);

public:
    // The TargetPoint is destroyed when the Owner is destroyed
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint")
    static FCk_Handle_Transform
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform);

    // Transient means that the onus of destroying the TargetPoint is now on the user
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (Transient)",
              meta = (WorldContext="InWorldContextObject"))
    static FCk_Handle_Transform
    Create_Transient(
        const FTransform& InTransform,
        const UObject* InWorldContextObject);
};

// --------------------------------------------------------------------------------------------------------------------
