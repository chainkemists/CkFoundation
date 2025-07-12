#include "CkHandle_TypeSafe.h"

#include "AngelscriptCodeModule.h"

#include <AngelscriptManager.h>

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
        ThisType&& InOther) noexcept
    : FCk_Handle(MoveTemp(InOther))
{
}

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
        const FCk_Handle_TypeSafe& InHandle)
    : FCk_Handle(InHandle)
{ }

auto
    FCk_Handle_TypeSafe::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return InOther.ConvertToHandle() == ConvertToHandle();
}

auto
    FCk_Handle_TypeSafe::
    operator=(
        ThisType InOther)
    -> ThisType&
{
    Swap(InOther);
    return *this;
}

auto
    FCk_Handle_TypeSafe::
    ConvertToHandle() const
    -> FCk_Handle
{
    return *this;
}

auto
    FCk_Handle_TypeSafe::
    NetSerialize(
        FArchive& Ar,
        UPackageMap* Map,
        bool& bOutSuccess)
    -> bool
{
    return Super::NetSerialize(Ar, Map, bOutSuccess);
}

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
        const FCk_Handle& InOther)
    : FCk_Handle(InOther)
{ }

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

// --------------------------------------------------------------------------------------------------------------------