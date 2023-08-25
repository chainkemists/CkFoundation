//#pragma once
//
//#include "CkEcs/Handle/CkHandle.h"
//
//#include "CkMacros/CkMacros.h"
//
//#include "CkUnreal/Entity/CkUnrealEntity_Fragment_Params.h"
//
//#include "CkUnrealEntity_ActorProxy.generated.h"
//
//// --------------------------------------------------------------------------------------------------------------------
//
//UCLASS(Abstract, Blueprintable, BlueprintType)
//class CKUNREAL_API ACk_UnrealEntity_ActorProxy_UE : public AActor
//{
//    GENERATED_BODY()
//
//public:
//    CK_GENERATED_BODY(ACk_UnrealEntity_ActorProxy_UE);
//
//public:
//    ACk_UnrealEntity_ActorProxy_UE();
//
//private:
//    auto DoInvokeOnEntityCreated(const FCk_Handle& InCreatedEntity) -> void;
//
//protected:
//    UFUNCTION(BlueprintImplementableEvent)
//    FCk_Handle
//    Get_TransientHandle() const;
//
//    UFUNCTION(BlueprintImplementableEvent)
//    void
//    OnEntityCreated(const FCk_Handle& InCreatedEntity) const;
//
//protected:
//#if WITH_EDITOR
//    virtual auto PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
//#endif
//
//public:
//    virtual auto BeginPlay() -> void override;
//
//public:
//    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
//    TSubclassOf<AActor> _ActorToSpawn;
//
//#if WITH_EDITORONLY_DATA
//    UPROPERTY(Transient)
//    TObjectPtr<UChildActorComponent> _ChildActorComponent;
//#endif
//};
//
//// --------------------------------------------------------------------------------------------------------------------
