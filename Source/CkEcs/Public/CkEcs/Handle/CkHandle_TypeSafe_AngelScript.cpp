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

    // Auxiliary data stored in static maps keyed by function pointer.
    // GetAuxiliary() doesn't work as expected, so we use GetFunction() at runtime to look up data.
    struct FAsMethodAuxData
    {
        FCastFunction CastFunc;
        FCastCheckedFunction CastCheckedFunc;
    };

    struct FIsMethodAuxData
    {
        FHasFunction HasFunc;
    };

    static TMap<asIScriptFunction*, FAsMethodAuxData> AsMethodAuxDataMap;
    static TMap<asIScriptFunction*, FIsMethodAuxData> IsMethodAuxDataMap;

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

            // Bind As_ method
            auto AsMethodKey = FString::Printf(TEXT("As_%s"), *TargetType.ShortName);
            if (FCkAngelScriptHandleBindingTracker::TryRegisterDerivedHandleMethod(SourceType.TypeName, AsMethodKey))
            {
                auto TargetTypeName = TargetType.TypeName;
                auto TargetShortName = TargetType.ShortName;

                auto AsMethodSig = FString::Printf(
                    TEXT("%s As_%s(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),
                    *TargetTypeName,
                    *TargetShortName);

                // Prepare aux data to store after binding
                auto AuxData = FAsMethodAuxData{};
                AuxData.CastFunc = TargetType.CastFunc;
                AuxData.CastCheckedFunc = TargetType.CastCheckedFunc;

                SourceBind.GenericMethod(TCHAR_TO_ANSI(*AsMethodSig),
                    [](asIScriptGeneric* InGeneric)
                    {
                        auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                        auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));

                        // Look up aux data by function pointer
                        auto* Function = InGeneric->GetFunction();
                        auto* AuxData = AsMethodAuxDataMap.Find(Function);

                        if (AuxData == nullptr)
                        {
                            UE_LOG(LogTemp, Error, TEXT("[As_] AuxData lookup failed!"));
                            auto* ReturnLocation = InGeneric->GetAddressOfReturnLocation();
                            FMemory::Memzero(ReturnLocation, sizeof(FCk_Handle));
                            return;
                        }

                        auto Result = (Checked == ECk_SanityCheck::UnChecked)
                            ? AuxData->CastFunc(*Handle)
                            : AuxData->CastCheckedFunc(*Handle);

                        auto* ReturnLocation = InGeneric->GetAddressOfReturnLocation();
                        new (ReturnLocation) FCk_Handle(Result);
                    },
                    nullptr);

                // Get the function that was just registered and store aux data for it
                auto* TypeInfo = SourceBind.GetTypeInfo();
                if (TypeInfo != nullptr)
                {
                    auto* RegisteredFunc = TypeInfo->GetMethodByName(TCHAR_TO_ANSI(*AsMethodKey));
                    if (RegisteredFunc != nullptr)
                    {
                        AsMethodAuxDataMap.Add(RegisteredFunc, AuxData);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("[As_] Failed to find registered method: %s on %s"),
                            *AsMethodKey, *SourceType.TypeName);
                    }
                }
            }

            // Bind Is_ method
            auto IsMethodKey = FString::Printf(TEXT("Is_%s"), *TargetType.ShortName);
            if (FCkAngelScriptHandleBindingTracker::TryRegisterDerivedHandleMethod(SourceType.TypeName, IsMethodKey))
            {
                auto TargetShortName = TargetType.ShortName;
                auto IsMethodSig = FString::Printf(TEXT("bool Is_%s() const"), *TargetShortName);

                // Prepare aux data to store after binding
                auto AuxData = FIsMethodAuxData{};
                AuxData.HasFunc = TargetType.HasFunc;

                SourceBind.GenericMethod(TCHAR_TO_ANSI(*IsMethodSig),
                    [](asIScriptGeneric* InGeneric)
                    {
                        auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());

                        // Look up aux data by function pointer
                        auto* Function = InGeneric->GetFunction();
                        auto* AuxData = IsMethodAuxDataMap.Find(Function);

                        if (AuxData == nullptr)
                        {
                            InGeneric->SetReturnByte(0);
                            return;
                        }

                        auto Result = AuxData->HasFunc(*Handle);
                        InGeneric->SetReturnByte(Result ? 1 : 0);
                    },
                    nullptr);

                // Get the function that was just registered and store aux data for it
                auto* TypeInfo = SourceBind.GetTypeInfo();
                if (TypeInfo != nullptr)
                {
                    auto* RegisteredFunc = TypeInfo->GetMethodByName(TCHAR_TO_ANSI(*IsMethodKey));
                    if (RegisteredFunc != nullptr)
                    {
                        IsMethodAuxDataMap.Add(RegisteredFunc, AuxData);
                    }
                }
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

    // Static map to store original base method keyed by derived method function pointer
    // GetAuxiliary() doesn't work, so we use GetFunction() at runtime to look up data.
    static TMap<asIScriptFunction*, asIScriptFunction*> BaseMixinMethodMap;

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

            // Create a generic method that forwards the call
            DerivedBind.GenericMethod(TCHAR_TO_ANSI(*MethodInfo.Declaration),
                [](asIScriptGeneric* InGeneric)
                {
                    // Look up the original base method by function pointer
                    auto* DerivedFunc = InGeneric->GetFunction();
                    auto* OriginalMethodPtr = BaseMixinMethodMap.Find(DerivedFunc);
                    
                    if (OriginalMethodPtr == nullptr || *OriginalMethodPtr == nullptr)
                    {
                        UE_LOG(LogTemp, Error, TEXT("[BaseMixin] Failed to find original method!"));
                        return;
                    }
                    
                    auto* OriginalMethod = *OriginalMethodPtr;

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

                    // Copy return value - MUST happen before ReturnContext()
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
                            // Object return - must copy to destination before context is returned.
                            auto* SrcAddress = Context->GetReturnAddress();
                            auto* DstAddress = InGeneric->GetAddressOfReturnLocation();
                            if (SrcAddress != nullptr && DstAddress != nullptr)
                            {
                                new (DstAddress) FCk_Handle(*static_cast<const FCk_Handle*>(SrcAddress));
                            }
                        }
                    }

                    // Return the context
                    Engine->ReturnContext(Context);
                },
                nullptr);

            // Get the function that was just registered and store the mapping
            auto* TypeInfo = DerivedBind.GetTypeInfo();
            if (TypeInfo != nullptr)
            {
                auto* RegisteredFunc = TypeInfo->GetMethodByName(TCHAR_TO_ANSI(*MethodInfo.Name));
                if (RegisteredFunc != nullptr)
                {
                    BaseMixinMethodMap.Add(RegisteredFunc, MethodInfo.Function);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[BaseMixin] Failed to find registered method: %s on %s"),
                        *MethodInfo.Name, *DerivedType.TypeName);
                }
            }
        }
    }
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------

