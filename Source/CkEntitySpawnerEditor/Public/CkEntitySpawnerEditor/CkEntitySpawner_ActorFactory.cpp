#include "CkEntitySpawner_ActorFactory.h"

#include "CkEntitySpawnerEditor/CkEntitySpawner_IconHelper.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkEntitySpawnerActorFactory"

UCk_EntitySpawner_ActorFactory_UE::
    UCk_EntitySpawner_ActorFactory_UE(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    DisplayName = LOCTEXT("CkEntitySpawner_DisplayName", "Ck Entity Spawner");
    NewActorClass = ACk_EntitySpawner_UE::StaticClass();
    bUseSurfaceOrientation = false;
    bShowInEditorQuickMenu = false;
}

auto
    UCk_EntitySpawner_ActorFactory_UE::
    CanCreateActorFrom(
        const FAssetData& InAssetData,
        FText& OutErrorMsg)
    -> bool
{
    if (NOT InAssetData.IsValid())
    {
        OutErrorMsg = LOCTEXT("CkEntitySpawner_InvalidAsset", "Invalid asset data");
        return false;
    }

    if (ck::Is_NOT_Valid(ResolveEntityScriptClass(InAssetData)))
    {
        OutErrorMsg = LOCTEXT("CkEntitySpawner_NotEntityScript",
            "Asset does not derive from UCk_EntityScript_UE");
        return false;
    }

    return true;
}

auto
    UCk_EntitySpawner_ActorFactory_UE::
    PostSpawnActor(
        UObject* InAsset,
        AActor* InNewActor)
    -> void
{
    Super::PostSpawnActor(InAsset, InNewActor);

    auto Spawner = Cast<ACk_EntitySpawner_UE>(InNewActor);

    if (ck::Is_NOT_Valid(Spawner))
    { return; }

    const auto AssetData = FAssetData{InAsset};
    const auto EntityScriptClass = ResolveEntityScriptClass(AssetData);

    if (ck::Is_NOT_Valid(EntityScriptClass))
    { return; }

    Spawner->EditorOnly_InitializeEntityScript(EntityScriptClass);

    // Construct the editor-only ECS entity right after the script UObject is set up. The
    // editor subsystem drives Construct() from the graph tick, so the entity becomes "live"
    // on the next editor frame.
    Spawner->EditorOnly_RebuildEntity();

    FCk_EntitySpawner_IconHelper::ApplyToActor(Spawner);
}

auto
    UCk_EntitySpawner_ActorFactory_UE::
    ResolveEntityScriptClass(
        const FAssetData& InAssetData)
    -> TSubclassOf<UCk_EntityScript_UE>
{
    if (NOT InAssetData.IsValid())
    { return nullptr; }

    const auto BaseClass = UCk_EntityScript_UE::StaticClass();

    if (auto Asset = InAssetData.GetAsset();
        ck::IsValid(Asset))
    {
        if (auto AsClass = Cast<UClass>(Asset);
            ck::IsValid(AsClass) && AsClass->IsChildOf(BaseClass) &&
            NOT AsClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            return AsClass;
        }

        if (auto Blueprint = Cast<UBlueprint>(Asset);
            ck::IsValid(Blueprint) && ck::IsValid(Blueprint->GeneratedClass) &&
            Blueprint->GeneratedClass->IsChildOf(BaseClass) &&
            NOT Blueprint->GeneratedClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
        {
            return Blueprint->GeneratedClass.Get();
        }
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
