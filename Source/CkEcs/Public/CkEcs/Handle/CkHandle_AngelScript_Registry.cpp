#include "CkHandle_AngelScript_Registry.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Ensure/CkEnsure_Utils.h"
#include "CkCore/Format/CkFormat.h"

#include "Algo/Reverse.h"

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
    Get_BoundMixinMethods()
    -> TSet<FString>&
{
    static TSet<FString> BoundMixinMethods;
    return BoundMixinMethods;
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
        TFunction<void()> InTypeBindingsCallback,
        const FString& InMixinParentTypeName)
    -> bool
{
    auto TypeInfo = FCkAngelScript_HandleTypeInfo{};
    TypeInfo.TypeName = InTypeName;
    TypeInfo.ShortName = InShortName;
    TypeInfo.IsValidAsType = MoveTemp(InHasFunc);
    TypeInfo.Cast = MoveTemp(InCastFunc);
    TypeInfo.CastChecked = MoveTemp(InCastCheckedFunc);
    TypeInfo.TypeBindingsCallback = MoveTemp(InTypeBindingsCallback);
    TypeInfo.MixinParentTypeName = InMixinParentTypeName;
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

    // The mixin-method dedup set persists across calls so that re-binding (RegisterNewTypesIncremental,
    // ForceRefreshDynamicHandleBindings) does not rebind the same {Derived}::{Decl} pair twice.
    // When the caller explicitly resets the bindings-complete flag, they intend to re-walk the
    // entire registered-types set — clearing the dedup set ensures children registered after a
    // parent's first bind pass still inherit that parent's mixin methods on the next walk.
    Get_BoundMixinMethods().Empty();
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

namespace
{
    struct FMixinMethodInfo
    {
        FString Name;
        FString Declaration;
        asIScriptFunction* Function;
        bool ReturnsHandle;
        TOptional<int32> DeterminesOutputTypeArgIndex;
        TOptional<FASBindFunctionPointers> NativeBinding;
    };

    auto ExtractNativeBinding(asIScriptFunction* InFunction) -> TOptional<FASBindFunctionPointers>
    {
        auto* ScriptFunc = static_cast<asCScriptFunction*>(InFunction);
        if (ScriptFunc == nullptr || ScriptFunc->sysFuncIntf == nullptr)
        { return {}; }

        const auto* SysFuncDef = ScriptFunc->sysFuncIntf;

        auto FuncPtr = asSFuncPtr{};
        FuncPtr.ptr.f.func = SysFuncDef->func;

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
    }

    /**
     * Walks the methods on a single AS source type and returns the subset eligible to be
     * propagated as mixin methods onto a derived handle. Source-agnostic: `InSource` may be
     * FCk_Handle (the universal root) or any registered typesafe handle whose AS type already
     * has its own mixins applied.
     *
     * NOTE — return-type non-covariance: methods declared to return e.g. FCk_Handle_Inventory
     * still return FCk_Handle_Inventory when called on a FCk_Handle_Inventory_Spatial from AS.
     * This matches C++ semantics — AS callers must `As_Inventory_Spatial(...)` the result if
     * they need the specialized type. The same note belongs in the AS-side guide so authors
     * aren't surprised. Auto-generated AS-side accessors (op*, As_*, Is_*, IsValid, ToString,
     * Debug, H) are filtered out to avoid copying them across types where they have already
     * been registered per type.
     */
    auto ExtractMixinMethodsFromTypeInfo(
        asITypeInfo* InSource,
        asIScriptEngine* InEngine,
        const TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>& InRegisteredTypes)
        -> TArray<FMixinMethodInfo>
    {
        auto Methods = TArray<FMixinMethodInfo>{};
        if (InSource == nullptr || InEngine == nullptr)
        { return Methods; }

        const auto MethodCount = InSource->GetMethodCount();
        for (asUINT MethodIndex = 0; MethodIndex < MethodCount; ++MethodIndex)
        {
            auto* Method = InSource->GetMethodByIndex(MethodIndex);
            if (Method == nullptr)
            { continue; }

            if (Method->GetFuncType() != asFUNC_SYSTEM)
            { continue; }

            auto MethodName = FString(Method->GetName());

            if (MethodName.StartsWith(TEXT("op")) ||
                MethodName.StartsWith(TEXT("As_")) ||
                MethodName.StartsWith(TEXT("Is_")) ||
                MethodName == TEXT("IsValid") ||
                MethodName == TEXT("ToString") ||
                MethodName == TEXT("Debug") ||
                MethodName == TEXT("H"))
            { continue; }

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
                    auto* RetTypeInfo = InEngine->GetTypeInfoById(RetTypeId);
                    if (RetTypeInfo != nullptr)
                    {
                        const auto RetTypeName = FString(RetTypeInfo->GetName());

                        if (RetTypeName == TEXT("FCk_Handle") || InRegisteredTypes.Contains(RetTypeName))
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
                                auto* ParamTypeInfo = InEngine->GetTypeInfoById(ParamTypeId);
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
                    }
                }
            }

            auto Declaration = FString(Method->GetDeclaration(false, false, true, false));
            auto NativeBinding = ExtractNativeBinding(Method);

            Methods.Add(FMixinMethodInfo{ MethodName, Declaration, Method, ReturnsHandle, DeterminesOutputTypeArgIdx, NativeBinding });
        }

        return Methods;
    }
}

auto
    FCkAngelScript_HandleRegistry::
    BindBaseMixinMethods()
    -> void
{
    auto* Engine = FAngelscriptManager::Get().GetScriptEngine();
    if (Engine == nullptr)
    { return; }

    auto* BaseTypeInfo = Engine->GetTypeInfoByName("FCk_Handle");
    if (BaseTypeInfo == nullptr)
    { return; }

    const auto& DerivedTypes = Get_RegisteredTypes();
    auto& BoundMixinMethods = Get_BoundMixinMethods();

    // Cache for already-extracted source method lists, keyed by source type name. Avoids
    // re-walking the same parent's AS-type for every child that descends from it.
    auto ExtractedMethodsByType = TMap<FString, TArray<FMixinMethodInfo>>{};

    auto GetOrExtractMethods = [&](const FString& InSourceTypeName) -> const TArray<FMixinMethodInfo>&
    {
        if (auto* Existing = ExtractedMethodsByType.Find(InSourceTypeName))
        { return *Existing; }

        auto* SourceTypeInfo = Engine->GetTypeInfoByName(TCHAR_TO_ANSI(*InSourceTypeName));
        auto Methods = ExtractMixinMethodsFromTypeInfo(SourceTypeInfo, Engine, DerivedTypes);
        return ExtractedMethodsByType.Add(InSourceTypeName, MoveTemp(Methods));
    };

    // ---- Compute depth + source chain per registered type, with cycle / missing-parent guards ----

    struct FDerivedEntry
    {
        TSharedPtr<FCkAngelScript_HandleTypeInfo> TypeInfo;
        int32 Depth = 0;
        TArray<FString> SourceChain;  // ordered: [FCk_Handle, immediate parent's parent, ..., direct parent]
    };

    auto WarnedTypes = TSet<FString>{};

    auto BuildEntry = [&](const TSharedPtr<FCkAngelScript_HandleTypeInfo>& InTypeInfo) -> FDerivedEntry
    {
        auto Entry = FDerivedEntry{};
        Entry.TypeInfo = InTypeInfo;
        Entry.SourceChain.Add(TEXT("FCk_Handle"));

        auto Visited = TSet<FString>{};
        auto Ancestors = TArray<FString>{};
        auto CurrentParent = InTypeInfo->MixinParentTypeName;

        while (NOT CurrentParent.IsEmpty())
        {
            if (CurrentParent == TEXT("FCk_Handle") || CurrentParent == TEXT("FCk_Handle_TypeSafe"))
            { break; }

            if (Visited.Contains(CurrentParent))
            {
                if (NOT WarnedTypes.Contains(InTypeInfo->TypeName))
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[HandleRegistry] Mixin parent chain for [%s] cycles at [%s]; treating as base-only."),
                        *InTypeInfo->TypeName, *CurrentParent);
                    WarnedTypes.Add(InTypeInfo->TypeName);
                }
                Ancestors.Reset();
                Entry.Depth = 0;
                break;
            }
            Visited.Add(CurrentParent);

            const auto* ParentInfo = DerivedTypes.Find(CurrentParent);
            if (ParentInfo == nullptr)
            {
                if (NOT WarnedTypes.Contains(InTypeInfo->TypeName))
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[HandleRegistry] Mixin parent [%s] of [%s] is not registered; treating as base-only."),
                        *CurrentParent, *InTypeInfo->TypeName);
                    WarnedTypes.Add(InTypeInfo->TypeName);
                }
                Ancestors.Reset();
                Entry.Depth = 0;
                break;
            }

            Ancestors.Add(CurrentParent);
            CurrentParent = (*ParentInfo)->MixinParentTypeName;
        }

        // Reverse so we process root-most ancestor first, direct parent last
        Algo::Reverse(Ancestors);
        for (const auto& Ancestor : Ancestors)
        { Entry.SourceChain.Add(Ancestor); }

        Entry.Depth = Ancestors.Num();
        return Entry;
    };

    auto Entries = TArray<FDerivedEntry>{};
    Entries.Reserve(DerivedTypes.Num());
    for (const auto& Pair : DerivedTypes)
    { Entries.Add(BuildEntry(Pair.Value)); }

    // ---- Depth-sorted: parents before children, so a child's source extraction sees the
    // parent's AS type with its inherited mixins already in place. ----
    Entries.Sort([](const FDerivedEntry& A, const FDerivedEntry& B) { return A.Depth < B.Depth; });

    for (const auto& Entry : Entries)
    {
        const auto& DerivedType = Entry.TypeInfo;
        auto DerivedBind = FAngelscriptBinds::ExistingClass(TCHAR_TO_ANSI(*DerivedType->TypeName));

        auto* DerivedTypeInfo = DerivedBind.GetTypeInfo();
        if (DerivedTypeInfo == nullptr)
        { continue; }

        // Walk source chain (FCk_Handle, then ancestors root→direct-parent), bind each method onto the
        // derived type. Per-derived dedup ensures parent passes don't double-bind on the same child.
        for (const auto& SourceTypeName : Entry.SourceChain)
        {
            // Don't propagate a type's methods onto itself (would no-op via dedup, but skip the work).
            if (SourceTypeName == DerivedType->TypeName)
            { continue; }

            const auto& Methods = GetOrExtractMethods(SourceTypeName);

            for (const auto& MethodInfo : Methods)
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