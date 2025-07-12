#pragma once

#include <CoreMinimal.h>
#include <Modules/ModuleManager.h>
#include <AssetRegistry/AssetData.h>

// ----------------------------------------------------------------------------------------------------------------

class FCk_ActorComponent_Visualizer;

class FCkCoreEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

protected:
    auto OnAssetUpdated(const FAssetData& InAssetData) -> void;
    auto OnAssetAdded(const FAssetData& InAssetData) -> void;
    auto OnAssetRenamed(const FAssetData& InAssetData, const FString&) -> void;
    auto OnAssetLoaded(UObject* InObject) -> void;
    auto OnAssetOpened(UObject* Asset, IAssetEditorInstance*) -> void;
    auto OnFilesLoaded() -> void;
    auto OnMapOpened(const FString& InFilename, bool InAsTemplate) -> void;
    auto OnNewCurrentLevel() -> void;

private:
    auto RegisterNativeComponentVisualizers() -> void;
    auto TryUpdateVisualizer(const FAssetData& InAssetData) -> void;
    auto TryUpdateVisualizer(const UObject* InObject) -> void;
    auto TryUpdateVisualizerForClass(const UClass* InComponentClass) -> void;
    auto TryUpdateVisualizerForComponentBlueprint(const UBlueprint* Blueprint) -> void;
    auto ScanWorldObjectsForVisualizers() -> void;
    auto ScanActorBlueprintForComponentVisualizers(const UBlueprint* ActorBlueprint) -> void;
    auto ScanSCSNodeForComponentVisualizers(const USCS_Node* Node) -> void;
    auto ScanActorComponentsForVisualizers(const AActor* Actor) -> void;
    auto ProcessComponentForVisualizers(const UActorComponent* Component) -> void;
    auto ProcessBlueprintForVisualizers(const UBlueprint* Blueprint) -> void;

private:
    static auto RegisterHiddenBlueprintFields() -> void;

private:
    TSharedPtr<class FCk_ActorComponent_Visualizer> _Visualizer;
    TSet<FName> _BlueprintsWithCustomVisualizerAdded;
    TSet<FName> _NativeComponentsWithVisualizerAdded;

    FDelegateHandle _FiledLoaded_DelegateHandle;
    FDelegateHandle _AssetUpdated_DelegateHandle;
    FDelegateHandle _AssetAdded_DelegateHandle;
    FDelegateHandle _AssetLoaded_DelegateHandle;
    FDelegateHandle _AssetOpened_DelegateHandle;
    FDelegateHandle _AssetRenamed_DelegateHandle;
    FDelegateHandle _MapOpened_DelegateHandle;
    FDelegateHandle _NewCurrentLevel_DelegateHandle;
};

// ----------------------------------------------------------------------------------------------------------------