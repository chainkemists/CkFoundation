#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment_Data.h"

#include "CkRenderTarget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UTextureRenderTarget2D;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RenderTarget"))
class CKRENDERTARGET_API UCk_Utils_RenderTarget_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RenderTarget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RenderTarget);

private:
    struct RecordOfRenderTargets_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfRenderTargets> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Add New Render Target")
    static FCk_Handle_RenderTarget
    Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Fragment_RenderTarget_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RenderTarget",
        DisplayName="[Ck][RenderTarget] Has Any Render Target")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|RenderTarget",
        DisplayName="[Ck][RenderTarget] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_RenderTarget
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RenderTarget",
        DisplayName="[Ck][RenderTarget] Handle -> RenderTarget Handle",
        meta = (CompactNodeTitle = "<AsRenderTarget>", BlueprintAutocast))
    static FCk_Handle_RenderTarget
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid RenderTarget Handle",
        Category = "Ck|Utils|RenderTarget",
        meta = (CompactNodeTitle = "INVALID_RenderTargetHandle", Keywords = "make"))
    static FCk_Handle_RenderTarget
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Try Get Render Target")
    static FCk_Handle_RenderTarget
    TryGet_RenderTarget(
        const FCk_Handle& InOwnerEntity,
        UPARAM(meta = (Categories = "RenderTarget")) FGameplayTag InSyncName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Sync Name")
    static FGameplayTag
    Get_SyncName(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    // The drawable target object. Invalid until the Setup processor has run (one tick after Add)
    // and on machines that cannot render.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Target")
    static UTextureRenderTarget2D*
    Get_Target(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    // The seq of the last instruction batch applied in THIS world — locally authored on the
    // authority, replayed from the wire on clients. 0 before any batch has applied.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Latest Applied Batch Seq")
    static int32
    Get_LatestAppliedBatchSeq(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    // CRC of this world's authoritative CPU pixel state — the host's last-synced snapshot, or
    // the client's staging mirror. 0 when no pixel state exists yet. Worlds that are pixel-synced
    // report the SAME hash, which is what the NET tests assert (no GPU required).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Pixel State Hash")
    static int32
    Get_PixelStateHash(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    // Receive-side observability: seq / kind of the last pixel payload applied in this world
    // (0 / FullSync before any payload applied), and the instruction watermark carried by the
    // last applied baseline.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Latest Applied Pixel Payload Seq")
    static int32
    Get_LatestAppliedPixelPayloadSeq(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Latest Applied Pixel Payload Kind")
    static ECk_RenderTarget_PixelPayloadKind
    Get_LatestAppliedPixelPayloadKind(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Baseline Instruction Watermark")
    static int32
    Get_BaselineInstructionWatermark(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

    // Host-side observability: number of connected players whose pixel baseline is established
    // (FullSync acked). 0 on clients and before any ack.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Get Num Baselined Clients")
    static int32
    Get_NumBaselinedClients(
        const FCk_Handle_RenderTarget& InRenderTargetEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Line")
    static FCk_Handle_RenderTarget
    Request_DrawLine(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawLine& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Texture")
    static FCk_Handle_RenderTarget
    Request_DrawTexture(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawTexture& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Material")
    static FCk_Handle_RenderTarget
    Request_DrawMaterial(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawMaterial& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Text")
    static FCk_Handle_RenderTarget
    Request_DrawText(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawText& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Box")
    static FCk_Handle_RenderTarget
    Request_DrawBox(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawBox& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Border")
    static FCk_Handle_RenderTarget
    Request_DrawBorder(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawBorder& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Triangles")
    static FCk_Handle_RenderTarget
    Request_DrawTriangles(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawTriangles& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Draw Polygon")
    static FCk_Handle_RenderTarget
    Request_DrawPolygon(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawPolygon& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Clear")
    static FCk_Handle_RenderTarget
    Request_Clear(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_Clear& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] Request Sync Pixels",
              meta=(AutoCreateRefTerm="InRequest"))
    static FCk_Handle_RenderTarget
    Request_SyncPixels(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_SyncPixels& InRequest);

public:
    // Test seam: injects a synthetic readback result directly after the GPU step, so
    // diff → compress → payload (and transport/apply downstream) run under -nullrhi CI. Not for
    // gameplay use; no-ops with an ensure in Shipping builds.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName="[Ck][RenderTarget] DEBUG Inject Captured Pixels")
    static FCk_Handle_RenderTarget
    Debug_InjectCapturedPixels(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const TArray<uint8>& InPixels,
        FIntPoint InSize);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Bind To OnInstructionsApplied")
    static FCk_Handle_RenderTarget
    BindTo_OnInstructionsApplied(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnInstructionsApplied& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Unbind From OnInstructionsApplied")
    static FCk_Handle_RenderTarget
    UnbindFrom_OnInstructionsApplied(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnInstructionsApplied& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Bind To OnPixelPayloadProduced")
    static FCk_Handle_RenderTarget
    BindTo_OnPixelPayloadProduced(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadProduced& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Unbind From OnPixelPayloadProduced")
    static FCk_Handle_RenderTarget
    UnbindFrom_OnPixelPayloadProduced(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadProduced& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Bind To OnPixelPayloadApplied")
    static FCk_Handle_RenderTarget
    BindTo_OnPixelPayloadApplied(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadApplied& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Unbind From OnPixelPayloadApplied")
    static FCk_Handle_RenderTarget
    UnbindFrom_OnPixelPayloadApplied(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadApplied& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Bind To OnClientBaselineEstablished")
    static FCk_Handle_RenderTarget
    BindTo_OnClientBaselineEstablished(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnClientBaselineEstablished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RenderTarget",
              DisplayName = "[Ck][RenderTarget] Unbind From OnClientBaselineEstablished")
    static FCk_Handle_RenderTarget
    UnbindFrom_OnClientBaselineEstablished(
        UPARAM(ref) FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnClientBaselineEstablished& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
