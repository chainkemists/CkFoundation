#include "CkEntitySpawnerEditor_Module.h"

#include "CkEntitySpawnerEditor/CkEntitySpawner_ActorFactory.h"
#include "CkEntitySpawnerEditor/CkEntitySpawner_Details.h"
#include "CkEntitySpawnerEditor/CkEntitySpawner_IconHelper.h"
#include "CkEntitySpawnerEditor/CkEntitySpawner_InjectTransform_Details.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"
#include "CkEntitySpawner/CkEntitySpawner_Settings.h"

#include "CkEntitySpawnerEditor_Log.h"

#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"

#include "ActorFactories/ActorFactory.h"
#include "AssetRegistry/AssetData.h"
#include "Components/BillboardComponent.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "IPlacementModeModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

#define LOCTEXT_NAMESPACE "FCkEntitySpawnerEditorModule"

namespace ck::entity_spawner_editor_internal
{
    static TStrongObjectPtr<UTexture2D> GEntitySpawnerIcon;

    static auto DoLoadIcon() -> UTexture2D*
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
        if (ck::Is_NOT_Valid(Plugin))
        {
            UE_LOG(CkEntitySpawnerEditor, Error, TEXT("CkFoundation plugin not found by IPluginManager"));
            return nullptr;
        }

        const auto IconPath = FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Resources"), TEXT("Editor"), TEXT("EntityIcon.png"));

        UE_LOG(CkEntitySpawnerEditor, Log, TEXT("Loading EntitySpawner icon from [%s]"), *IconPath);

        if (NOT FPaths::FileExists(IconPath))
        {
            UE_LOG(CkEntitySpawnerEditor, Error, TEXT("Icon file does NOT exist: [%s]"), *IconPath);
            return nullptr;
        }

        TArray<uint8> FileData;
        if (NOT FFileHelper::LoadFileToArray(FileData, *IconPath))
        {
            UE_LOG(CkEntitySpawnerEditor, Error, TEXT("Failed to read file bytes: [%s]"), *IconPath);
            return nullptr;
        }

        auto Texture = FImageUtils::ImportBufferAsTexture2D(FileData);

        if (Texture == nullptr)
        {
            UE_LOG(CkEntitySpawnerEditor, Error,
                TEXT("ImportBufferAsTexture2D returned null for [%s] (%d bytes)"),
                *IconPath, FileData.Num());
            return nullptr;
        }

        Texture->SRGB = true;
        Texture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
        Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
        Texture->LODGroup = TEXTUREGROUP_UI;
        Texture->NeverStream = true;
        Texture->UpdateResource();

        UE_LOG(CkEntitySpawnerEditor, Log, TEXT("Loaded EntitySpawner icon OK: size [%dx%d]"),
            Texture->GetSizeX(), Texture->GetSizeY());

        return Texture;
    }

    static auto DoApplyIconTo(AActor* InActor, UTexture2D* InTexture) -> void
    {
        auto Spawner = Cast<ACk_EntitySpawner_UE>(InActor);
        if (ck::Is_NOT_Valid(Spawner))
        { return; }

        UE_LOG(CkEntitySpawnerEditor, Log, TEXT("ApplyIconTo [%s]: texture [%s]"),
            *Spawner->GetName(),
            InTexture != nullptr ? *InTexture->GetName() : TEXT("NULL"));

        auto Sprite = Spawner->GetSpriteComponent();
        if (ck::Is_NOT_Valid(Sprite))
        {
            UE_LOG(CkEntitySpawnerEditor, Warning, TEXT("EntitySpawner [%s] has no SpriteComponent"),
                *Spawner->GetName());
            return;
        }

        Sprite->SetSprite(InTexture);

        const auto Settings = GetDefault<UCk_EntitySpawner_UserSettings_UE>();
        if (ck::IsValid(Settings))
        {
            constexpr auto DefaultScreenSize = 0.0025f;
            Sprite->ScreenSize = DefaultScreenSize * Settings->Get_IconScale();
            Sprite->SetRelativeLocation(FVector{0.0f, 0.0f, Settings->Get_IconVerticalOffset()});
        }

        Sprite->MarkRenderStateDirty();

        UE_LOG(CkEntitySpawnerEditor, Verbose, TEXT("Applied icon to EntitySpawner [%s]"), *Spawner->GetName());
    }

    // ----------------------------------------------------------------------------------------------------
    // Place Actors panel: one draggable item per opt-in
    // (UCk_EntityScript_UE::Get_ShowInPlaceActors) EntityScript class. No Blueprint wrapper is
    // needed because UCk_EntitySpawner_ActorFactory_UE accepts a bare EntityScript UClass as its asset.

    static auto Get_PlacementCategoryHandle() -> FName
    {
        static const FName Handle = TEXT("CkEntityScripts");
        return Handle;
    }

    static TArray<FPlacementModeID> GRegisteredPlaceableItemIDs;
    static bool GPlacementCategoryRegistered = false;

    static auto DoRegisterPlacementCategory() -> void
    {
        if (GPlacementCategoryRegistered)
        { return; }

        auto& PlacementMode = IPlacementModeModule::Get();

        constexpr auto SortOrder = 45;
        const auto CategoryInfo = FPlacementCategoryInfo
        {
            LOCTEXT("CkEntityScripts_DisplayName", "Ck Entity Scripts"),
            Get_PlacementCategoryHandle(),
            TEXT("PMCkEntityScripts"),
            SortOrder
        };

        GPlacementCategoryRegistered = PlacementMode.RegisterPlacementCategory(CategoryInfo);
    }

    static auto DoEnumeratePlaceableEntityScriptClasses() -> TArray<UClass*>
    {
        auto Result = TArray<UClass*>{};

        auto DerivedClasses = TArray<UClass*>{};
        constexpr auto Recursive = true;
        GetDerivedClasses(UCk_EntityScript_UE::StaticClass(), DerivedClasses, Recursive);

        for (auto* DerivedClass : DerivedClasses)
        {
            if (ck::Is_NOT_Valid(DerivedClass))
            { continue; }

            if (DerivedClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
            { continue; }

            if (UCk_Utils_Reflection_UE::Is_PlaceholderClass(DerivedClass))
            { continue; }

            const auto* CDO = Cast<UCk_EntityScript_UE>(DerivedClass->GetDefaultObject());
            if (ck::Is_NOT_Valid(CDO))
            { continue; }

            if (NOT CDO->Get_ShowInPlaceActors())
            { continue; }

            Result.Add(DerivedClass);
        }

        return Result;
    }

    static auto DoRefreshPlaceableEntityScripts() -> void
    {
        if (GEditor == nullptr)
        { return; }

        DoRegisterPlacementCategory();

        if (NOT GPlacementCategoryRegistered)
        { return; }

        auto& PlacementMode = IPlacementModeModule::Get();

        for (const auto& ItemID : GRegisteredPlaceableItemIDs)
        {
            PlacementMode.UnregisterPlaceableItem(ItemID);
        }
        GRegisteredPlaceableItemIDs.Reset();

        // The factory is registered with GEditor in DoPostEngineInit; FindActorFactoryByClass returns that
        // shared instance, so every placeable item reuses the same factory the content-browser drag uses.
        auto* Factory = GEditor->FindActorFactoryByClass(UCk_EntitySpawner_ActorFactory_UE::StaticClass());
        if (ck::Is_NOT_Valid(Factory))
        { return; }

        for (auto* EntityScriptClass : DoEnumeratePlaceableEntityScriptClasses())
        {
            const auto AssetData = FAssetData{EntityScriptClass};
            const auto Item = MakeShared<FPlaceableItem>(Factory, AssetData);

            if (const auto ItemID = PlacementMode.RegisterPlaceableItem(Get_PlacementCategoryHandle(), Item);
                ItemID.IsSet())
            {
                GRegisteredPlaceableItemIDs.Add(ItemID.GetValue());
            }
        }

        UE_LOG(CkEntitySpawnerEditor, Log,
            TEXT("Place Actors 'Ck Entity Scripts': registered %d placeable EntityScript item(s)"),
            GRegisteredPlaceableItemIDs.Num());
    }

    static auto DoUnregisterPlacement() -> void
    {
        if (NOT IPlacementModeModule::IsAvailable())
        {
            GRegisteredPlaceableItemIDs.Reset();
            GPlacementCategoryRegistered = false;
            return;
        }

        auto& PlacementMode = IPlacementModeModule::Get();

        for (const auto& ItemID : GRegisteredPlaceableItemIDs)
        {
            PlacementMode.UnregisterPlaceableItem(ItemID);
        }
        GRegisteredPlaceableItemIDs.Reset();

        if (GPlacementCategoryRegistered)
        {
            PlacementMode.UnregisterPlacementCategory(Get_PlacementCategoryHandle());
            GPlacementCategoryRegistered = false;
        }
    }
}

void FCkEntitySpawnerEditorModule::StartupModule()
{
    _PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
        this, &FCkEntitySpawnerEditorModule::DoPostEngineInit);

    _ObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(
        this, &FCkEntitySpawnerEditorModule::OnObjectsReplaced);

    auto& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyEditor.RegisterCustomPropertyTypeLayout(
        FCk_EntitySpawner_ScriptPropertyBinding::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(
            &ck::layout::FCk_EntitySpawner_InjectTransform_Details::MakeInstance));

    PropertyEditor.RegisterCustomClassLayout(
        ACk_EntitySpawner_UE::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(
            &ck::layout::FCk_EntitySpawner_Details::MakeInstance));

#if WITH_ANGELSCRIPT_CK
    // AS body-only edits (e.g. DoConstruct changes on script subclasses) hot-patch into the same
    // UClass*, so neither OnObjectsReplaced nor OnClassReload fires — only GetPostCompile catches
    // them. Coalesces into the existing _PendingSpawnerRebuild end-of-frame path.
    _PostAngelscriptCompileHandle = FAngelscriptCodeModule::GetPostCompile().AddLambda(
        [this]()
        {
            if (_PendingSpawnerRebuild || GEditor == nullptr)
            { return; }

            _PendingSpawnerRebuild = true;
            _EndFrameRebuildHandle = FCoreDelegates::OnEndFrame.AddRaw(
                this, &FCkEntitySpawnerEditorModule::OnEndFrame_RebuildSpawners);
        });
#endif
}

void FCkEntitySpawnerEditorModule::DoPostEngineInit()
{
    if (GEditor == nullptr)
    { return; }

    auto Factory = NewObject<UCk_EntitySpawner_ActorFactory_UE>(
        GetTransientPackage(), UCk_EntitySpawner_ActorFactory_UE::StaticClass());
    GEditor->ActorFactories.Add(Factory);

    if (auto IconTexture = ck::entity_spawner_editor_internal::DoLoadIcon();
        ck::IsValid(IconTexture))
    {
        ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Reset(IconTexture);

        DoApplyIconToAllInstances();

        if (GEngine != nullptr)
        {
            _LevelActorAddedHandle = GEngine->OnLevelActorAdded().AddRaw(
                this, &FCkEntitySpawnerEditorModule::OnLevelActorAdded);
        }

        _MapOpenedHandle = FEditorDelegates::OnMapOpened.AddLambda(
            [this](const FString&, bool)
            {
                DoApplyIconToAllInstances();
                DoRebuildAllEditorEntities();
            });
    }
    else
    {
        // Icon load failed but we still need rebuild-on-map-open behaviour so the editor entity
        // pipeline works even if the billboard is missing.
        if (GEngine != nullptr)
        {
            _LevelActorAddedHandle = GEngine->OnLevelActorAdded().AddRaw(
                this, &FCkEntitySpawnerEditorModule::OnLevelActorAdded);
        }

        _MapOpenedHandle = FEditorDelegates::OnMapOpened.AddLambda(
            [this](const FString&, bool) { DoRebuildAllEditorEntities(); });
    }

    ck::entity_spawner_editor_internal::DoRefreshPlaceableEntityScripts();
}

void FCkEntitySpawnerEditorModule::ShutdownModule()
{
    if (_PostEngineInitHandle.IsValid())
    { FCoreDelegates::OnPostEngineInit.Remove(_PostEngineInitHandle); }

    if (GEngine != nullptr && _LevelActorAddedHandle.IsValid())
    { GEngine->OnLevelActorAdded().Remove(_LevelActorAddedHandle); }

    if (_MapOpenedHandle.IsValid())
    { FEditorDelegates::OnMapOpened.Remove(_MapOpenedHandle); }

    if (_ObjectsReplacedHandle.IsValid())
    { FCoreUObjectDelegates::OnObjectsReplaced.Remove(_ObjectsReplacedHandle); }

    if (_EndFrameRebuildHandle.IsValid())
    { FCoreDelegates::OnEndFrame.Remove(_EndFrameRebuildHandle); }

    ck::entity_spawner_editor_internal::DoUnregisterPlacement();

    ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Reset();

    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyEditor.UnregisterCustomPropertyTypeLayout(
            FCk_EntitySpawner_ScriptPropertyBinding::StaticStruct()->GetFName());
        PropertyEditor.UnregisterCustomClassLayout(
            ACk_EntitySpawner_UE::StaticClass()->GetFName());
    }

#if WITH_ANGELSCRIPT_CK
    if (_PostAngelscriptCompileHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("AngelscriptCode"))
    {
        FAngelscriptCodeModule::GetPostCompile().Remove(_PostAngelscriptCompileHandle);
        _PostAngelscriptCompileHandle.Reset();
    }
#endif

    if (GEditor == nullptr)
    { return; }

    GEditor->ActorFactories.RemoveAll([](const UActorFactory* InFactory)
    {
        return ck::IsValid(InFactory) && InFactory->IsA<UCk_EntitySpawner_ActorFactory_UE>();
    });
}

void FCkEntitySpawnerEditorModule::DoApplyIconToAllInstances()
{
    auto IconTexture = ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Get();
    if (ck::Is_NOT_Valid(IconTexture) || GEditor == nullptr)
    { return; }

    for (const auto& WorldContext : GEditor->GetWorldContexts())
    {
        auto World = WorldContext.World();
        if (ck::Is_NOT_Valid(World))
        { continue; }

        for (TActorIterator<ACk_EntitySpawner_UE> It(World); It; ++It)
        {
            ck::entity_spawner_editor_internal::DoApplyIconTo(*It, IconTexture);
        }
    }
}

void FCkEntitySpawnerEditorModule::DoRebuildAllEditorEntities()
{
    if (GEditor == nullptr)
    { return; }

    for (const auto& WorldContext : GEditor->GetWorldContexts())
    {
        auto World = WorldContext.World();
        if (ck::Is_NOT_Valid(World))
        { continue; }

        if (World->WorldType != EWorldType::Editor)
        { continue; }

        for (TActorIterator<ACk_EntitySpawner_UE> It(World); It; ++It)
        {
            It->EditorOnly_RebuildEntity();
        }
    }
}

void FCkEntitySpawnerEditorModule::OnLevelActorAdded(AActor* InActor)
{
    ck::entity_spawner_editor_internal::DoApplyIconTo(
        InActor, ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Get());

    if (auto* Spawner = Cast<ACk_EntitySpawner_UE>(InActor);
        ck::IsValid(Spawner) &&
        ck::IsValid(InActor->GetWorld()) &&
        InActor->GetWorld()->WorldType == EWorldType::Editor)
    {
        Spawner->EditorOnly_RebuildEntity();
    }
}

void FCkEntitySpawnerEditorModule::OnObjectsReplaced(
    const TMap<UObject*, UObject*>& InReplacementMap)
{
    // Rebuild is deferred to end-of-frame so Unreal has fully patched object references before we
    // touch the editor ECS. NO Request_RebuildProcessorGraph here: the C++ processor set does not
    // change on script reload, and tearing down schedulers mid-life crashes in handle destruction.
    if (InReplacementMap.IsEmpty() || GEditor == nullptr)
    { return; }

    const auto HasScriptReplacement = [&]()
    {
        for (const auto& [Old, New] : InReplacementMap)
        {
            if (ck::IsValid(New) && New->IsA<UCk_EntityScript_UE>())
            { return true; }
        }
        return false;
    }();

    if (NOT HasScriptReplacement)
    { return; }

    if (_PendingSpawnerRebuild)
    { return; }

    _PendingSpawnerRebuild = true;
    _EndFrameRebuildHandle = FCoreDelegates::OnEndFrame.AddRaw(
        this, &FCkEntitySpawnerEditorModule::OnEndFrame_RebuildSpawners);
}

void FCkEntitySpawnerEditorModule::OnEndFrame_RebuildSpawners()
{
    FCoreDelegates::OnEndFrame.Remove(_EndFrameRebuildHandle);
    _EndFrameRebuildHandle.Reset();
    _PendingSpawnerRebuild = false;

    if (GEditor == nullptr)
    { return; }

    for (const auto& WorldContext : GEditor->GetWorldContexts())
    {
        auto World = WorldContext.World();
        if (ck::Is_NOT_Valid(World))
        { continue; }

        if (World->WorldType != EWorldType::Editor)
        { continue; }

        for (TActorIterator<ACk_EntitySpawner_UE> It(World); It; ++It)
        {
            It->EditorOnly_RebuildEntity();
        }
    }

    // AngelScript post-compile and BP/AS reinstancing both funnel here — that is exactly when the set of
    // opt-in EntityScript classes (or their display names) can change, so refresh the Place Actors items.
    ck::entity_spawner_editor_internal::DoRefreshPlaceableEntityScripts();
}

UTexture2D* FCkEntitySpawnerEditorModule::Get_EntitySpawnerIconTexture()
{
    return ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Get();
}

void FCkEntitySpawnerEditorModule::ApplyIconToActor(AActor* InActor)
{
    ck::entity_spawner_editor_internal::DoApplyIconTo(
        InActor, ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Get());
}

UTexture2D* FCk_EntitySpawner_IconHelper::Get_Texture()
{
    return ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Get();
}

void FCk_EntitySpawner_IconHelper::ApplyToActor(AActor* InActor)
{
    ck::entity_spawner_editor_internal::DoApplyIconTo(
        InActor, ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Get());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEntitySpawnerEditorModule, CkEntitySpawnerEditor)
