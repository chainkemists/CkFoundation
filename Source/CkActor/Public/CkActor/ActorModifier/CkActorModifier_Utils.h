#pragma once
#include "CkActorModifier_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <InstancedStruct.h>

#include "CkActorModifier_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKACTOR_API UCk_Utils_ActorModifier_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorModifier_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier",
        meta = (AutoCreateRefTerm = "InOptionalPayload, OnActorSpawned",
                DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject", AdvancedDisplay = "6"),
        DisplayName = "[Ck][ActorModifier] Request Spawn Actor (Replicated)")
    static void
    Request_SpawnActor_Replicated(
        UObject* InWorldContextObject,
        TSubclassOf<AActor> InActorClass,
        const FTransform& InSpawnTransform,
        ESpawnActorCollisionHandlingMethod InCollisionHandling,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorSpawned& OnActorSpawned,
        FName InNonUniqueActorName,
        FString InActorLabel);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier",
        meta = (AutoCreateRefTerm = "InOptionalPayload, OnActorSpawned",
                DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject", AdvancedDisplay = "6"),
        DisplayName = "[Ck][ActorModifier] Request Spawn Actor (Not Replicated)")
    static void
    Request_SpawnActor_NotReplicated(
        UObject* InWorldContextObject,
        TSubclassOf<AActor> InActorClass,
        const FTransform& InSpawnTransform,
        ESpawnActorCollisionHandlingMethod InCollisionHandling,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorSpawned& OnActorSpawned,
        FName InNonUniqueActorName,
        FString InActorLabel);

    static void
    Request_SpawnActor(
        const FCk_Handle& InHandle,
        const FCk_Request_ActorModifier_SpawnActor& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorSpawned& OnActorSpawned);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier",
        meta = (AutoCreateRefTerm = "InOptionalPayload, OnActorComponentAdded",
                DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject", AdvancedDisplay = "5"),
        DisplayName = "[Ck][ActorModifier] Request Add Actor Component")
    static void
    Request_AddActorComponent(
        UObject* InWorldContextObject,
        TSubclassOf<UActorComponent> InComponentClass,
        USceneComponent* InComponentParent,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& OnActorComponentAdded,
        FName InAttachmentSocket,
        ECk_ActorComponent_AttachmentPolicy InAttachmentPolicy = ECk_ActorComponent_AttachmentPolicy::Attach,
        bool InIsUnique = true);

    static void
    Request_AddActorComponent(
        const FCk_Handle& InHandle,
        const FCk_Request_ActorModifier_AddActorComponent& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& OnActorComponentAdded);
};

// --------------------------------------------------------------------------------------------------------------------
