#include "CkHandle_TypeSafe.h"

#include "AngelscriptCodeModule.h"

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

AS_FORCE_LINK const FAngelscriptBinds::FBind BindEquals_FCk_Handle (FAngelscriptBinds::EOrder::Early, []
{
    const FBindFlags Flags;
    auto Bind = FAngelscriptBinds::ValueClass<FCk_Handle>("FCk_Handle", Flags);
    Bind.Method("bool opEquals(const FCk_Handle& Other) const",
      METHODPR_TRIVIAL(bool, FCk_Handle, operator==, (const FCk_Handle&) const));

    Bind.Method("FString ToString() const", [](FCk_Handle const& Self) -> FString
    {
        return Self.ToString();
    });

    Bind.Method("bool IsValid() const", [](FCk_Handle const& Self) -> bool
    {
        return ck::IsValid(Self);
    });

    Bind.Method("FString Debug() const", [](FCk_Handle const& Self) -> FString
    {
        Self.DoFireEnsure();
        return Self.ToString();
    });
});

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