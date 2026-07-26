#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkJolt/Character/CkJoltCharacter_Fragment.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"

#include "CkJoltCharacter_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_JoltCharacter"))
class CKJOLT_API UCk_Utils_JoltCharacter_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltCharacter_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_JoltCharacter);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Add New JoltCharacter")
    static FCk_Handle_JoltCharacter
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_JoltCharacter_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|JoltCharacter",
        DisplayName="[Ck][JoltCharacter] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_JoltCharacter
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|JoltCharacter",
        DisplayName="[Ck][JoltCharacter] Handle -> JoltCharacter Handle",
        meta = (CompactNodeTitle = "<AsJoltCharacter>", BlueprintAutocast))
    static FCk_Handle_JoltCharacter
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid JoltCharacter Handle",
        Category = "Ck|Utils|JoltCharacter",
        meta = (CompactNodeTitle = "INVALID_JoltCharacterHandle", Keywords = "make"))
    static FCk_Handle_JoltCharacter
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Get Ground State")
    static ECk_JoltCharacter_GroundState
    Get_GroundState(
        const FCk_Handle_JoltCharacter& InJoltCharacter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Get Ground Normal")
    static FVector
    Get_GroundNormal(
        const FCk_Handle_JoltCharacter& InJoltCharacter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Get Ground Velocity")
    static FVector
    Get_GroundVelocity(
        const FCk_Handle_JoltCharacter& InJoltCharacter);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Request Move")
    static FCk_Handle_JoltCharacter
    Request_Move(
        UPARAM(ref) FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Request_JoltCharacter_Move& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Request Jump")
    static FCk_Handle_JoltCharacter
    Request_Jump(
        UPARAM(ref) FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Request_JoltCharacter_Jump& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName="[Ck][JoltCharacter] Request Teleport")
    static FCk_Handle_JoltCharacter
    Request_Teleport(
        UPARAM(ref) FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Request_JoltCharacter_Teleport& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName = "[Ck][JoltCharacter] Bind To OnGroundStateChanged")
    static FCk_Handle_JoltCharacter
    BindTo_OnJoltCharacterGroundStateChanged(
        UPARAM(ref) FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Delegate_JoltCharacter_OnGroundStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltCharacter",
              DisplayName = "[Ck][JoltCharacter] Unbind From OnGroundStateChanged")
    static FCk_Handle_JoltCharacter
    UnbindFrom_OnJoltCharacterGroundStateChanged(
        UPARAM(ref) FCk_Handle_JoltCharacter& InJoltCharacter,
        const FCk_Delegate_JoltCharacter_OnGroundStateChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
