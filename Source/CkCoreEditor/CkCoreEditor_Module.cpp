#include "CkCoreEditor_Module.h"

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCoreEditor/Component/CkActorComponent_Visualizer.h"

#include <Editor/UnrealEdEngine.h>
#include <UnrealEdGlobals.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/World.h>
#include <Engine/Blueprint.h>
#include <Editor/EditorEngine.h>
#include <EngineUtils.h>
#include <Editor.h>
#include <Toolkits/IToolkitHost.h>
#include <Engine/SCS_Node.h>
#include <Engine/SimpleConstructionScript.h>

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

    RegisterHiddenBlueprintFields();
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

    if (ck::IsValid(GEditor))
    {
        if (auto* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
            ck::IsValid(AssetEditorSubsystem))
        {
            AssetEditorSubsystem->OnAssetOpenedInEditor().Remove(_AssetOpened_DelegateHandle);
        }
    }

    for (const auto& ComponentName : _BlueprintsWithCustomVisualizerAdded)
    {
        GUnrealEd->UnregisterComponentVisualizer(ComponentName);
    }

    for (const auto& ComponentName : _NativeComponentsWithVisualizerAdded)
    {
        GUnrealEd->UnregisterComponentVisualizer(ComponentName);
    }
}

auto
    FCkCoreEditorModule::
    OnAssetUpdated(
        const FAssetData& InAssetData)
    -> void
{
    TryUpdateVisualizer(InAssetData);
}

auto
    FCkCoreEditorModule::
    OnAssetAdded(
        const FAssetData& InAssetData)
    -> void
{
    TryUpdateVisualizer(InAssetData);
}

auto
    FCkCoreEditorModule::
    OnAssetRenamed(
        const FAssetData& InAssetData,
        const FString&)
    -> void
{
    TryUpdateVisualizer(InAssetData);
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
    OnAssetOpened(
        UObject* Asset,
        IAssetEditorInstance*)
    -> void
{
    if (auto* Blueprint = Cast<UBlueprint>(Asset);
        ck::IsValid(Blueprint))
    {
        ProcessBlueprintForVisualizers(Blueprint);
    }
}

auto
    FCkCoreEditorModule::
    OnFilesLoaded()
    -> void
{
    RegisterNativeComponentVisualizers();

    if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
        ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
    {
        _AssetRenamed_DelegateHandle = AssetRegistry->OnAssetRenamed().AddRaw(this, &FCkCoreEditorModule::OnAssetRenamed);
        _AssetUpdated_DelegateHandle = AssetRegistry->OnAssetUpdated().AddRaw(this, &FCkCoreEditorModule::OnAssetUpdated);
        _AssetAdded_DelegateHandle = AssetRegistry->OnAssetAdded().AddRaw(this, &FCkCoreEditorModule::OnAssetAdded);
        _AssetLoaded_DelegateHandle = FCoreUObjectDelegates::OnAssetLoaded.AddRaw(this, &FCkCoreEditorModule::OnAssetLoaded);
    }

    if (ck::IsValid(GEditor))
    {
        if (auto* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
            ck::IsValid(AssetEditorSubsystem))
        {
            _AssetOpened_DelegateHandle = AssetEditorSubsystem->OnAssetOpenedInEditor().AddRaw(this, &FCkCoreEditorModule::OnAssetOpened);
        }
    }

    ScanWorldObjectsForVisualizers();
}

auto
    FCkCoreEditorModule::
    OnMapOpened(
        const FString& InFilename,
        bool InAsTemplate)
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
        const FAssetData& InAssetData)
    -> void
{
    if (InAssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName())
    { return; }

    TryUpdateVisualizer(InAssetData.GetAsset());
}

auto
    FCkCoreEditorModule::
    TryUpdateVisualizer(
        const UObject* InObject)
    -> void
{
    if (auto* Blueprint = Cast<UBlueprint>(InObject);
        ck::IsValid(Blueprint))
    {
        ProcessBlueprintForVisualizers(Blueprint);
    }
}

auto
    FCkCoreEditorModule::
    ProcessBlueprintForVisualizers(
        const UBlueprint* Blueprint)
    -> void
{
    if (ck::Is_NOT_Valid(Blueprint))
    { return; }

    #if WITH_EDITOR
    if (IsRunningCookCommandlet())
    { return; }
    #endif

    // First, handle the Blueprint itself if it's a component
    if (ck::IsValid(Blueprint->GeneratedClass) &&
        Blueprint->GeneratedClass->IsChildOf(UActorComponent::StaticClass()))
    {
        TryUpdateVisualizerForComponentBlueprint(Blueprint);
    }

    // If it's an Actor Blueprint, also check its components
    if (ck::IsValid(Blueprint->GeneratedClass) &&
        Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
    {
        ScanActorBlueprintForComponentVisualizers(Blueprint);
    }
}

auto
    FCkCoreEditorModule::
    TryUpdateVisualizerForComponentBlueprint(
        const UBlueprint* Blueprint)
    -> void
{
    if (ck::Is_NOT_Valid(Blueprint) || ck::Is_NOT_Valid(Blueprint->GeneratedClass))
    { return; }

    if (NOT Blueprint->GeneratedClass->IsChildOf(UActorComponent::StaticClass()))
    { return; }

    const auto& Name = Blueprint->GeneratedClass->GetFName();

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

auto
    FCkCoreEditorModule::
    TryUpdateVisualizerForClass(
        const UClass* InComponentClass)
    -> void
{
    #if WITH_EDITOR
    if (IsRunningCookCommandlet())
    { return; }
    #endif

    if (ck::Is_NOT_Valid(InComponentClass))
    { return; }

    // Skip Blueprint-generated classes as they're handled separately
    if (ck::IsValid(InComponentClass->ClassGeneratedBy))
    { return; }

    const auto& Name = InComponentClass->GetFName();
    const auto& MaybeExistingCompVisualizer = GUnrealEd->FindComponentVisualizer(Name);
    const auto HasCustomVisualizer = ck::IsValid(MaybeExistingCompVisualizer);
    const auto ImplementsCustomVisualizerInterface = InComponentClass->ImplementsInterface(UCk_CustomActorComponentVisualizer_Interface::StaticClass());

    if (ImplementsCustomVisualizerInterface && NOT HasCustomVisualizer)
    {
        GUnrealEd->RegisterComponentVisualizer(Name, _Visualizer);
        _NativeComponentsWithVisualizerAdded.Add(Name);
    }
    else if (NOT ImplementsCustomVisualizerInterface && HasCustomVisualizer && _NativeComponentsWithVisualizerAdded.Contains(Name))
    {
        GUnrealEd->UnregisterComponentVisualizer(Name);
        _NativeComponentsWithVisualizerAdded.Remove(Name);
    }
}

auto
    FCkCoreEditorModule::
    ScanActorBlueprintForComponentVisualizers(
        const UBlueprint* ActorBlueprint)
    -> void
{
    if (ck::Is_NOT_Valid(ActorBlueprint) || ck::Is_NOT_Valid(ActorBlueprint->GeneratedClass))
    { return; }

    // Get the CDO (Class Default Object) of the Actor
    if (auto* ActorCDO = Cast<AActor>(ActorBlueprint->GeneratedClass->GetDefaultObject());
        ck::IsValid(ActorCDO))
    {
        // Scan all components in the Actor CDO
        ScanActorComponentsForVisualizers(ActorCDO);
    }

    // Also scan the Blueprint's component templates directly
    // This catches components that might not be in the CDO yet
    if (ck::IsValid(ActorBlueprint->SimpleConstructionScript))
    {
        for (const auto& RootNodes = ActorBlueprint->SimpleConstructionScript->GetRootNodes();
            const auto* Node : RootNodes)
        {
            ScanSCSNodeForComponentVisualizers(Node);
        }
    }
}

auto
    FCkCoreEditorModule::
    ScanSCSNodeForComponentVisualizers(
        const USCS_Node* Node)
    -> void
{
    if (ck::Is_NOT_Valid(Node))
    { return; }

    // Check the component template
    if (const auto* ComponentTemplate = Node->ComponentTemplate.Get();
        ck::IsValid(ComponentTemplate))
    {
        ProcessComponentForVisualizers(ComponentTemplate);
    }

    // Recursively scan child nodes
    for (const auto* ChildNode : Node->GetChildNodes())
    {
        ScanSCSNodeForComponentVisualizers(ChildNode);
    }
}

auto
    FCkCoreEditorModule::
    ScanActorComponentsForVisualizers(
        const AActor* Actor)
    -> void
{
    if (ck::Is_NOT_Valid(Actor))
    { return; }

    for (const auto* Component : Actor->GetComponents())
    {
        ProcessComponentForVisualizers(Component);
    }
}

auto
    FCkCoreEditorModule::
    ProcessComponentForVisualizers(
        const UActorComponent* Component)
    -> void
{
    if (ck::Is_NOT_Valid(Component))
    { return; }

    if (const auto* ComponentClass = Component->GetClass();
        ck::IsValid(ComponentClass))
    {
        // Handle Blueprint components
        if (auto* ComponentBlueprint = UBlueprint::GetBlueprintFromClass(ComponentClass);
            ck::IsValid(ComponentBlueprint))
        {
            TryUpdateVisualizerForComponentBlueprint(ComponentBlueprint);
        }
        // Handle native C++ components
        else if (ck::Is_NOT_Valid(ComponentClass->ClassGeneratedBy))
        {
            TryUpdateVisualizerForClass(ComponentClass);
        }
    }
}

auto
    FCkCoreEditorModule::
    RegisterNativeComponentVisualizers()
    -> void
{
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        UClass* Class = *ClassIterator;

        if (ck::Is_NOT_Valid(Class))
        { continue; }

        // Only process native classes (not Blueprint-generated)
        if (ck::IsValid(Class->ClassGeneratedBy))
        { continue; }

        if (Class->IsChildOf(UActorComponent::StaticClass()) &&
            Class->ImplementsInterface(UCk_CustomActorComponentVisualizer_Interface::StaticClass()))
        {
            TryUpdateVisualizerForClass(Class);
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

    auto ProcessedClasses = TSet<const UClass*>{};

    for (auto ActorItr = TActorIterator<AActor>(EditorWorld); ActorItr; ++ActorItr)
    {
        const auto* Actor = *ActorItr;

        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        // Use the shared component processing logic
        for (const auto* Component : Actor->GetComponents())
        {
            if (ck::Is_NOT_Valid(Component))
            { continue; }

            ProcessComponentForVisualizers(Component);

            // Track processed native classes to avoid duplicates
            if (const auto* ComponentClass = Component->GetClass();
                ck::IsValid(ComponentClass) && ck::Is_NOT_Valid(ComponentClass->ClassGeneratedBy))
            {
                ProcessedClasses.Add(ComponentClass);
            }
        }
    }
}

auto
    FCkCoreEditorModule::
    RegisterHiddenBlueprintFields()
    -> void
{
    if (ck::Is_NOT_Valid(GConfig, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const auto PluginConfigPath = FPaths::Combine(UCk_Utils_IO_UE::Get_PluginsDir(TEXT("CkFoundation")), TEXT("Config/DefaultCkFoundation.ini"));

    auto PluginConfig = FConfigFile{};
    PluginConfig.Read(PluginConfigPath);

    if (PluginConfig.IsEmpty())
    { return; }

    constexpr auto Section = TEXT("BlueprintEditor.Menu");
    constexpr auto GlobalConfig_Key = TEXT("BlueprintHiddenFields");
    constexpr auto PluginConfig_Key = TEXT("+BlueprintHiddenFields");

    auto GlobalHiddenFields = TArray<FString>{};
    GConfig->GetArray(Section, GlobalConfig_Key, GlobalHiddenFields, GEditorIni);

    auto PluginHiddenFields = TArray<FString>{};
    PluginConfig.GetArray(Section, PluginConfig_Key, PluginHiddenFields);

    auto ConfigChanged = false;
    for (const auto& Field : PluginHiddenFields)
    {
        if (NOT Field.IsEmpty() && NOT GlobalHiddenFields.Contains(Field))
        {
            GlobalHiddenFields.Add(Field);
            ConfigChanged = true;
        }
    }

    if (ConfigChanged)
    {
        GConfig->SetArray(Section, GlobalConfig_Key, GlobalHiddenFields, GEditorIni);
        GConfig->Flush(false, GEditorIni);
    }
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkCoreEditorModule, CkCoreEditor)