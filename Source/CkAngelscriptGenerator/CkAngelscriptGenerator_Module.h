#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class FCkAngelscriptGeneratorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	FDelegateHandle _PostEngineInitHandle;
	FDelegateHandle _EngineLoopInitCompleteHandle;
	FDelegateHandle _PostAngelscriptCompileHandle;

	// Rev 10 AS bootstrap self-heal — armed only when neither the
	// -NoCkAsRegen CLI flag nor the project-setting opt-out is set.
	FDelegateHandle _ReloadHadErrorsHandle;
	bool            _SelfHealArmed = false;
#endif
};

// --------------------------------------------------------------------------------------------------------------------