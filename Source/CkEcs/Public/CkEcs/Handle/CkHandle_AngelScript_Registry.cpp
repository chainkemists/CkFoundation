#include "CkHandle_AngelScript_Registry.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Format/CkFormat.h"

#include <AngelscriptManager.h>
#include "AngelscriptCodeModule.h"

#include "AngelscriptInclude.h"
#include "StartAngelscriptHeaders.h"
#include "as_scriptfunction.h"
#include "as_callfunc.h"
#include "EndAngelscriptHeaders.h"

// --------------------------------------------------------------------------------------------------------------------
// Anonymous namespace for internal helpers
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GetTypeInfoFromGeneric(asIScriptGeneric* InGeneric) -> FCkAngelScript_HandleTypeInfo*
    {
        if (InGeneric == nullptr)
        {
            return nullptr;
        }

        auto* Function = static_cast<asCScriptFunction*>(InGeneric->GetFunction());
        if (Function == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<FCkAngelScript_HandleTypeInfo*>(Function->userData);
    }

    void SetPreviousFunctionUserData(void* InUserData)
    {
        const auto FunctionId = FAngelscriptBinds::GetPreviousFunctionId();
        auto* ScriptFunction = static_cast<asCScriptFunction*>(
            FAngelscriptManager::Get().Engine->GetFunctionById(FunctionId));

        if (ScriptFunction != nullptr)
        {
            ScriptFunction->userData = InUserData;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Storage
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    Get_RegisteredTypes()
    -> TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&
{
    static TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>> RegisteredTypes;
    return RegisteredTypes;
}

auto
    FCkAngelScript_HandleRegistry::
    Get_PendingTypes()
    -> TArray<FCkAngelScript_HandleTypeInfo>&
{
    static TArray<FCkAngelScript_HandleTypeInfo> PendingTypes;
    return PendingTypes;
}

auto
    FCkAngelScript_HandleRegistry::
    Get_BoundConversionPairs()
    -> TSet<TPair<FString, FString>>&
{
    static TSet<TPair<FString, FString>> BoundPairs;
    return BoundPairs;
}

auto
    FCkAngelScript_HandleRegistry::
    Get_DeferredCallbacks()
    -> TArray<TFunction<void()>>&
{
    static TArray<TFunction<void()>> DeferredCallbacks;
    return DeferredCallbacks;
}

auto
    FCkAngelScript_HandleRegistry::
    Get_DynamicHandleTypeFactory()
    -> TFunction<void(const FString&, const FString&)>&
{
    static TFunction<void(const FString&, const FString&)> Factory;
    return Factory;
}

// --------------------------------------------------------------------------------------------------------------------
// Registration - Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    RegisterStaticHandle(
        const FString& InTypeName,
        const FString& InShortName,
        TFunction<bool(const FCk_Handle&)> InHasFunc,
        TFunction<FCk_Handle(const FCk_Handle&)> InCastFunc,
        TFunction<FCk_Handle(const FCk_Handle&)> InCastCheckedFunc,
        TFunction<void()> InTypeBindingsCallback)
    -> bool
{
    auto TypeInfo = FCkAngelScript_HandleTypeInfo{};
    TypeInfo.TypeName = InTypeName;
    TypeInfo.ShortName = InShortName;
    TypeInfo.IsValidAsType = MoveTemp(InHasFunc);
    TypeInfo.Cast = MoveTemp(InCastFunc);
    TypeInfo.CastChecked = MoveTemp(InCastCheckedFunc);
    TypeInfo.TypeBindingsCallback = MoveTemp(InTypeBindingsCallback);
    TypeInfo.IsDynamicHandle = false;

    return RegisterHandleType(MoveTemp(TypeInfo));
}

auto
    FCkAngelScript_HandleRegistry::
    RegisterDynamicHandle(
        const FString& InTypeName,
        const FString& InShortName,
        TFunction<bool(const FCk_Handle&)> InValidator,
        const TArray<FString>& InRequiredFragments,
        const FString& InDescription,
        const FString& InSourceAsset)
    -> bool
{
    auto TypeInfo = FCkAngelScript_HandleTypeInfo{};
    TypeInfo.TypeName = InTypeName;
    TypeInfo.ShortName = InShortName;
    TypeInfo.IsValidAsType = InValidator;
    TypeInfo.RequiredFragments = InRequiredFragments;
    TypeInfo.Description = InDescription;
    TypeInfo.SourceAsset = InSourceAsset;
    TypeInfo.IsDynamicHandle = true;

    TypeInfo.Cast = [Validator = InValidator](const FCk_Handle& InHandle) -> FCk_Handle
    {
        if (Validator && Validator(InHandle))
        {
            return InHandle;
        }
        return FCk_Handle{};
    };

    TypeInfo.CastChecked = [Validator = InValidator, TypeName = InTypeName](const FCk_Handle& InHandle) -> FCk_Handle
    {
        if (Validator && Validator(InHandle))
        {
            return InHandle;
        }

        const auto& Message = ck::Format_UE(
            TEXT("Handle [{}] does NOT have required fragments for [{}]. Unable to convert Handle."),
            InHandle,
            TypeName);
        UCk_Utils_Ensure_UE::TriggerEnsure(FText::FromString(Message), nullptr);

        return FCk_Handle{};
    };

    return RegisterHandleType(MoveTemp(TypeInfo));
}

auto
    FCkAngelScript_HandleRegistry::
    RegisterDeferredCallback(
        TFunction<void()> InCallback)
    -> void
{
    if (NOT InCallback)
    {
        return;
    }

    Get_DeferredCallbacks().Add(MoveTemp(InCallback));
    EnsureCallbackRegistered();
}

auto
    FCkAngelScript_HandleRegistry::
    RegisterNewTypesIncremental()
    -> int32
{
    auto NewTypeCount = int32{ 0 };

    // Process any pending types that haven't been registered yet
    for (auto& PendingInfo : Get_PendingTypes())
    {
        if (Get_RegisteredTypes().Contains(PendingInfo.TypeName))
        {
            continue;
        }

        auto SharedInfo = MakeShared<FCkAngelScript_HandleTypeInfo>(MoveTemp(PendingInfo));
        Get_RegisteredTypes().Add(SharedInfo->TypeName, SharedInfo);
        CreateTypeBindings(SharedInfo->TypeName);
        NewTypeCount++;
    }

    Get_PendingTypes().Reset();

    // Bind cross-handle conversions for any new type combinations
    BindCrossHandleConversions();
    BindBaseMixinMethods();

    return NewTypeCount;
}

auto
    FCkAngelScript_HandleRegistry::
    ResetBindingsCompleteFlag()
    -> void
{
    _BindingsComplete = false;
}

auto
    FCkAngelScript_HandleRegistry::
    SetDynamicHandleTypeFactory(
        TFunction<void(const FString&, const FString&)> InFactory)
    -> void
{
    Get_DynamicHandleTypeFactory() = MoveTemp(InFactory);
}

auto
    FCkAngelScript_HandleRegistry::
    ExecuteDeferredCallbacks()
    -> void
{
    for (const auto& Callback : Get_DeferredCallbacks())
    {
        if (Callback)
        {
            Callback();
        }
    }

    Get_DeferredCallbacks().Reset();
}

// --------------------------------------------------------------------------------------------------------------------
// Registration - Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    RegisterHandleType(
        FCkAngelScript_HandleTypeInfo&& InTypeInfo)
    -> bool
{
    if (InTypeInfo.TypeName.IsEmpty())
    {
        return false;
    }

    if (InTypeInfo.ShortName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HandleRegistry] ShortName is required for type: %s"), *InTypeInfo.TypeName);
        return false;
    }

    if (Get_RegisteredTypes().Contains(InTypeInfo.TypeName))
    {
        return false;
    }

    for (const auto& Pending : Get_PendingTypes())
    {
        if (Pending.TypeName == InTypeInfo.TypeName)
        {
            return false;
        }
    }

    if (FAngelscriptManager::IsInitialized())
    {
        auto SharedInfo = MakeShared<FCkAngelScript_HandleTypeInfo>(MoveTemp(InTypeInfo));
        Get_RegisteredTypes().Add(SharedInfo->TypeName, SharedInfo);
        CreateTypeBindings(SharedInfo->TypeName);
        return true;
    }

    Get_PendingTypes().Add(MoveTemp(InTypeInfo));
    EnsureCallbackRegistered();
    return true;
}

auto
    FCkAngelScript_HandleRegistry::
    RegisterAllPendingTypes()
    -> void
{
    for (auto& PendingInfo : Get_PendingTypes())
    {
        if (Get_RegisteredTypes().Contains(PendingInfo.TypeName))
        {
            continue;
        }

        auto SharedInfo = MakeShared<FCkAngelScript_HandleTypeInfo>(MoveTemp(PendingInfo));
        Get_RegisteredTypes().Add(SharedInfo->TypeName, SharedInfo);
        CreateTypeBindings(SharedInfo->TypeName);
    }

    Get_PendingTypes().Reset();
}

// --------------------------------------------------------------------------------------------------------------------
// Queries
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    IsHandleTypeRegistered(
        const FString& InTypeName)
    -> bool
{
    return Get_RegisteredTypes().Contains(InTypeName);
}

auto
    FCkAngelScript_HandleRegistry::
    GetHandleTypeInfo(
        const FString& InTypeName)
    -> const FCkAngelScript_HandleTypeInfo*
{
    const auto* Found = Get_RegisteredTypes().Find(InTypeName);
    if (Found == nullptr)
    {
        return nullptr;
    }
    return Found->Get();
}

auto
    FCkAngelScript_HandleRegistry::
    FindByShortName(
        const FString& InShortName)
    -> const FCkAngelScript_HandleTypeInfo*
{
    for (const auto& Pair : Get_RegisteredTypes())
    {
        if (Pair.Value->ShortName == InShortName)
        {
            return Pair.Value.Get();
        }
    }
    return nullptr;
}

auto
    FCkAngelScript_HandleRegistry::
    GetAllRegisteredTypes()
    -> const TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&
{
    return Get_RegisteredTypes();
}

// --------------------------------------------------------------------------------------------------------------------
// Binding Lifecycle
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    EnsureCallbackRegistered()
    -> void
{
    static auto CallbackRegistered = false;
    if (CallbackRegistered)
    {
        return;
    }

    _PreCompileDelegateHandle = FAngelscriptCodeModule::GetPreCompile().AddStatic([]
    {
        EnsureAllBindingsComplete();
    });

    CallbackRegistered = true;
}

auto
    FCkAngelScript_HandleRegistry::
    EnsureAllBindingsComplete()
    -> void
{
    if (_BindingsComplete)
    {
        return;
    }

    // Execute deferred callbacks first - this registers static handles
    ExecuteDeferredCallbacks();

    // Then process any pending types
    RegisterAllPendingTypes();

    // Finally bind cross-handle conversions and mixin methods
    BindCrossHandleConversions();
    BindBaseMixinMethods();

    _BindingsComplete = true;
}

// --------------------------------------------------------------------------------------------------------------------
// Type Bindings
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    CreateTypeBindings(
        const FString& InTypeName)
    -> void
{
    const auto* TypeInfoPtr = Get_RegisteredTypes().Find(InTypeName);
    if (TypeInfoPtr == nullptr)
    {
        return;
    }

    const auto& TypeInfo = *TypeInfoPtr;

    if (TypeInfo->IsDynamicHandle)
    {
        CreateDynamicTypeValueClass(*TypeInfo);
    }
    else if (TypeInfo->TypeBindingsCallback)
    {
        TypeInfo->TypeBindingsCallback();
    }

    BindBaseHandleMethods(*TypeInfo);
}

auto
    FCkAngelScript_HandleRegistry::
    CreateDynamicTypeValueClass(
        const FCkAngelScript_HandleTypeInfo& InTypeInfo)
    -> void
{
    const auto& TypeName = InTypeInfo.TypeName;
    const auto TypeNameAnsi = StringCast<ANSICHAR>(*TypeName);
    const auto TypeNameStr = TypeNameAnsi.Get();

    auto* UserData = const_cast<FCkAngelScript_HandleTypeInfo*>(&InTypeInfo);

    // Allow external modules to register custom FAngelscriptType
    const auto& TypeFactory = Get_DynamicHandleTypeFactory();
    if (TypeFactory)
    {
        TypeFactory(TypeName, InTypeInfo.ShortName);
    }

    auto Bind = FAngelscriptBinds::ValueClass(TypeNameStr, sizeof(FCk_Handle), FBindFlags());

    // Constructors
    Bind.Constructor("void f()", [](FCk_Handle* Address)
    {
        new(Address) FCk_Handle();
    });

    auto CopyCtorSig = ck::Format_ANSI(TEXT("void f(const {}& in InOther)"), TypeName);
    Bind.Constructor(CopyCtorSig.c_str(), [](FCk_Handle* Address, const FCk_Handle& InOther)
    {
        new(Address) FCk_Handle(InOther);
    });

    Bind.Constructor("void f(const FCk_Handle& in InHandle)",
        [](FCk_Handle* Address, const FCk_Handle& InHandle)
    {
        new(Address) FCk_Handle(InHandle);
    });

    // Destructor
    Bind.Destructor("void f()", [](FCk_Handle* Address)
    {
        Address->~FCk_Handle();
    });

    // Assignment operators
    auto AssignSelfSig = ck::Format_ANSI(TEXT("{}& opAssign(const {}& in InOther)"), TypeName, TypeName);
    Bind.Method(AssignSelfSig.c_str(), [](FCk_Handle& Self, const FCk_Handle& InOther) -> FCk_Handle&
    {
        Self = InOther;
        return Self;
    });

    auto AssignBaseSig = ck::Format_ANSI(TEXT("{}& opAssign(const FCk_Handle& in InHandle)"), TypeName);
    Bind.Method(AssignBaseSig.c_str(), [](FCk_Handle& Self, const FCk_Handle& InHandle) -> FCk_Handle&
    {
        Self = InHandle;
        return Self;
    });

    // Implicit conversions
    Bind.Method("FCk_Handle& opImplConv()", [](FCk_Handle& Self) -> FCk_Handle&
    {
        return Self;
    });

    Bind.Method("const FCk_Handle& opImplConv() const", [](const FCk_Handle& Self) -> const FCk_Handle&
    {
        return Self;
    });

    Bind.Method("FCk_Handle& opImplCast()", [](FCk_Handle& Self) -> FCk_Handle&
    {
        return Self;
    });

    Bind.Method("const FCk_Handle& opImplCast() const", [](const FCk_Handle& Self) -> const FCk_Handle&
    {
        return Self;
    });

    // Handle accessors
    Bind.Method("FCk_Handle& H()", [](FCk_Handle& Self) -> FCk_Handle&
    {
        return Self;
    });

    Bind.Method("const FCk_Handle& H() const", [](const FCk_Handle& Self) -> const FCk_Handle&
    {
        return Self;
    });

    // IsValid with type validation
    Bind.GenericMethod("bool IsValid() const",
        [](asIScriptGeneric* InGeneric)
    {
        auto* Self = static_cast<FCk_Handle*>(InGeneric->GetObject());
        auto* TypeInfo = GetTypeInfoFromGeneric(InGeneric);

        auto Result = false;
        if (TypeInfo != nullptr && TypeInfo->IsValidAsType)
        {
            Result = TypeInfo->IsValidAsType(*Self);
        }
        else
        {
            Result = ck::IsValid(*Self);
        }

        InGeneric->SetReturnByte(Result ? 1 : 0);
    }, nullptr);
    SetPreviousFunctionUserData(UserData);

    // Utility methods
    Bind.Method("FString ToString() const", [](const FCk_Handle& Self) -> FString
    {
        return Self.ToString();
    });

    Bind.Method("FString Debug() const", [](const FCk_Handle& Self) -> FString
    {
        Self.DoFireEnsure();
        return Self.ToString();
    });

    // Equality operators
    auto EqualsSelfSig = ck::Format_ANSI(TEXT("bool opEquals(const {}& in Other) const"), TypeName);
    Bind.Method(EqualsSelfSig.c_str(), [](const FCk_Handle& A, const FCk_Handle& B) -> bool
    {
        return A == B;
    });

    Bind.Method("bool opEquals(const FCk_Handle& in Other) const",
        [](const FCk_Handle& A, const FCk_Handle& B) -> bool
    {
        return A == B;
    });
}

auto
    FCkAngelScript_HandleRegistry::
    BindBaseHandleMethods(
        const FCkAngelScript_HandleTypeInfo& InTypeInfo)
    -> void
{
    const auto& TypeName = InTypeInfo.TypeName;
    const auto& ShortName = InTypeInfo.ShortName;

    auto* UserData = const_cast<FCkAngelScript_HandleTypeInfo*>(&InTypeInfo);

    auto BaseBind = FAngelscriptBinds::ExistingClass("FCk_Handle");
    if (BaseBind.GetTypeInfo() == nullptr)
    {
        return;
    }

    // As_ShortName method
    auto AsMethodSig = ck::Format_ANSI(
        TEXT("{} As_{}(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),
        TypeName,
        ShortName);

    BaseBind.GenericMethod(AsMethodSig.c_str(),
        [](asIScriptGeneric* InGeneric)
    {
        auto* Self = static_cast<const FCk_Handle*>(InGeneric->GetObject());
        auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));
        auto* TypeInfo = GetTypeInfoFromGeneric(InGeneric);

        auto Result = FCk_Handle{};

        if (TypeInfo != nullptr)
        {
            Result = (Checked == ECk_SanityCheck::UnChecked)
                ? TypeInfo->Cast(*Self)
                : TypeInfo->CastChecked(*Self);
        }

        new(InGeneric->GetAddressOfReturnLocation()) FCk_Handle(Result);
    }, nullptr);
    SetPreviousFunctionUserData(UserData);

    // Is_ShortName method
    auto IsMethodSig = ck::Format_ANSI(TEXT("bool Is_{}() const"), ShortName);
    BaseBind.GenericMethod(IsMethodSig.c_str(),
        [](asIScriptGeneric* InGeneric)
    {
        auto* Self = static_cast<const FCk_Handle*>(InGeneric->GetObject());
        auto* TypeInfo = GetTypeInfoFromGeneric(InGeneric);

        auto Result = false;
        if (TypeInfo != nullptr && TypeInfo->IsValidAsType)
        {
            Result = TypeInfo->IsValidAsType(*Self);
        }

        InGeneric->SetReturnByte(Result ? 1 : 0);
    }, nullptr);
    SetPreviousFunctionUserData(UserData);

    // Equality with this type
    auto BaseEqualsSig = ck::Format_ANSI(TEXT("bool opEquals(const {}& in Other) const"), TypeName);
    BaseBind.Method(BaseEqualsSig.c_str(), [](const FCk_Handle& A, const FCk_Handle& B) -> bool
    {
        return A == B;
    });
}

// --------------------------------------------------------------------------------------------------------------------
// Cross-Handle Conversions
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    BindCrossHandleConversions()
    -> void
{
    const auto& Types = Get_RegisteredTypes();
    auto& BoundPairs = Get_BoundConversionPairs();

    struct FAsMethodAuxData
    {
        TFunction<FCk_Handle(const FCk_Handle&)> CastFunc;
        TFunction<FCk_Handle(const FCk_Handle&)> CastCheckedFunc;
    };

    struct FIsMethodAuxData
    {
        TFunction<bool(const FCk_Handle&)> IsValidFunc;
    };

    static TMap<asIScriptFunction*, FAsMethodAuxData> AsMethodAuxDataMap;
    static TMap<asIScriptFunction*, FIsMethodAuxData> IsMethodAuxDataMap;

    for (const auto& SourcePair : Types)
    {
        const auto& SourceType = SourcePair.Value;
        auto SourceBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*SourceType->TypeName));
        if (SourceBind.GetTypeInfo() == nullptr)
        {
            continue;
        }

        for (const auto& TargetPair : Types)
        {
            const auto& TargetType = TargetPair.Value;

            if (SourceType->TypeName == TargetType->TypeName)
            {
                continue;
            }

            auto PairKey = TPair<FString, FString>{ SourceType->TypeName, TargetType->TypeName };
            if (BoundPairs.Contains(PairKey))
            {
                continue;
            }

            // As_ method
            auto AsMethodSig = ck::Format_ANSI(
                TEXT("{} As_{}(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),
                TargetType->TypeName,
                TargetType->ShortName);

            auto AsAuxData = FAsMethodAuxData{};
            AsAuxData.CastFunc = TargetType->Cast;
            AsAuxData.CastCheckedFunc = TargetType->CastChecked;

            SourceBind.GenericMethod(AsMethodSig.c_str(),
                [](asIScriptGeneric* InGeneric)
            {
                auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));
                auto* Function = InGeneric->GetFunction();
                auto* AuxData = AsMethodAuxDataMap.Find(Function);

                if (AuxData == nullptr)
                {
                    auto* ReturnLocation = InGeneric->GetAddressOfReturnLocation();
                    FMemory::Memzero(ReturnLocation, sizeof(FCk_Handle));
                    return;
                }

                auto Result = (Checked == ECk_SanityCheck::UnChecked)
                    ? AuxData->CastFunc(*Handle)
                    : AuxData->CastCheckedFunc(*Handle);

                new(InGeneric->GetAddressOfReturnLocation()) FCk_Handle(Result);
            }, nullptr);

            auto* TypeInfo = SourceBind.GetTypeInfo();
            if (TypeInfo != nullptr)
            {
                auto AsMethodKey = ck::Format_ANSI(TEXT("As_{}"), TargetType->ShortName);
                auto* RegisteredFunc = TypeInfo->GetMethodByName(AsMethodKey.c_str());
                if (RegisteredFunc != nullptr)
                {
                    AsMethodAuxDataMap.Add(RegisteredFunc, AsAuxData);
                }
            }

            // Is_ method
            auto IsMethodSig = ck::Format_ANSI(TEXT("bool Is_{}() const"), TargetType->ShortName);

            auto IsAuxData = FIsMethodAuxData{};
            IsAuxData.IsValidFunc = TargetType->IsValidAsType;

            SourceBind.GenericMethod(IsMethodSig.c_str(),
                [](asIScriptGeneric* InGeneric)
            {
                auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                auto* Function = InGeneric->GetFunction();
                auto* AuxData = IsMethodAuxDataMap.Find(Function);

                auto Result = false;
                if (AuxData != nullptr && AuxData->IsValidFunc)
                {
                    Result = AuxData->IsValidFunc(*Handle);
                }

                InGeneric->SetReturnByte(Result ? 1 : 0);
            }, nullptr);

            if (TypeInfo != nullptr)
            {
                auto IsMethodKey = ck::Format_ANSI(TEXT("Is_{}"), TargetType->ShortName);
                auto* RegisteredFunc = TypeInfo->GetMethodByName(IsMethodKey.c_str());
                if (RegisteredFunc != nullptr)
                {
                    IsMethodAuxDataMap.Add(RegisteredFunc, IsAuxData);
                }
            }

            BoundPairs.Add(PairKey);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Base Mixin Methods
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
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

    // Cache the FCk_Handle type ID for comparison
    const auto HandleTypeId = BaseTypeInfo->GetTypeId();

    const auto& DerivedTypes = Get_RegisteredTypes();

    // ---- Helper: extract native binding from a registered AS system function ----

    auto ExtractNativeBinding = [](asIScriptFunction* InFunction) -> TOptional<FASBindFunctionPointers>
    {
        auto* ScriptFunc = static_cast<asCScriptFunction*>(InFunction);
        if (ScriptFunc == nullptr || ScriptFunc->sysFuncIntf == nullptr)
        { return {}; }

        const auto* SysFuncDef = ScriptFunc->sysFuncIntf;

        auto FuncPtr = asSFuncPtr{};
        FuncPtr.ptr.f.func = SysFuncDef->func;

        // ---- Map internal call convention to asSFuncPtr flag ----

        switch (SysFuncDef->callConv)
        {
            case ICC_CDECL_OBJFIRST:
            case ICC_CDECL_OBJFIRST_RETURNINMEM:
                FuncPtr.flag = 2;
                break;
            case ICC_THISCALL:
            case ICC_THISCALL_RETURNINMEM:
            case ICC_VIRTUAL_THISCALL:
            case ICC_VIRTUAL_THISCALL_RETURNINMEM:
                FuncPtr.flag = 3;
                break;
            default:
                return {};
        }

        return FASBindFunctionPointers{ FuncPtr, SysFuncDef->caller };
    };

    // ----

    struct FMethodInfo
    {
        FString Name;
        FString Declaration;
        asIScriptFunction* Function;
        bool ReturnsHandle;
        TOptional<int32> DeterminesOutputTypeArgIndex;
        TOptional<FASBindFunctionPointers> NativeBinding;
    };
    TArray<FMethodInfo> MethodsToBind;

    const auto MethodCount = BaseTypeInfo->GetMethodCount();
    for (asUINT MethodIndex = 0; MethodIndex < MethodCount; ++MethodIndex)
    {
        auto* Method = BaseTypeInfo->GetMethodByIndex(MethodIndex);
        if (Method == nullptr)
        {
            continue;
        }

        if (Method->GetFuncType() != asFUNC_SYSTEM)
        {
            continue;
        }

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

        const auto RetTypeId = Method->GetReturnTypeId();
        const auto ReturnsVoid = (RetTypeId == asTYPEID_VOID);
        auto ReturnsHandle = false;
        auto DeterminesOutputTypeArgIdx = TOptional<int32>{};

        if (NOT ReturnsVoid)
        {
            const auto IsPrimitive = (RetTypeId == asTYPEID_BOOL ||
                                     RetTypeId == asTYPEID_INT8 ||
                                     RetTypeId == asTYPEID_UINT8 ||
                                     RetTypeId == asTYPEID_INT16 ||
                                     RetTypeId == asTYPEID_UINT16 ||
                                     RetTypeId == asTYPEID_INT32 ||
                                     RetTypeId == asTYPEID_UINT32 ||
                                     RetTypeId == asTYPEID_INT64 ||
                                     RetTypeId == asTYPEID_UINT64 ||
                                     RetTypeId == asTYPEID_FLOAT32 ||
                                     RetTypeId == asTYPEID_FLOAT64);

            if (NOT IsPrimitive)
            {
                auto* RetTypeInfo = Engine->GetTypeInfoById(RetTypeId);
                if (RetTypeInfo != nullptr)
                {
                    const auto RetTypeName = FString(RetTypeInfo->GetName());

                    if (RetTypeName == TEXT("FCk_Handle") || DerivedTypes.Contains(RetTypeName))
                    {
                        ReturnsHandle = true;
                    }
                    else if (RetTypeName == TEXT("FScriptStructWildcard"))
                    {
                        const auto ParamCount = Method->GetParamCount();
                        for (asUINT ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
                        {
                            auto ParamTypeId = 0;
                            Method->GetParam(ParamIdx, &ParamTypeId);
                            auto* ParamTypeInfo = Engine->GetTypeInfoById(ParamTypeId);
                            if (ParamTypeInfo != nullptr)
                            {
                                const auto ParamTypeName = FString(ParamTypeInfo->GetName());
                                if (ParamTypeName == TEXT("UScriptStruct"))
                                {
                                    DeterminesOutputTypeArgIdx = static_cast<int32>(ParamIdx);
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        // No return type filtering needed — native re-registration handles all types
                    }
                }
            }
        }

        auto Declaration = FString(Method->GetDeclaration(false, false, true, false));
        auto NativeBinding = ExtractNativeBinding(Method);

        MethodsToBind.Add(FMethodInfo{ MethodName, Declaration, Method, ReturnsHandle, DeterminesOutputTypeArgIdx, NativeBinding });
    }

    static TSet<FString> BoundMixinMethods;

    for (const auto& DerivedPair : DerivedTypes)
    {
        const auto& DerivedType = DerivedPair.Value;
        auto DerivedBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*DerivedType->TypeName));

        auto* DerivedTypeInfo = DerivedBind.GetTypeInfo();
        if (DerivedTypeInfo == nullptr)
        { continue; }

        for (const auto& MethodInfo : MethodsToBind)
        {
            auto BoundKey = ck::Format_UE(TEXT("{}::{}"), DerivedType->TypeName, MethodInfo.Declaration);
            if (BoundMixinMethods.Contains(BoundKey))
            { continue; }

            if (NOT MethodInfo.NativeBinding.IsSet())
            { continue; }

            const auto MethodCountBefore = DerivedTypeInfo->GetMethodCount();

            DerivedBind.Method(
                TCHAR_TO_ANSI(*MethodInfo.Declaration),
                MethodInfo.NativeBinding.GetValue());

            const auto MethodCountAfter = DerivedTypeInfo->GetMethodCount();
            if (MethodCountAfter <= MethodCountBefore)
            { continue; }

            if (MethodInfo.DeterminesOutputTypeArgIndex.IsSet())
            {
                FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(
                    MethodInfo.DeterminesOutputTypeArgIndex.GetValue());
            }

            BoundMixinMethods.Add(BoundKey);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Static Initialization
// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_HandleRegistry_Init(
    FAngelscriptBinds::EOrder::Late, []
{
    FCkAngelScript_HandleRegistry::EnsureCallbackRegistered();
});

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_FCk_Handle_Base(
    FAngelscriptBinds::EOrder::Late, []
{
    static auto Bound = false;
    if (Bound)
    {
        return;
    }
    Bound = true;

    auto Bind = FAngelscriptBinds::ExistingClass("FCk_Handle");
    if (Bind.GetTypeInfo() == nullptr)
    {
        return;
    }

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

#endif // WITH_ANGELSCRIPT_CK