#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Types/DataAsset/CkDataAsset.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkUnrealEntity_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, NotBlueprintable, EditInlineNew)
class CKNET_API UCk_UnrealEntity_Base_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_UnrealEntity_Base_PDA);

public:
    auto Build(FCk_Handle InEntity) const -> void;

    [[nodiscard]]
    auto Get_EntityConstructionScript() const -> class UCk_Entity_ConstructionScript_PDA*;

protected:
    virtual auto DoBuild(FCk_Handle InHandle) const -> void;

    [[nodiscard]]
    virtual auto DoGet_EntityConstructionScript() const -> class UCk_Entity_ConstructionScript_PDA*;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKNET_API UCk_UnrealEntity_PDA : public UCk_UnrealEntity_Base_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_UnrealEntity_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const -> class UCk_Entity_ConstructionScript_PDA* override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<class UCk_Entity_ConstructionScript_PDA> _EntityConstructionScript;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKNET_API UCk_UnrealEntity_WithActor_PDA : public UCk_UnrealEntity_Base_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_UnrealEntity_WithActor_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const ->  class UCk_Entity_ConstructionScript_PDA* override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<class UCk_UnrealEntity_ConstructionScript_WithTransform_PDA> _EntityConstructionScript;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_Request_UnrealEntity_Spawn
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_UnrealEntity_Spawn);

public:
    using PreBuildFunc = TFunction<void(const FCk_Handle&)>;
    using PostSpawnFunc = TFunction<void(const FCk_Handle&)>;

public:
    FCk_Request_UnrealEntity_Spawn() = default;

    explicit FCk_Request_UnrealEntity_Spawn(
        const UCk_UnrealEntity_Base_PDA* InEntity);

    FCk_Request_UnrealEntity_Spawn(
        const UCk_UnrealEntity_Base_PDA* InEntity,
        PreBuildFunc InPreBuildFunc,
        PostSpawnFunc InPostSpawnFunc);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true))
    const UCk_UnrealEntity_Base_PDA* _UnrealEntity = nullptr;

    // TODO:
    // - add an owner
    // - then, in the construction script, have an enum REMOTE, LOCAL, ALL
    // - call the construction script for ALL and then for REMOTE and LOCAL

private:
    PreBuildFunc _PreBuildFunc = [](const FCk_Handle&){};
    PostSpawnFunc _PostSpawnFunc = [](const FCk_Handle&){};

public:
    CK_PROPERTY_GET(_UnrealEntity);
    CK_PROPERTY_GET(_PreBuildFunc);
    CK_PROPERTY_GET(_PostSpawnFunc);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_Payload_UnrealEntity_EntityCreated
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_UnrealEntity_EntityCreated);

public:
    FCk_Payload_UnrealEntity_EntityCreated() = default;
    FCk_Payload_UnrealEntity_EntityCreated(
        FCk_Handle InHandle,
        FCk_Handle InCreatedEntity);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _CreatedEntity;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_CreatedEntity);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_UnrealEntity_OnEntityCreated,
    const FCk_Payload_UnrealEntity_EntityCreated&,
    InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_UnrealEntity_OnEntityCreated_MC,
    const FCk_Payload_UnrealEntity_EntityCreated&,
    InPayload);

// --------------------------------------------------------------------------------------------------------------------
