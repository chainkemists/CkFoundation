#include "CkEcs_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_EntityScriptSpawnParamsFolderName()
    -> FString
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_EntityScriptSpawnParamsFolderName();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_IgnoredSpawnParamsPropertyNames()
    -> TArray<FString>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_IgnoredSpawnParamsPropertyNames();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Is_IgnoredSpawnParamsProperty(const FString& InPropertyName)
    -> bool
{
    // Always ignore the dummy variable that Blueprint structs require when they have no real members
    if (InPropertyName == TEXT("MemberVar_0"))
    { return true; }

    const auto& IgnoredNames = Get_IgnoredSpawnParamsPropertyNames();

    for (const auto& IgnoredName : IgnoredNames)
    {
        if (InPropertyName.Equals(IgnoredName, ESearchCase::IgnoreCase))
        { return true; }
    }

    return false;
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_HandleDebuggerBehavior()
    -> ECk_Ecs_HandleDebuggerBehavior
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_Ecs_HandleDebuggerBehavior::Disable; }

    return Settings->Get_HandleDebuggerBehavior();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_HandleDebuggerBehavior(
        ECk_Ecs_HandleDebuggerBehavior InHandleDebuggerBehavior)
    -> void
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_HandleDebuggerBehavior(InHandleDebuggerBehavior);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_EntityMapPolicy()
    -> ECk_Ecs_EntityMap_Policy
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_Ecs_EntityMap_Policy::DoNotLog; }

    return Settings->Get_EntityMapPolicy();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_CaptureCallstack_Cpp()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_CaptureCallstack_Cpp();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_CaptureCallstack_Cpp(bool InEnabled)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_CaptureCallstack_Cpp(InEnabled);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_CaptureCallstack_Blueprint()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_CaptureCallstack_Blueprint();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_CaptureCallstack_Blueprint(bool InEnabled)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_CaptureCallstack_Blueprint(InEnabled);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_CaptureCallstack_Angelscript()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_CaptureCallstack_Angelscript();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_CaptureCallstack_Angelscript(bool InEnabled)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_CaptureCallstack_Angelscript(InEnabled);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_MaxCallstackFrames_Cpp()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 8; }

    return Settings->Get_MaxCallstackFrames_Cpp();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_MaxCallstackFrames_Cpp(int32 InMaxFrames)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_MaxCallstackFrames_Cpp(InMaxFrames);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_MaxCallstackFrames_Blueprint_Override()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 0; }

    return Settings->Get_MaxCallstackFrames_Blueprint_Override();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_MaxCallstackFrames_Blueprint_Override(int32 InMaxFrames)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_MaxCallstackFrames_Blueprint_Override(InMaxFrames);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_MaxCallstackFrames_Angelscript_Override()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 0; }

    return Settings->Get_MaxCallstackFrames_Angelscript_Override();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_MaxCallstackFrames_Angelscript_Override(int32 InMaxFrames)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_MaxCallstackFrames_Angelscript_Override(InMaxFrames);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_MaxCallstackEntries()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 8; }

    return Settings->Get_MaxCallstackEntries();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_MaxCallstackEntries(int32 InMaxEntries)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_MaxCallstackEntries(InMaxEntries);
}

// --------------------------------------------------------------------------------------------------------------------