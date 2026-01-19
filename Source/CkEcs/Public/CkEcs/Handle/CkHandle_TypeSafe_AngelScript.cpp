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

        // After all individual handle registrations, bind cross-handle conversions
        FCkAngelScriptHandleTypeRegistry::BindCrossHandleConversions();

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

// --------------------------------------------------------------------------------------------------------------------
// FCkAngelScriptHandleTypeRegistry implementation

auto
    FCkAngelScriptHandleTypeRegistry::
    GetMutableRegisteredHandleTypes()
    -> TArray<FHandleTypeInfo>&
{
    static TArray<FHandleTypeInfo> RegisteredTypes;
    return RegisteredTypes;
}

auto
    FCkAngelScriptHandleTypeRegistry::
    GetRegisteredHandleTypes()
    -> const TArray<FHandleTypeInfo>&
{
    return GetMutableRegisteredHandleTypes();
}

auto
    FCkAngelScriptHandleTypeRegistry::
    GetBoundPairs()
    -> TSet<TPair<FString, FString>>&
{
    static TSet<TPair<FString, FString>> BoundPairs;
    return BoundPairs;
}

auto
    FCkAngelScriptHandleTypeRegistry::
    RegisterHandleType(
        const FString& InTypeName,
        const FString& InShortName,
        FHasFunction InHasFunc,
        FCastFunction InCastFunc,
        FCastCheckedFunction InCastCheckedFunc)
    -> void
{
    auto& Types = GetMutableRegisteredHandleTypes();

    // Check for duplicate registration
    for (const auto& Existing : Types)
    {
        if (Existing.TypeName == InTypeName)
        {
            return;
        }
    }

    Types.Add(FHandleTypeInfo{
        InTypeName,
        InShortName,
        MoveTemp(InHasFunc),
        MoveTemp(InCastFunc),
        MoveTemp(InCastCheckedFunc)
    });
}

auto
    FCkAngelScriptHandleTypeRegistry::
    FindHandleTypeByShortName(
        const FString& InShortName)
    -> const FHandleTypeInfo*
{
    for (const auto& Info : GetRegisteredHandleTypes())
    {
        if (Info.ShortName == InShortName)
        {
            return &Info;
        }
    }
    return nullptr;
}

auto
    FCkAngelScriptHandleTypeRegistry::
    BindCrossHandleConversions()
    -> void
{
    const auto& Types = GetRegisteredHandleTypes();
    auto& BoundPairs = GetBoundPairs();

    // For each source handle type, bind As_/Is_ methods to all OTHER handle types
    for (const auto& SourceType : Types)
    {
        auto SourceBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*SourceType.TypeName));
        if (SourceBind.GetTypeInfo() == nullptr)
        {
            continue;
        }

        for (const auto& TargetType : Types)
        {
            // Skip self-conversion (handled elsewhere or unnecessary)
            if (SourceType.TypeName == TargetType.TypeName)
            {
                continue;
            }

            // Check if this pair is already bound
            auto PairKey = TPair<FString, FString>{SourceType.TypeName, TargetType.TypeName};
            if (BoundPairs.Contains(PairKey))
            {
                continue;
            }

            // Use the tracker to prevent duplicate method registration
            auto AsMethodKey = FString::Printf(TEXT("As_%s"), *TargetType.ShortName);
            if (FCkAngelScriptHandleBindingTracker::TryRegisterDerivedHandleMethod(SourceType.TypeName, AsMethodKey))
            {
                auto TargetTypeName = TargetType.TypeName;
                auto TargetShortName = TargetType.ShortName;

                auto AsMethodSig = FString::Printf(
                    TEXT("%s As_%s(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),
                    *TargetTypeName,
                    *TargetShortName);

                // Store the short name in a static map so the stateless lambda can look it up
                static TMap<FString, FString> ShortNameLookup;
                auto LookupKey = FString::Printf(TEXT("%s_As_%s"), *SourceType.TypeName, *TargetShortName);
                ShortNameLookup.Add(LookupKey, TargetShortName);

                // Use GenericMethod with userdata to pass the short name
                SourceBind.GenericMethod(TCHAR_TO_ANSI(*AsMethodSig),
                    [](asIScriptGeneric* InGeneric)
                    {
                        auto Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                        auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));
                        auto ShortName = static_cast<FString*>(InGeneric->GetAuxiliary());

                        auto TypeInfo = FCkAngelScriptHandleTypeRegistry::FindHandleTypeByShortName(*ShortName);
                        if (TypeInfo == nullptr)
                        {
                            new (InGeneric->GetAddressOfReturnLocation()) FCk_Handle{};
                            return;
                        }

                        auto Result = (Checked == ECk_SanityCheck::UnChecked)
                            ? TypeInfo->CastFunc(*Handle)
                            : TypeInfo->CastCheckedFunc(*Handle);
                        new (InGeneric->GetAddressOfReturnLocation()) FCk_Handle{Result};
                    },
                    &ShortNameLookup[LookupKey]);
            }

            auto IsMethodKey = FString::Printf(TEXT("Is_%s"), *TargetType.ShortName);
            if (FCkAngelScriptHandleBindingTracker::TryRegisterDerivedHandleMethod(SourceType.TypeName, IsMethodKey))
            {
                auto TargetShortName = TargetType.ShortName;
                auto IsMethodSig = FString::Printf(TEXT("bool Is_%s() const"), *TargetShortName);

                static TMap<FString, FString> IsShortNameLookup;
                auto LookupKey = FString::Printf(TEXT("%s_Is_%s"), *SourceType.TypeName, *TargetShortName);
                IsShortNameLookup.Add(LookupKey, TargetShortName);

                SourceBind.GenericMethod(TCHAR_TO_ANSI(*IsMethodSig),
                    [](asIScriptGeneric* InGeneric)
                    {
                        auto Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                        auto ShortName = static_cast<FString*>(InGeneric->GetAuxiliary());

                        auto TypeInfo = FCkAngelScriptHandleTypeRegistry::FindHandleTypeByShortName(*ShortName);
                        if (TypeInfo == nullptr)
                        {
                            InGeneric->SetReturnByte(0);
                            return;
                        }

                        auto Result = TypeInfo->HasFunc(*Handle);
                        InGeneric->SetReturnByte(Result ? 1 : 0);
                    },
                    &IsShortNameLookup[LookupKey]);
            }

            BoundPairs.Add(PairKey);
        }
    }
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
