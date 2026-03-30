#include "CkCVar_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_CVar_Settings_UE::
    UCk_CVar_Settings_UE()
{
    CategoryName = FName{TEXT("CkFoundation")};
    SectionName = FName{TEXT("CVar Registry")};
}

auto
    UCk_CVar_Settings_UE::
    Get()
    -> UCk_CVar_Settings_UE*
{
    return GetMutableDefault<UCk_CVar_Settings_UE>();
}

auto
    UCk_CVar_Settings_UE::
    RegisterDefinition(
        const FCk_CVarDefinition& InDefinition)
    -> void
{
    const auto& Name = InDefinition.Get_Name();

    for (auto& Existing : _RegisteredCVars)
    {
        if (Existing.Get_Name() == Name)
        {
            Existing = InDefinition;
            TryUpdateDefaultConfigFile();
            return;
        }
    }

    _RegisteredCVars.Add(InDefinition);
    TryUpdateDefaultConfigFile();
}

auto
    UCk_CVar_Settings_UE::
    UnregisterDefinition(
        FName InName)
    -> void
{
    _RegisteredCVars.RemoveAll([InName](const FCk_CVarDefinition& InDef)
    {
        return InDef.Get_Name() == InName;
    });

    TryUpdateDefaultConfigFile();
}

auto
    UCk_CVar_Settings_UE::
    GetType(
        FName InName) const
    -> TOptional<ECk_CVarType>
{
    for (const auto& Definition : _RegisteredCVars)
    {
        if (Definition.Get_Name() == InName)
        {
            return Definition.Get_Type();
        }
    }

    return {};
}

auto
    UCk_CVar_Settings_UE::
    GetAllRegisteredNames() const
    -> TArray<FName>
{
    auto Names = TArray<FName>{};
    Names.Reserve(_RegisteredCVars.Num());

    for (const auto& Definition : _RegisteredCVars)
    {
        Names.Add(Definition.Get_Name());
    }

    return Names;
}

auto
    UCk_CVar_Settings_UE::
    Get_RegisteredCVars() const
    -> const TArray<FCk_CVarDefinition>&
{
    return _RegisteredCVars;
}

// --------------------------------------------------------------------------------------------------------------------
