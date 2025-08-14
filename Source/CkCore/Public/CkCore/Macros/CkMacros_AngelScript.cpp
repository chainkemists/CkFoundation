#include "CkMacros_AngelScript.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

#include "AngelscriptCodeModule.h"

auto
    FCkAngelScriptCtorFunctionRegistration::
    EnsureCallbackRegistered()
    -> void
{
    static auto CallbackRegistered = false;
    if (CallbackRegistered)
    { return; }

    _PreCompileDelegateHandle = FAngelscriptCodeModule::GetPreCompile().AddStatic([]
    {
        RegisterAllCtorFunctions();
        Get_AllCtorFunctions().Reset();
    });
    CallbackRegistered = true;
}

auto
    FCkAngelScriptCtorFunctionRegistration::
    RegisterAllCtorFunctions()
    -> void
{
    for (const auto& RegFunc : Get_AllCtorFunctions())
    {
        RegFunc();
    }
}

auto
    FCkAngelScriptCtorFunctionRegistration::
    Get_AllCtorFunctions()
    -> TArray<FCtorFunction>&
{
    static TArray<FCtorFunction> CtorFunctions;
    return CtorFunctions;
}

auto
    FCkAngelScriptCtorFunctionRegistration::
    RegisterCtorFunction(
        const FCtorFunction& InCtorFunction)
    -> void
{
    Get_AllCtorFunctions().Add(InCtorFunction);

    // If AngelScript is already initialized, register immediately
    if (FAngelscriptManager::IsInitialized())
    {
        InCtorFunction();
    }
    else
    {
        // Otherwise, ensure we have a callback set up
        EnsureCallbackRegistered();
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
