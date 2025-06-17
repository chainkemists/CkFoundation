#include "CkCoreEditor_Module.h"

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"

#include "CkCoreEditor/Component/CkActorComponent_Visualizer.h"

#include <Editor/UnrealEdEngine.h>
#include <UnrealEdGlobals.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/World.h>
#include <Engine/Blueprint.h>
#include <Editor/EditorEngine.h>
#include <EngineUtils.h>
#include <Editor.h>

#define LOCTEXT_NAMESPACE "FCkCoreEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkCoreEditorModule::
    StartupModule()
    -> void
{
    _Visualizer = MakeShareable(new FCk_ActorComponent_Visualizer());
    _Visualizer->OnRegister();

    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        _FiledLoaded_DelegateHandle = AssetRegistry->OnFilesLoaded().AddRaw(this, &FCkCoreEditorModule::OnFilesLoaded);
    }

    _MapOpened_DelegateHandle = FEditorDelegates::OnMapOpened.AddRaw(this, &FCkCoreEditorModule::OnMapOpened);
    _NewCurrentLevel_DelegateHandle = FEditorDelegates::NewCurrentLevel.AddRaw(this, &FCkCoreEditorModule::OnNewCurrentLevel);
}

auto
    FCkCoreEditorModule::
    ShutdownModule()
    -> void
{
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        AssetRegistry->OnFilesLoaded().Remove(_FiledLoaded_DelegateHandle);
        AssetRegistry->OnAssetRenamed().Remove(_AssetRenamed_DelegateHandle);
        AssetRegistry->OnAssetUpdated().Remove(_AssetUpdated_DelegateHandle);
        AssetRegistry->OnAssetAdded().Remove(_AssetAdded_DelegateHandle);
        FCoreUObjectDelegates::OnAssetLoaded.Remove(_AssetLoaded_DelegateHandle);
    }

    FEditorDelegates::OnMapOpened.Remove(_MapOpened_DelegateHandle);
    FEditorDelegates::NewCurrentLevel.Remove(_NewCurrentLevel_DelegateHandle);

    for (const auto& ComponentName : _BlueprintsWithCustomVisualizerAdded)
    {
        GUnrealEd->UnregisterComponentVisualizer(ComponentName);
    }
}

auto
    FCkCoreEditorModule::
    OnAssetUpdated(
        const FAssetData& AssetData)
    -> void
{
    TryUpdateVisualizer(AssetData);
}

auto
    FCkCoreEditorModule::
    OnAssetAdded(
        const FAssetData& AssetData)
    -> void
{
    TryUpdateVisualizer(AssetData);
}

auto
    FCkCoreEditorModule::
    OnAssetRenamed(
        const FAssetData& AssetData,
        const FString&)
    -> void
{
    TryUpdateVisualizer(AssetData);
}

auto
    FCkCoreEditorModule::
    OnAssetLoaded(
        UObject* InObject)
    -> void
{
    TryUpdateVisualizer(InObject);
}

auto
    FCkCoreEditorModule::
    OnFilesLoaded()
    -> void
{
    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        _AssetRenamed_DelegateHandle = AssetRegistry->OnAssetRenamed().AddRaw(this, &FCkCoreEditorModule::OnAssetRenamed);
        _AssetUpdated_DelegateHandle = AssetRegistry->OnAssetUpdated().AddRaw(this, &FCkCoreEditorModule::OnAssetUpdated);
        _AssetAdded_DelegateHandle = AssetRegistry->OnAssetAdded().AddRaw(this, &FCkCoreEditorModule::OnAssetAdded);
        _AssetLoaded_DelegateHandle = FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &FCkCoreEditorModule::OnAssetLoaded);
    }

    ScanWorldObjectsForVisualizers();
}

auto
    FCkCoreEditorModule::
    OnMapOpened(
        const FString& Filename,
        bool bAsTemplate)
    -> void
{
    ScanWorldObjectsForVisualizers();
}

auto
    FCkCoreEditorModule::
    OnNewCurrentLevel()
    -> void
{
    ScanWorldObjectsForVisualizers();
}

auto
    FCkCoreEditorModule::
    TryUpdateVisualizer(
        const FAssetData& AssetData)
    -> void
{
    if (AssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName())
    { return; }

    TryUpdateVisualizer(AssetData.GetAsset());
}

auto
    FCkCoreEditorModule::
    TryUpdateVisualizer(
        const UObject* InObject)
    -> void
{
    #if WITH_EDITOR
    if (IsRunningCookCommandlet())
    { return; }
    #endif

    if (auto* Blueprint = Cast<UBlueprint>(InObject);
        ck::IsValid(Blueprint))
    {
        const auto& GeneratedClass = Blueprint->GeneratedClass;

        if (ck::Is_NOT_Valid(GeneratedClass))
        { return; }

        const auto& Name = GeneratedClass->GetFName();

        const auto& MaybeExistingCompVisualizer = GUnrealEd->FindComponentVisualizer(Name);
        const auto HasCustomVisualizer = ck::IsValid(MaybeExistingCompVisualizer);
        const auto ImplementsCustomVisualizerInterface = UCk_Utils_EditorOnly_UE::Get_DoesBlueprintImplementInterface(Blueprint, UCk_CustomActorComponentVisualizer_Interface::StaticClass());

        if (ImplementsCustomVisualizerInterface && NOT HasCustomVisualizer)
        {
            GUnrealEd->RegisterComponentVisualizer(Name, _Visualizer);
            _BlueprintsWithCustomVisualizerAdded.Add(Name);
        }
        else if (NOT ImplementsCustomVisualizerInterface && HasCustomVisualizer && _BlueprintsWithCustomVisualizerAdded.Contains(Name))
        {
            GUnrealEd->UnregisterComponentVisualizer(Name);
            _BlueprintsWithCustomVisualizerAdded.Remove(Name);
        }
    }
}

auto
    FCkCoreEditorModule::
    ScanWorldObjectsForVisualizers()
    -> void
{
    if (ck::Is_NOT_Valid(GEditor))
    { return; }

    const auto& EditorWorld = GEditor->GetEditorWorldContext().World();

    if (ck::Is_NOT_Valid(EditorWorld))
    { return; }

    for (auto ActorItr = TActorIterator<AActor>(EditorWorld); ActorItr; ++ActorItr)
    {
        const auto* Actor = *ActorItr;

        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        for (const auto* Component : Actor->GetComponents())
        {
            if (ck::Is_NOT_Valid(Component))
            { continue; }

            if (const auto* ComponentClass = Component->GetClass();
                ck::IsValid(ComponentClass))
            {
                TryUpdateVisualizer(UBlueprint::GetBlueprintFromClass(ComponentClass));
            }
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkCoreEditorModule, CkCoreEditor)