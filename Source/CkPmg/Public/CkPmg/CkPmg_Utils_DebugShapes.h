#pragma once

#include "CkPmg_Fragment_Data_DebugShapes.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkPmg_Utils_DebugShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Meta = (ScriptMixin = "FCk_Handle_Pmg_DebugShape"))
class CKPMG_API UCk_Utils_Pmg_DebugShape_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_DebugShape_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Pmg_DebugShape);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // Opt this shape in as a click-selection handle for its editor preview's placed actor.
    // Call right after Add/Create — the mesh component's outer is chosen once, when the setup
    // processor runs. Composite shapes propagate to their children. Editor previews only.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Request Act As Editor Selection Handle")
    static FCk_Handle_Pmg_DebugShape
    Request_ActAsEditorSelectionHandle(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InDebugShape);

public:
    // Returns the selection-proxy host when FTag_Pmg_EditorSelectionHandle is stamped on the entity
    // or any lifetime ancestor; the World (owner-less, click-through) otherwise.
    static auto
    Get_MeshComponentOuter(
        UWorld* InWorld,
        const FCk_Handle_Pmg_DebugShape& InShape) -> UObject*;

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Pmg|DebugShape",
        DisplayName="[Ck][Pmg][DebugShape] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Pmg_DebugShape
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Pmg|DebugShape",
        DisplayName="[Ck][Pmg][DebugShape] Handle -> Pmg DebugShape Handle",
        meta = (CompactNodeTitle = "<AsPmgDebugShape>", BlueprintAutocast))
    static FCk_Handle_Pmg_DebugShape
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Pmg DebugShape Handle",
        Category = "Ck|Utils|Pmg|DebugShape",
        meta = (CompactNodeTitle = "INVALID_PmgDebugShapeHandle", Keywords = "make"))
    static FCk_Handle_Pmg_DebugShape
    Get_InvalidHandle() { return {}; };

public:
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Color",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetColor(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetColor& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Text",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetText(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        FString InNewText,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Line Thickness",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetLineThickness(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Draw Lines",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetDrawLines(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Duration",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetDuration(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDuration& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Render Mode",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetRenderMode(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Show/hide toggle covering the filled mesh AND the wireframe overlay: True → DoubleSided,
    // False → Hidden. Per-frame consumers should track last-applied state and only call on an
    // actual change rather than re-requesting every tick.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Visible",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetVisible(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        bool InIsVisible,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Enable Collision",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Pmg_DebugShape
    Request_SetEnableCollision(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
