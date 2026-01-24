#include "CkDynamic_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include <Misc/Paths.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr auto RegistryFileName = TEXT("DynamicHandleTypes.json");
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Dynamic_ProjectSettings_UE::
UCk_Dynamic_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (_DynamicHandleRegistryDirectory.Path.IsEmpty())
    {
        _DynamicHandleRegistryDirectory.Path = FPaths::ProjectConfigDir();
    }
}

auto
    UCk_Dynamic_ProjectSettings_UE::
    Get_DynamicHandleRegistryFilePath() const
    -> FString
{
    if (_DynamicHandleRegistryDirectory.Path.IsEmpty())
    {
        return FPaths::ProjectConfigDir() / RegistryFileName;
    }

    return _DynamicHandleRegistryDirectory.Path / RegistryFileName;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Dynamic_Settings_UE::
    Get_DynamicHandleRegistryFilePath()
    -> FString
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Dynamic_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    {
        return FPaths::ProjectConfigDir() / RegistryFileName;
    }

    return Settings->Get_DynamicHandleRegistryFilePath();
}

// --------------------------------------------------------------------------------------------------------------------