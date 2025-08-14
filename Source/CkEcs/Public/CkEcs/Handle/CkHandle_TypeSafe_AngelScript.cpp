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
    GetRegistrationFunctions().Add(InRegistrationFunc);

    // If AngelScript is already initialized, register immediately
    if (FAngelscriptManager::IsInitialized())
    {
        InRegistrationFunc();
    }
    else
    {
        // Otherwise, ensure we have a callback set up
        EnsureCallbackRegistered();
    }
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------