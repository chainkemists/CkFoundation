#pragma once

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcs/Transform/CkTransform_Fragment_Data.h"
#include "CkEcs/SceneNode/CkSceneNode_Fragment_Data.h"

#include "CkWorldSpaceWidget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_Utils_WorldSpaceWidget_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_WorldSpaceWidget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_WorldSpaceWidget);

public:
    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Create (At Location)")
    static FCk_Handle_WorldSpaceWidget
    Create_AtLocation(
        FVector InLocation,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Create And Attach (To Entity)")
    static FCk_Handle_WorldSpaceWidget
    CreateAndAttach_ToEntity(
        UPARAM(ref) FCk_Handle_Transform& InAttachTo,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Create And Attach (To Unreal Component)")
    static FCk_Handle_WorldSpaceWidget
    CreateAndAttach_ToUnrealComponent(
        USceneComponent* InAttachTo,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Get Widget Instance")
    static UUserWidget*
    Get_Instance(
        const FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|WorldSpaceWidget",
        DisplayName="[Ck][WorldSpaceWidget] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_WorldSpaceWidget
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|WorldSpaceWidget",
        DisplayName="[Ck][WorldSpaceWidget] Handle -> WorldSpaceWidget Handle",
        meta = (CompactNodeTitle = "<AsWorldSpaceWidget>", BlueprintAutocast))
    static FCk_Handle_WorldSpaceWidget
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid WorldSpaceWidget Handle",
        Category = "Ck|Utils|WorldSpaceWidget",
        meta = (CompactNodeTitle = "INVALID_WorldSpaceWidgetHandle", Keywords = "make"))
    static FCk_Handle_WorldSpaceWidget
    Get_InvalidHandle() { return {}; };

private:
    static auto
    DoAdd(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams) -> FCk_Handle_WorldSpaceWidget;
};

// --------------------------------------------------------------------------------------------------------------------
