#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UTexture2D;
class AActor;

class FCkEntitySpawnerEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static UTexture2D* Get_EntitySpawnerIconTexture();
    static void ApplyIconToActor(AActor* InActor);

private:
    void DoPostEngineInit();
    void DoApplyIconToAllInstances();
    void DoRebuildAllEditorEntities();
    void OnLevelActorAdded(AActor* InActor);
    void OnObjectsReplaced(const TMap<UObject*, UObject*>& InReplacementMap);
    void OnEndFrame_RebuildSpawners();

private:
    FDelegateHandle _PostEngineInitHandle;
    FDelegateHandle _LevelActorAddedHandle;
    FDelegateHandle _MapOpenedHandle;
    FDelegateHandle _ObjectsReplacedHandle;
    FDelegateHandle _EndFrameRebuildHandle;
#if WITH_ANGELSCRIPT_CK
    // AS body-only changes (e.g. DoConstruct edits) don't go through OnObjectsReplaced because
    // they're hot-patched into the same UClass*. Investigation B (plan:
    // you-are-the-technical-imperative-teacup.md) confirmed empirically that
    // FAngelscriptCodeModule::GetPostCompile is the only hook that fires for those. Coalesces
    // into the same end-of-frame rebuild path as OnObjectsReplaced.
    FDelegateHandle _PostAngelscriptCompileHandle;
#endif
    bool _PendingSpawnerRebuild = false;
};
