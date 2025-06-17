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
    auto OnAssetUpdated(const FAssetData& AssetData) -> void;
    auto OnAssetAdded(const FAssetData& AssetData) -> void;
    auto OnAssetRenamed(const FAssetData& AssetData, const FString&) -> void;
    auto OnAssetLoaded(UObject* InObject) -> void;
    auto OnFilesLoaded() -> void;
    auto OnMapOpened(const FString& Filename, bool bAsTemplate) -> void;
    auto OnNewCurrentLevel() -> void;

private:
    auto TryUpdateVisualizer(const FAssetData& AssetData) -> void;
    auto TryUpdateVisualizer(const UObject* InObject) -> void;
    auto ScanWorldObjectsForVisualizers() -> void;

private:
    TSharedPtr<class FCk_ActorComponent_Visualizer> _Visualizer;
    TSet<FName> _BlueprintsWithCustomVisualizerAdded;

    FDelegateHandle _FiledLoaded_DelegateHandle;
    FDelegateHandle _AssetUpdated_DelegateHandle;
    FDelegateHandle _AssetAdded_DelegateHandle;
    FDelegateHandle _AssetLoaded_DelegateHandle;
    FDelegateHandle _AssetRenamed_DelegateHandle;
    FDelegateHandle _MapOpened_DelegateHandle;
    FDelegateHandle _NewCurrentLevel_DelegateHandle;
};

// ----------------------------------------------------------------------------------------------------------------
