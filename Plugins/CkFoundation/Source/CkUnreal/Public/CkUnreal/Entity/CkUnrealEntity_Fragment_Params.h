#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkTypes/DataAsset/CkDataAsset.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkUnrealEntity_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, NotBlueprintable, EditInlineNew)
class CKUNREAL_API UCk_UnrealEntity_Base_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_UnrealEntity_Base_PDA);

public:
    auto Build(FCk_Registry& InRegistry) const -> FCk_Handle;
    auto Get_EntityConstructionScript() const -> class UCk_UnrealEntity_ConstructionScript_PDA*;

protected:
    virtual auto DoBuild(FCk_Handle InHandle) const -> void;
    virtual auto DoGet_EntityConstructionScript() const -> UCk_UnrealEntity_ConstructionScript_PDA*;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKUNREAL_API UCk_UnrealEntity_PDA : public UCk_UnrealEntity_Base_PDA
{
    GENERATED_BODY()

    friend class ACk_UnrealEntity_ActorProxy_UE;

public:
    CK_GENERATED_BODY(UCk_UnrealEntity_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const -> UCk_UnrealEntity_ConstructionScript_PDA* override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<class UCk_UnrealEntity_ConstructionScript_PDA> _EntityConstructionScript;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUNREAL_API FCk_Request_UnrealEntity_Spawn
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_UnrealEntity_Spawn);

public:
    using PostSpawnFunc = TFunction<void(const FCk_Handle&)>;

public:
    FCk_Request_UnrealEntity_Spawn() = default;

    explicit FCk_Request_UnrealEntity_Spawn(
        const UCk_UnrealEntity_Base_PDA* InEntity);

    FCk_Request_UnrealEntity_Spawn(
        const UCk_UnrealEntity_Base_PDA* InEntity,
        PostSpawnFunc InPostSpawnFunc);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true))
    const UCk_UnrealEntity_Base_PDA* _UnrealEntity = nullptr;

private:
    PostSpawnFunc _PostSpawnFunc = [](const FCk_Handle&){};

public:
    CK_PROPERTY_GET(_UnrealEntity);
    CK_PROPERTY_GET(_PostSpawnFunc);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUNREAL_API FCk_Payload_UnrealEntity_EntityCreated
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
