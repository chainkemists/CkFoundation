#pragma once

#include "CkCamera/GameplayCamera/CkGameplayCamera_Fragment.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkGameplayCamera_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_GameplayCamera"))
class CKCAMERA_API UCk_Utils_GameplayCamera_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayCamera_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_GameplayCamera);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][GameplayCamera] Add")
    static FCk_Handle_GameplayCamera
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_GameplayCamera_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|GameplayCamera",
        DisplayName = "[Ck][GameplayCamera] Has")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|GameplayCamera",
        DisplayName = "[Ck][GameplayCamera] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_GameplayCamera
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|GameplayCamera",
        DisplayName = "[Ck][GameplayCamera] Handle -> GameplayCamera Handle",
        meta = (CompactNodeTitle = "<AsGameplayCamera>", BlueprintAutocast))
    static FCk_Handle_GameplayCamera
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid GameplayCamera Handle",
        Category = "Ck|Utils|GameplayCamera",
        meta = (CompactNodeTitle = "INVALID_GameplayCameraHandle", Keywords = "make"))
    static FCk_Handle_GameplayCamera
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][GameplayCamera] Request Add Modifier")
    static FCk_Handle_GameplayCamera
    Request_AddModifier(
        UPARAM(ref) FCk_Handle_GameplayCamera& InCamera,
        const FCk_Request_GameplayCamera_AddModifier& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][GameplayCamera] Request Remove Modifier")
    static FCk_Handle_GameplayCamera
    Request_RemoveModifier(
        UPARAM(ref) FCk_Handle_GameplayCamera& InCamera,
        const FCk_Request_GameplayCamera_RemoveModifier& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
