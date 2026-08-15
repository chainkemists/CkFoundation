#pragma once

#include "CkWorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"

#include "CkWorldSpaceWidget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_WorldSpaceWidget"))
class CKWORLDSPACEWIDGET_API UCk_Utils_WorldSpaceWidget_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_WorldSpaceWidget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_WorldSpaceWidget);

public:
    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Create (At Location)", meta = (Keywords = "add"))
    static FCk_Handle_WorldSpaceWidget
    Create_AtLocation(
        FVector InLocation,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Create And Attach (To Entity)", meta = (Keywords = "add"))
    static FCk_Handle_WorldSpaceWidget
    CreateAndAttach_ToEntity(
        UPARAM(ref) FCk_Handle_Transform& InAttachTo,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Create And Attach (To Unreal Component)", meta = (Keywords = "add"))
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
    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Request Set Enabled",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_WorldSpaceWidget
    Request_SetEnabled(
        UPARAM(ref) FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        bool InEnabled,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Request Set Location Info",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_WorldSpaceWidget
    Request_SetLocationInfo(
        UPARAM(ref) FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_LocationInfo& InLocationInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Request Set Scaling Info",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_WorldSpaceWidget
    Request_SetScalingInfo(
        UPARAM(ref) FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_ScalingInfo& InScalingInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Request Set Fading Info",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_WorldSpaceWidget
    Request_SetFadingInfo(
        UPARAM(ref) FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_FadingInfo& InFadingInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Request Set Occlusion Info",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_WorldSpaceWidget
    Request_SetOcclusionInfo(
        UPARAM(ref) FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_OcclusionInfo& InOcclusionInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Get Widget Instance")
    static UUserWidget*
    Get_Instance(
        const FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle);

    // Traces from the player camera to the widget's world anchor (transform + LocationInfo world
    // offset) on the configured channel, ignoring the player pawn; a blocking hit means occluded.
    // Does NOT gate on OcclusionPolicy, and performs a world line trace on every call.
    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
              Category = "Ck|Utils|WorldSpaceWidget",
              DisplayName="[Ck][WorldSpaceWidget] Get Is Anchor Occluded")
    static bool
    Get_IsAnchorOccluded(
        const FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle);

    // Test/diagnostic hook: true iff the ScreenOverlay wrapper exists and is Collapsed.
    // Always false for WorldComponent mode.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WorldSpaceWidget|Debug",
              DisplayName="[Ck][WorldSpaceWidget] Get Debug Is Wrapper Collapsed")
    static bool
    Get_Debug_IsWrapperCollapsed(
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
    Get_InvalidHandle() { return {}; }

private:
    static auto
    DoAdd(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams) -> FCk_Handle_WorldSpaceWidget;

    static auto
    DoAdd_ScreenOverlay(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams) -> FCk_Handle_WorldSpaceWidget;

    static auto
    DoAdd_WorldComponent(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams) -> FCk_Handle_WorldSpaceWidget;
};
