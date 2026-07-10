#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Editor twin of CkVat: hosts the in-editor VAT baker (Gate 1 — skeletal mesh + clips -> baked static
// mesh with lookup UVs, VAT textures, serialized clip table on the UCk_VatCollection_Data asset).
class FCkVatEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
