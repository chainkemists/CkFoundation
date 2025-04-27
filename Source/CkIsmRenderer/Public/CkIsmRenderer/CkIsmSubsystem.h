#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include "CkIsmSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKISMRENDERER_API UCk_EntityBridge_IsmRenderer_UE : public UCk_EntityBridge_ActorComponent_UE
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKISMRENDERER_API ACk_IsmRenderer_Actor_UE final : public AActor, public ICk_Entity_ConstructionScript_Interface
{
    GENERATED_BODY()

public:
    friend class UCk_IsmRenderer_Subsystem_UE;

public:
    CK_GENERATED_BODY(ACk_IsmRenderer_Actor_UE);

public:
    ACk_IsmRenderer_Actor_UE();

    void
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<USceneComponent> _RootNode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<UCk_EntityBridge_IsmRenderer_UE> _EntityBridge;

    UPROPERTY()
    const UCk_IsmRenderer_Data* _RenderData = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_IsmRenderer")
class CKISMRENDERER_API UCk_IsmRenderer_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IsmRenderer_Subsystem_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|IsmRenderer")
    ACk_IsmRenderer_Actor_UE*
    GetOrCreate_IsmRenderer(
        const UCk_IsmRenderer_Data* InDataAsset);

private:
    UPROPERTY()
    TMap<const UCk_IsmRenderer_Data*, TObjectPtr<ACk_IsmRenderer_Actor_UE>> _IsmRenderers;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKISMRENDERER_API UCk_Utils_IsmRenderer_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_Utils_IsmRenderer_Subsystem_UE;

public:
    static auto
    GetOrCreate_IsmRenderer(
        const UWorld* InWorld,
        const UCk_IsmRenderer_Data* InDataAsset) -> ACk_IsmRenderer_Actor_UE*;
};

// --------------------------------------------------------------------------------------------------------------------
