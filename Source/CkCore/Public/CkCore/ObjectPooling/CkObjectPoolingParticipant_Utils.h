#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCore/ObjectPooling/CkObjectPoolingParticipant.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkObjectPoolingParticipant_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ObjectPoolingParticipant"))
class CKCORE_API UCk_Utils_ObjectPoolingParticipant_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ObjectPoolingParticipant_UE);

public:
    // Binds are idempotent per (object, function) — safe to call from Construct, which re-runs on
    // every acquire of a recycled instance
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Bind To OnAcquiredFromPool")
    static FCk_Handle_ObjectPoolingParticipant
    BindTo_OnAcquiredFromPool(
        UPARAM(ref) FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnAcquired& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Unbind From OnAcquiredFromPool")
    static FCk_Handle_ObjectPoolingParticipant
    UnbindFrom_OnAcquiredFromPool(
        UPARAM(ref) FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnAcquired& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Bind To OnReleasedToPool")
    static FCk_Handle_ObjectPoolingParticipant
    BindTo_OnReleasedToPool(
        UPARAM(ref) FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnReleased& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Unbind From OnReleasedToPool")
    static FCk_Handle_ObjectPoolingParticipant
    UnbindFrom_OnReleasedToPool(
        UPARAM(ref) FCk_Handle_ObjectPoolingParticipant& InParticipant,
        const FCk_Delegate_ObjectPoolingParticipant_OnReleased& InDelegate);

    // ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Request Unbind All")
    static void
    Request_UnbindAll(
        UPARAM(ref) FCk_Handle_ObjectPoolingParticipant& InParticipant,
        UObject* InBoundObject);

    // ----

    // Per-instance veto read by the pooling subsystem at release time. False = the instance is
    // destroyed instead of stored. Flip it back to true before the next release to re-enable pooling
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Set Can Be Pooled")
    static FCk_Handle_ObjectPoolingParticipant
    Set_CanBePooled(
        UPARAM(ref) FCk_Handle_ObjectPoolingParticipant& InParticipant,
        bool InCanBePooled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectPooling",
              DisplayName = "[Ck][ObjectPooling] Get Can Be Pooled")
    static bool
    Get_CanBePooled(
        const FCk_Handle_ObjectPoolingParticipant& InParticipant);

public:
    // Subsystem-side plumbing (not UFUNCTIONs): reflection-scan InObject for
    // FCk_Handle_ObjectPoolingParticipant properties (IncludeSuper) and fire the hook on each.
    // Objects without a participant no-op

    static auto
    Broadcast_AcquiredFromPool_OnObject(
        UObject* InObject,
        const FInstancedStruct& InPerUseParams) -> void;

    static auto
    Broadcast_ReleasedToPool_OnObject(
        UObject* InObject) -> void;

    // True when the object has NO participant (opt-in only gates instances that declared one).
    // With multiple participants, ANY _CanBePooled == false vetoes
    static auto
    Get_CanBePooled_OnObject(
        const UObject* InObject) -> bool;

    static auto
    Has_Participant(
        const UObject* InObject) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
