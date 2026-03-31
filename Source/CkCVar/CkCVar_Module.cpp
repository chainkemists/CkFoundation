#include "CkCVar_Module.h"

#include "CkCVar/Settings/CkCVar_Settings.h"
#include "CkCVar_Log.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkCVarModule"

void FCkCVarModule::StartupModule()
{
	const auto* Settings = GetDefault<UCk_CVar_Settings_UE>();

	for (const auto& Definition : Settings->Get_RegisteredCVars())
	{
		const auto& Name = Definition.Get_Name();
		const auto& DefaultValue = Definition.Get_DefaultValue();
		const auto& HelpText = Definition.Get_HelpText();

		if (IConsoleManager::Get().FindConsoleObject(*Name.ToString()) != nullptr)
		{
			continue;
		}

		switch (Definition.Get_Type())
		{
			case ECk_CVarType::Int32:
			{
				IConsoleManager::Get().RegisterConsoleVariable(
					*Name.ToString(),
					FCString::Atoi(*DefaultValue),
					*HelpText);
				break;
			}
			case ECk_CVarType::Float:
			{
				IConsoleManager::Get().RegisterConsoleVariable(
					*Name.ToString(),
					FCString::Atof(*DefaultValue),
					*HelpText);
				break;
			}
			case ECk_CVarType::Bool:
			{
				const auto BoolValue = DefaultValue.ToBool();
				IConsoleManager::Get().RegisterConsoleVariable(
					*Name.ToString(),
					BoolValue ? 1 : 0,
					*HelpText);
				break;
			}
			case ECk_CVarType::String:
			{
				IConsoleManager::Get().RegisterConsoleVariable(
					*Name.ToString(),
					*DefaultValue,
					*HelpText);
				break;
			}
			case ECk_CVarType::Command:
			{
				IConsoleManager::Get().RegisterConsoleCommand(
					*Name.ToString(),
					*HelpText,
					FConsoleCommandDelegate(),
					ECVF_Default);
				break;
			}
		}

		ck::cvar::Verbose(TEXT("Registered persisted CVar [%s]"), *Name.ToString());
	}
}

void FCkCVarModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkCVarModule, CkCVar)

// --------------------------------------------------------------------------------------------------------------------
