#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"
#include "UObject/WeakObjectPtr.h"

class UCkUsf_LookDefinition;
class UPackage;
class FObjectPostSaveContext;

class FCkUsfEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // Saving a package that contains a LookDefinition queues that look for regeneration on the next
    // tick (deferred so we never save the master package from inside another package's save callback).
    void OnPackageSaved(const FString& InFilename, UPackage* InPackage, FObjectPostSaveContext InContext);
    bool DoDrainPendingRegens(float InDeltaT);

    FDelegateHandle _PackageSavedHandle;
    FTSTicker::FDelegateHandle _PendingTicker;
    TArray<TWeakObjectPtr<UCkUsf_LookDefinition>> _PendingRegens;
};
