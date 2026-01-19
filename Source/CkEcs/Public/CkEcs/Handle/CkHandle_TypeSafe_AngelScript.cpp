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

        // Propagate base FCk_Handle mixin methods to all derived handle types
        FCkAngelScriptHandleTypeRegistry::BindBaseMixinMethods();

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

// --------------------------------------------------------------------------------------------------------------------
// Bind FCk_Handle mixin methods to all derived handle types
// This enumerates all methods bound to FCk_Handle and re-binds them to derived types
// using generic method wrappers that forward calls through the base type

auto
    FCkAngelScriptHandleTypeRegistry::
    BindBaseMixinMethods()
    -> void
{
    auto* Engine = FAngelscriptManager::Get().GetScriptEngine();
    if (Engine == nullptr)
    {
        return;
    }

    auto* BaseTypeInfo = Engine->GetTypeInfoByName("FCk_Handle");
    if (BaseTypeInfo == nullptr)
    {
        return;
    }

    const auto& DerivedTypes = GetRegisteredHandleTypes();

    // Collect method info from FCk_Handle for propagation
    struct FMethodInfo
    {
        FString Name;
        FString Declaration;
        asIScriptFunction* Function;
    };
    TArray<FMethodInfo> MethodsToBind;

    auto MethodCount = BaseTypeInfo->GetMethodCount();
    for (asUINT MethodIndex = 0; MethodIndex < MethodCount; ++MethodIndex)
    {
        auto* Method = BaseTypeInfo->GetMethodByIndex(MethodIndex);
        if (Method == nullptr)
        {
            continue;
        }

        // Skip methods that aren't system functions (i.e., native bindings)
        if (Method->GetFuncType() != asFUNC_SYSTEM)
        {
            continue;
        }

        // Skip operators and internal methods that are already bound per-type
        auto MethodName = FString(Method->GetName());
        if (MethodName.StartsWith(TEXT("op")) ||
            MethodName.StartsWith(TEXT("As_")) ||
            MethodName.StartsWith(TEXT("Is_")) ||
            MethodName == TEXT("IsValid") ||
            MethodName == TEXT("ToString") ||
            MethodName == TEXT("Debug") ||
            MethodName == TEXT("H"))
        {
            continue;
        }

        // Get the full declaration for re-binding (without object name, without namespace, with param names)
        auto Declaration = FString(Method->GetDeclaration(false, false, true, false));

        MethodsToBind.Add(FMethodInfo{ MethodName, Declaration, Method });
    }

    // Bind collected methods to each derived handle type
    for (const auto& DerivedType : DerivedTypes)
    {
        auto DerivedBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*DerivedType.TypeName));
        if (DerivedBind.GetTypeInfo() == nullptr)
        {
            continue;
        }

        for (const auto& MethodInfo : MethodsToBind)
        {
            if (NOT FCkAngelScriptHandleBindingTracker::TryRegisterDerivedHandleMethod(
                DerivedType.TypeName, MethodInfo.Name))
            {
                continue;
            }

            // Store method function pointer for the generic callback
            // We use a static map keyed by derived type + method name
            static TMap<FString, asIScriptFunction*> MethodLookup;
            auto LookupKey = FString::Printf(TEXT("%s::%s"), *DerivedType.TypeName, *MethodInfo.Name);
            MethodLookup.Add(LookupKey, MethodInfo.Function);

            // Create a generic method that forwards the call
            // The derived handle implicitly converts to FCk_Handle& so we can call the base method
            DerivedBind.GenericMethod(TCHAR_TO_ANSI(*MethodInfo.Declaration),
                [](asIScriptGeneric* InGeneric)
                {
                    // Get the original method from auxiliary
                    auto* OriginalMethod = static_cast<asIScriptFunction*>(InGeneric->GetAuxiliary());
                    if (OriginalMethod == nullptr)
                    {
                        return;
                    }

                    // Get a context to call the original method
                    auto* Engine = InGeneric->GetEngine();
                    auto* Context = Engine->RequestContext();
                    if (Context == nullptr)
                    {
                        return;
                    }

                    // Prepare the call
                    Context->Prepare(OriginalMethod);

                    // Set the object (the derived handle, which implicitly converts to FCk_Handle)
                    Context->SetObject(InGeneric->GetObject());

                    // Copy all arguments
                    auto ArgCount = static_cast<asUINT>(InGeneric->GetArgCount());
                    for (asUINT ArgIdx = 0; ArgIdx < ArgCount; ++ArgIdx)
                    {
                        // Get argument info
                        asDWORD Flags = 0;
                        auto TypeId = InGeneric->GetArgTypeId(ArgIdx, &Flags);

                        // Copy the argument based on type
                        if (TypeId == asTYPEID_BOOL || TypeId == asTYPEID_INT8 || TypeId == asTYPEID_UINT8)
                        {
                            Context->SetArgByte(ArgIdx, InGeneric->GetArgByte(ArgIdx));
                        }
                        else if (TypeId == asTYPEID_INT16 || TypeId == asTYPEID_UINT16)
                        {
                            Context->SetArgWord(ArgIdx, InGeneric->GetArgWord(ArgIdx));
                        }
                        else if (TypeId == asTYPEID_INT32 || TypeId == asTYPEID_UINT32 || TypeId == asTYPEID_FLOAT32)
                        {
                            Context->SetArgDWord(ArgIdx, InGeneric->GetArgDWord(ArgIdx));
                        }
                        else if (TypeId == asTYPEID_INT64 || TypeId == asTYPEID_UINT64 || TypeId == asTYPEID_FLOAT64)
                        {
                            Context->SetArgQWord(ArgIdx, InGeneric->GetArgQWord(ArgIdx));
                        }
                        else
                        {
                            // Object or handle - pass by address
                            Context->SetArgAddress(ArgIdx, InGeneric->GetAddressOfArg(ArgIdx));
                        }
                    }

                    // Execute
                    Context->Execute();

                    // Copy return value
                    asDWORD RetFlags = 0;
                    auto RetTypeId = InGeneric->GetReturnTypeId(&RetFlags);
                    if (RetTypeId != asTYPEID_VOID)
                    {
                        if (RetTypeId == asTYPEID_BOOL || RetTypeId == asTYPEID_INT8 || RetTypeId == asTYPEID_UINT8)
                        {
                            InGeneric->SetReturnByte(Context->GetReturnByte());
                        }
                        else if (RetTypeId == asTYPEID_INT16 || RetTypeId == asTYPEID_UINT16)
                        {
                            InGeneric->SetReturnWord(Context->GetReturnWord());
                        }
                        else if (RetTypeId == asTYPEID_INT32 || RetTypeId == asTYPEID_UINT32 || RetTypeId == asTYPEID_FLOAT32)
                        {
                            InGeneric->SetReturnDWord(Context->GetReturnDWord());
                        }
                        else if (RetTypeId == asTYPEID_INT64 || RetTypeId == asTYPEID_UINT64 || RetTypeId == asTYPEID_FLOAT64)
                        {
                            InGeneric->SetReturnQWord(Context->GetReturnQWord());
                        }
                        else
                        {
                            // Object return - copy address
                            InGeneric->SetReturnAddress(Context->GetReturnAddress());
                        }
                    }

                    // Return the context
                    Engine->ReturnContext(Context);
                },
                MethodInfo.Function);
        }
    }
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
