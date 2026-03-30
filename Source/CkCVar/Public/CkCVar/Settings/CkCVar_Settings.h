#pragma once

#include "CkCVar/CkCVar_Data.h"

#include <Engine/DeveloperSettings.h>

#include "CkCVar_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Config = CkCVar, DefaultConfig)
class CKCVAR_API UCk_CVar_Settings_UE : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UCk_CVar_Settings_UE();

public:
    static auto
    Get() -> UCk_CVar_Settings_UE*;

public:
    auto
    RegisterDefinition(
        const FCk_CVarDefinition& InDefinition) -> void;

    auto
    UnregisterDefinition(
        FName InName) -> void;

    auto
    GetType(
        FName InName) const -> TOptional<ECk_CVarType>;

    auto
    GetAllRegisteredNames() const -> TArray<FName>;

private:
    UPROPERTY(Config, EditAnywhere,
              Category = "CVar Registry")
    TArray<FCk_CVarDefinition> _RegisteredCVars;

public:
    auto Get_RegisteredCVars() const -> const TArray<FCk_CVarDefinition>&;
};

// --------------------------------------------------------------------------------------------------------------------
