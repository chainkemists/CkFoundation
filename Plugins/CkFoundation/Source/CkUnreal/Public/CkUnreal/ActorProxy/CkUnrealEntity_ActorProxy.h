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
//public:
//    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;
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
//    UFUNCTION()
//    void OnRep_ObjectReplicator(UCk_ObjectReplicator_ActorComponent_UE* InObjectReplicator);
//
//public:
//    UPROPERTY(ReplicatedUsing = OnRep_ObjectReplicator, meta = (AllowPrivateAccess = true))
//    class UCk_ObjectReplicator_ActorComponent_UE* _ObjectReplicator;
//
//    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
//    TObjectPtr<UCk_UnrealEntity_Base_PDA> _UnrealEntity;
//
//#if WITH_EDITORONLY_DATA
//    UPROPERTY(Transient)
//    TObjectPtr<UChildActorComponent> _ChildActorComponent;
//#endif
//};
//
//// --------------------------------------------------------------------------------------------------------------------
