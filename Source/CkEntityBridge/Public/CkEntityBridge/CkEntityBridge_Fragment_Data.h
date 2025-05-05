#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Types/DataAsset/CkDataAsset.h"
#include "CkEcs/Registry/CkRegistry.h"

#include <InstancedStruct.h>
#include <GameplayTagContainer.h>

#include "CkEntityBridge_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, NotBlueprintable, EditInlineNew)
class CKENTITYBRIDGE_API UCk_EntityBridge_Config_Base_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_EntityBridge_Config_Base_PDA);

public:
    auto
    Build(
        const FCk_Handle& InEntity,
        const UObject* InOptionalObjectConstructionScript = nullptr) const -> void;

    [[nodiscard]]
    auto
    Get_EntityConstructionScript() const -> class UCk_Entity_ConstructionScript_PDA*;

protected:
    virtual auto DoBuild(
        FCk_Handle InHandle,
        const UObject* InOptionalObjectConstructionScript) const -> void;

    [[nodiscard]]
    virtual auto DoGet_EntityConstructionScript() const -> class UCk_Entity_ConstructionScript_PDA*;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKENTITYBRIDGE_API UCk_EntityBridge_Config_PDA : public UCk_EntityBridge_Config_Base_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_EntityBridge_Config_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const -> class UCk_Entity_ConstructionScript_PDA* override;

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<class UCk_Entity_ConstructionScript_PDA> _EntityConstructionScript;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKENTITYBRIDGE_API UCk_EntityBridge_Config_WithActor_PDA : public UCk_EntityBridge_Config_Base_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_EntityBridge_Config_WithActor_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const ->  class UCk_Entity_ConstructionScript_PDA* override;

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<class UCk_Entity_ConstructionScript_WithTransform_PDA> _EntityConstructionScript;

public:
    CK_PROPERTY_GET(_EntityConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYBRIDGE_API FCk_Request_EntityBridge_SpawnEntity
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityBridge_SpawnEntity);

public:
    using PreBuildFunc = TFunction<void(FCk_Handle&)>;
    using PostSpawnFunc = TFunction<void(FCk_Handle&)>;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true))
    const UCk_EntityBridge_Config_Base_PDA* _EntityConfig = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    UObject* _OptionalObjectConstructionScript = nullptr;

    // TODO:
    // - add an owner
    // - then, in the construction script, have an enum REMOTE, LOCAL, ALL
    // - call the construction script for ALL and then for REMOTE and LOCAL

private:
    PreBuildFunc _PreBuildFunc = [](const FCk_Handle&){};
    PostSpawnFunc _PostSpawnFunc = [](const FCk_Handle&){};

public:
    CK_PROPERTY_GET(_EntityConfig);
    CK_PROPERTY(_OptionalObjectConstructionScript);
    CK_PROPERTY(_PreBuildFunc);
    CK_PROPERTY(_PostSpawnFunc);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityBridge_SpawnEntity, _EntityConfig);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_EntityBridge_OnEntitySpawned,
    const FCk_Handle&, InEntitySpawned,
    const FInstancedStruct&, InOptionalPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_EntityBridge_OnEntitySpawned_MC,
    const FCk_Handle&, InEntitySpawned,
    const FInstancedStruct&, InOptionalPayload);

// --------------------------------------------------------------------------------------------------------------------
