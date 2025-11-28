#include "CkHandle_TypeSafe_AngelScript.h"

#if WITH_ANGELSCRIPT_CK

#include "AngelscriptCodeModule.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScriptHandleRegistration::
    EnsureCallbackRegistered()
    -> void
{
    static bool BCallbackRegistered = false;
    if (BCallbackRegistered)
    { return; }

    _PreCompileDelegateHandle = FAngelscriptCodeModule::GetPreCompile().AddStatic([]
    {
        RegisterAllHandleConversions();
        GetRegistrationFunctions().Reset();
    });
    BCallbackRegistered = true;
}

auto
    FCkAngelScriptHandleRegistration::
    RegisterAllHandleConversions()
    -> void
{
    for (const auto& RegFunc : GetRegistrationFunctions())
    {
        RegFunc();
    }
}

auto
    FCkAngelScriptHandleRegistration::
    GetRegistrationFunctions()
    -> TArray<FCkAngelScriptHandleRegistration::FRegistrationFunction>&
{
    static TArray<FRegistrationFunction> RegistrationFunctions;
    return RegistrationFunctions;
}

auto
    FCkAngelScriptHandleRegistration::
    RegisterHandleConversion(
        const FRegistrationFunction& InRegistrationFunc)
    -> void
{
    // If AngelScript is already initialized, register immediately and DON'T add to list
    // (adding to list + immediate call = double registration when callback fires)
    if (FAngelscriptManager::IsInitialized())
    {
        InRegistrationFunc();
        return;
    }

    // Otherwise, add to list for later execution and ensure callback is set up
    GetRegistrationFunctions().Add(InRegistrationFunc);
    EnsureCallbackRegistered();
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------