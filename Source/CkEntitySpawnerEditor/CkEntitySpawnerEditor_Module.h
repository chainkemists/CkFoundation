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

private:
    FDelegateHandle _PostEngineInitHandle;
    FDelegateHandle _LevelActorAddedHandle;
    FDelegateHandle _MapOpenedHandle;
    FDelegateHandle _ObjectsReplacedHandle;
};
