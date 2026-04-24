#include "CkEntitySpawnerEditor_Module.h"

#include "CkEntitySpawnerEditor/CkEntitySpawner_ActorFactory.h"
#include "CkEntitySpawnerEditor/CkEntitySpawner_Details.h"
#include "CkEntitySpawnerEditor/CkEntitySpawner_IconHelper.h"
#include "CkEntitySpawnerEditor/CkEntitySpawner_InjectTransform_Details.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"
#include "CkEntitySpawner/CkEntitySpawner_Settings.h"

#include "CkEntitySpawnerEditor_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "UObject/UObjectGlobals.h"

#include "ActorFactories/ActorFactory.h"
#include "Components/BillboardComponent.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

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

    ck::entity_spawner_editor_internal::GEntitySpawnerIcon.Reset();

    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyEditor.UnregisterCustomPropertyTypeLayout(
            FCk_EntitySpawner_ScriptPropertyBinding::StaticStruct()->GetFName());
        PropertyEditor.UnregisterCustomClassLayout(
            ACk_EntitySpawner_UE::StaticClass()->GetFName());
    }

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
    // AS and BP reinstancing fire this. Editor entities hold references to the replaced
    // UCk_EntityScript_UE instances — rebuilding spawners drops those and picks up the new
    // instances the reinstancer wrote into _EntityScript.
    //
    // Rebuild is deferred to end-of-frame so Unreal has fully patched object references
    // before we touch the editor ECS. The C++ processor set does NOT change when script
    // classes reload, so there is no Request_RebuildProcessorGraph here — tearing down
    // schedulers mid-life exposes a latent handle-destructor crash in processor teardown.
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
