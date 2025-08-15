#pragma once

#include "CkCore/Object/CkWorldContextObject.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CoreMinimal.h"

#include "CkEntityScript.generated.h"

// -----------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
}

// -----------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityScript_ConstructionFlow : uint8
{
    // Construction is finished. BeginPlay can be called
    Finished UMETA(DisplayName = "Finish Construction"),

    // Construction is still ongoing. ContinueConstruction event will be called and the user will need
    // to manually invoke FinishConstruction in order for BeginPlay to be called
    Continue UMETA(DisplayName = "Continue Construction")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityScript_ConstructionFlow);

// -----------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityScript_InstancingPolicy : uint8
{
    // This EntityScript is never instanced. Any entity that uses the EntityScript is operating on the CDO
    NotInstanced UMETA(DisplayName = "Not Instanced (uses CDO)"),

    // A new instance of the EntityScript is made every entity that exists.
    InstancedPerEntity,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityScript_InstancingPolicy);

// -----------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_EntityScript_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;
    friend class UCk_Utils_EntityScript_UE;

public:
    [[nodiscard]]
    virtual auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow;

    virtual auto
    ContinueConstruction(
        FCk_Handle InHandle) -> void;

    virtual auto
    BeginPlay() -> void;

    virtual auto
    EndPlay() -> void;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "ConstructionScript")
    ECk_EntityScript_ConstructionFlow
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "ContinueConstruction")
    void
    DoContinueConstruction(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "BeginPlay")
    void
    DoBeginPlay(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "EndPlay")
    void
    DoEndPlay(
        FCk_Handle InHandle);

public:
    auto
    GetPrimaryAssetId() const -> FPrimaryAssetId override;

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|EntityScript",
        DisplayName = "[Ck][EntityScript] Get Script Entity",
        meta = (CompactNodeTitle="ScriptEntity", Keywords="this, associated", HideSelfPin = true))
    FCk_Handle
    DoGet_ScriptEntity() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|EntityScript",
              DisplayName = "[Ck][EntityScript] Finish Construction",
              meta = (CompactNodeTitle="✔Constructed", HideSelfPin = true, Keywords = "ongoing"))
    void
    DoFinishConstruction();

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|EntityScript",
              DisplayName = "[Ck][EntityScript] Deactivate On EndPlay",
              meta = (CompactNodeTitle="🛑", HideSelfPin = true, Keywords = "register, track, stop"))
    void
    DoRequest_DeactivateTaskOnEndPlay(
        class UObject* InTask);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess))
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess, InvalidEnumValues = "NotInstanced"))
    ECk_EntityScript_InstancingPolicy _InstancingPolicy = ECk_EntityScript_InstancingPolicy::InstancedPerEntity;

    UPROPERTY(Transient)
    FCk_Handle _AssociatedEntity;

    UPROPERTY(EditDefaultsOnly,
        Category="Developer Settings", AssetRegistrySearchable, AdvancedDisplay,
        meta = (AllowPrivateAccess = true))
    FName _AssetRegistryCategory = TEXT("CkEntityScript");

private:
    TArray<TWeakObjectPtr<class UObject>> _TasksToDeactivate;

public:
    CK_PROPERTY_GET(_Replication);
    CK_PROPERTY_GET(_InstancingPolicy);
};

// ----------------------------------------------------------------------------------------------------------