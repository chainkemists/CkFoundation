#pragma once

#include "CkPmg_Fragment_Data_DebugShapes.h"

#include "CkEcs/Handle/CkHandle.h"

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

    // Opt this shape in as a click-selection handle for its editor preview's placed actor
    // (see ck::FTag_Pmg_EditorSelectionHandle). Call right after the shape's Add/Create — the
    // mesh component's outer is chosen once, when the shape's setup processor runs. Composite
    // shapes propagate to their child entities' components automatically. No-op outside
    // editor-preview worlds.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Request Act As Editor Selection Handle")
    static FCk_Handle_Pmg_DebugShape
    Request_ActAsEditorSelectionHandle(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InDebugShape);

public:
    // Component outer for a PMG shape's UProceduralMeshComponent: the per-owner selection-proxy
    // host when the shape opted in via ck::FTag_Pmg_EditorSelectionHandle (stamped on the entity
    // or any lifetime ancestor — composite shapes render through child entities; editor previews
    // only), the World otherwise — owner-less and therefore click-through, the default.
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
    // Live mutation requests — drained by FProcessor_Pmg_DebugShape_HandleRequests.
    // Each utility AddOrGets the per-shape Requests fragment and emplaces the
    // request; the processor updates the cached Common-fragment field plus any
    // procmesh-side effect (material parameter, visibility, collision) on the
    // next tick. Works for any PMG shape variant (basic / angular / directional /
    // icon / symbol) since the Common fragment is shared.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Color")
    static FCk_Handle_Pmg_DebugShape
    Request_SetColor(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetColor& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Text")
    static FCk_Handle_Pmg_DebugShape
    Request_SetText(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        FString InNewText);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Line Thickness")
    static FCk_Handle_Pmg_DebugShape
    Request_SetLineThickness(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Draw Lines")
    static FCk_Handle_Pmg_DebugShape
    Request_SetDrawLines(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Duration")
    static FCk_Handle_Pmg_DebugShape
    Request_SetDuration(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDuration& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Render Mode")
    static FCk_Handle_Pmg_DebugShape
    Request_SetRenderMode(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest);

    // Convenience wrapper around Request_SetRenderMode for the common "show or hide" toggle.
    // True  → DoubleSided (the default visible mode).
    // False → Hidden (procmesh hidden + DrawLines processor skips wireframe emission).
    //
    // Use this in preference to the per-property renderMode setter when callers just want a
    // visibility toggle — covers both the filled mesh AND the wireframe overlay in one call.
    // Per-frame consumers (e.g. CrowdAgent body viz) should call this only when the visibility
    // actually changes (track last-applied state); the underlying request handler is cheap but
    // skipping the request entirely when steady-state is cheaper.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Visible")
    static FCk_Handle_Pmg_DebugShape
    Request_SetVisible(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        bool InIsVisible);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName = "[Ck][Pmg][DebugShape] Request Set Enable Collision")
    static FCk_Handle_Pmg_DebugShape
    Request_SetEnableCollision(
        UPARAM(ref) FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
