#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/DebugFeatureFlags/CkDebugFeatureFlags.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

#include "CkDebugFeatureFlags_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// BP/AS surface over ck::debug_feature_flags (see CkDebugFeatureFlags.h for the contract).
// The registry is resolved from any live handle in the target world.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_DebugFeatureFlags_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DebugFeatureFlags_UE);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][DebugFeatureFlags] Request Enable",
              Category = "Ck|Utils|DebugFeatureFlags")
    static void
    Request_Enable(
        const FCk_Handle& InAnyHandleInWorld);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][DebugFeatureFlags] Request Disable",
              Category = "Ck|Utils|DebugFeatureFlags")
    static void
    Request_Disable(
        const FCk_Handle& InAnyHandleInWorld);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][DebugFeatureFlags] Get Is Enabled",
              Category = "Ck|Utils|DebugFeatureFlags")
    static bool
    Get_IsEnabled(
        const FCk_Handle& InAnyHandleInWorld);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][DebugFeatureFlags] Get Flags",
              Category = "Ck|Utils|DebugFeatureFlags")
    static int64
    Get_Flags(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][DebugFeatureFlags] Get Has Feature",
              Category = "Ck|Utils|DebugFeatureFlags")
    static bool
    Get_HasFeature(
        const FCk_Handle& InHandle,
        FName InFeatureId);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][DebugFeatureFlags] Get Bit Index",
              Category = "Ck|Utils|DebugFeatureFlags")
    static int32
    Get_BitIndex(
        FName InFeatureId);
};
