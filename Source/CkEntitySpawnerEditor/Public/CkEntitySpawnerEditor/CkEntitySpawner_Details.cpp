#include "CkEntitySpawner_Details.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <IDetailPropertyRow.h>
#include <PropertyHandle.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FCk_EntitySpawner_Details::
        MakeInstance()
        -> TSharedRef<IDetailCustomization>
    {
        return MakeShareable(new FCk_EntitySpawner_Details);
    }

    auto
        FCk_EntitySpawner_Details::
        CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder)
        -> void
    {
        auto SelectedObjects = TArray<TWeakObjectPtr<UObject>>{};
        DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

        if (SelectedObjects.Num() != 1)
        { return; }

        auto Spawner = Cast<ACk_EntitySpawner_UE>(SelectedObjects[0].Get());
        if (ck::Is_NOT_Valid(Spawner))
        { return; }

        // Literal FName: _EntityScript is private, GET_MEMBER_NAME_CHECKED can't see it from here.
        const auto EntityScriptHandle = DetailBuilder.GetProperty(FName{TEXT("_EntityScript")});

        if (NOT EntityScriptHandle->IsValidHandle())
        { return; }

        uint32 NumChildren = 0;
        EntityScriptHandle->GetNumChildren(NumChildren);

        for (auto ChildIndex = uint32{0}; ChildIndex < NumChildren; ++ChildIndex)
        {
            auto ChildHandle = EntityScriptHandle->GetChildHandle(ChildIndex);
            if (NOT ChildHandle.IsValid())
            { continue; }

            const auto* ChildProperty = ChildHandle->GetProperty();
            if (ck::Is_NOT_Valid(ChildProperty))
            { continue; }

            auto SpawnerWeak = TWeakObjectPtr<ACk_EntitySpawner_UE>{Spawner};
            const auto ChildNameCaptured = ChildProperty->GetFName();

            auto IsEnabled = TAttribute<bool>::Create([SpawnerWeak, ChildNameCaptured]()
            {
                const auto* SpawnerResolved = SpawnerWeak.Get();
                if (ck::Is_NOT_Valid(SpawnerResolved))
                { return true; }

                const auto& BoundName =
                    SpawnerResolved->Get_InjectActorTransformToScriptProperty().Get_PropertyName();

                return BoundName != ChildNameCaptured;
            });

            if (auto* PropertyRow = DetailBuilder.EditDefaultProperty(ChildHandle);
                ck::IsValid(PropertyRow, ck::IsValid_Policy_NullptrOnly{}))
            {
                PropertyRow->IsEnabled(IsEnabled);
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
