#include "CkDestructibleActor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include "CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include "CkPhysics/GeometryCollection/CkDestructibleAnchor.h"

#include <Field/FieldSystemObjects.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_Destructible::
    ACk_Destructible(
        const FObjectInitializer& InObjectInitializer)
{
    // The following settings are crucial for the replication of the geometry collection
    // TODO: We definitely do NOT want destructible Actors to always be relevant. For now, we just want things to work.
    bAlwaysRelevant = false;
    bReplicates = false;
    bNetLoadOnClient = false;
    SetReplicatingMovement(false);

    // Optimized network settings to reduce the network load of replicated destruction
    SetNetUpdateFrequency(250.0f);
    SetMinNetUpdateFrequency( 10.0f);
    NetPriority = 0.01f; // destruction should be the lowest priority

    bReplicateUsingRegisteredSubObjectList = true;

    _GeometryCollection = CreateDefaultSubobject<UCk_GeometryCollectionComponent>(TEXT("Ck_GeometryCollection"));
    _GeometryCollection->Request_EnableAsyncPhysics();
    _GeometryCollection->SetupAttachment(GetRootComponent());

    _UniformKinematic = CreateDefaultSubobject<UCk_UniformKinematic>(TEXT("Ck_KinematicUniformInteger"));
}

auto
    ACk_Destructible::
    OnConstruction(
        const FTransform& Transform)
    -> void
{
    DoRequest_DeleteAllFieldNodes();
    GetFieldSystemComponent()->ResetFieldSystem();

    auto Anchors = TArray<UCk_DestructibleAnchor_ActorComponent_UE*>{};
    GetComponents(Anchors);

    for (const auto Anchor : Anchors)
    {
        const auto& WorldTransform = Anchor->GetComponentToWorld();

        switch(Anchor->Get_FieldShape())
        {
            case ECk_FieldShape::Box:
            {
                auto Falloff = Cast<UCk_BoxFalloff>(this->AddComponentByClass(UCk_BoxFalloff::StaticClass(), true, {}, false));
                Falloff->SetBoxFalloff(1.0f, 0.0f, 1.0f, 0.0f, WorldTransform, Field_FallOff_None);

                auto CullingField = Cast<UCk_CullingField>(this->AddComponentByClass(UCk_CullingField::StaticClass(), true, {}, false));
                CullingField->SetCullingField(Falloff, _UniformKinematic, Field_Culling_Outside);

                GetFieldSystemComponent()->AddFieldCommand(true, Field_DynamicState, {}, CullingField);

                break;
            }
            case ECk_FieldShape::Sphere:
            {
                auto Falloff = Cast<URadialFalloff>(this->AddComponentByClass(URadialFalloff::StaticClass(), true, {}, false));
                Falloff->SetRadialFalloff(1.0f, 0.0f, 1.0f, 0.0f,
                    Anchor->GetComponentScale().Component(0), WorldTransform.GetLocation(), Field_FallOff_None);

                auto CullingField = Cast<UCk_CullingField>(this->AddComponentByClass(UCk_CullingField::StaticClass(), true, {}, false));
                CullingField->SetCullingField(Falloff, _UniformKinematic, Field_Culling_Outside);

                GetFieldSystemComponent()->AddFieldCommand(true, Field_DynamicState, {}, CullingField);

                break;
            }
            case ECk_FieldShape::Plane:
            {
                // not yet supported
                CK_TRIGGER_ENSURE(TEXT("Plane FieldShape NOT yet supported for CK Destructibles"));
                break;
            }
        }
    }

    auto GCs = TArray<UCk_GeometryCollectionComponent*>{};
    GetComponents(GCs);

    for (const auto GC : GCs)
    {
        GC->InitializationFields.Empty();
        GC->InitializationFields.Emplace(this);
    }

    Super::OnConstruction(Transform);
}

auto
    ACk_Destructible::
    DoRequest_DeleteAllFieldNodes() const
    -> void
{
    auto AllComponents = GetComponents();
    auto ComponentsToDelete = TArray<UActorComponent*>{};
    for (const auto Component : AllComponents)
    {
        if (Component != _UniformKinematic && Cast<UFieldNodeBase>(Component))
        {
            ComponentsToDelete.Emplace(Component);
        }
    }

    for (const auto Component : ComponentsToDelete)
    {
        Component->DestroyComponent();
    }
}

// --------------------------------------------------------------------------------------------------------------------
