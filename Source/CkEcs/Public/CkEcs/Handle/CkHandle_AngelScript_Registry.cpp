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
    Get_BoundParentConversions()
    -> TSet<FString>&
{
    static TSet<FString> BoundParentConversions;
    return BoundParentConversions;
}

auto
    FCkAngelScript_HandleRegistry::
    Get_WarnedMixinTypes()
    -> TSet<FString>&
{
    static TSet<FString> WarnedMixinTypes;
    return WarnedMixinTypes;
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
    UpdateExistingDynamicHandle(
        const FString& InTypeName,
        TFunction<bool(const FCk_Handle&)> InValidator,
        const TArray<FString>& InRequiredFragments,
        const FString& InDescription,
        const FString& InSourceAsset)
    -> bool
{
    if (InTypeName.IsEmpty())
    { return false; }

    auto* Found = Get_RegisteredTypes().Find(InTypeName);
    if (Found == nullptr || NOT Found->IsValid())
    { return false; }

    auto& Info = **Found;

    // Replace validator + metadata in place. The Cast / CastChecked lambdas
    // capture the new validator so they reflect the same strictness as
    // IsValidAsType. AS-bound methods deref TypeInfo via a stable pointer
    // (see GetTypeInfoFromGeneric + the AuxData refactor in this file's
    // BindCrossHandleConversions), so they pick up the new lambdas on the
    // next call with no re-binding required.
    Info.IsValidAsType     = InValidator;
    Info.RequiredFragments = InRequiredFragments;
    Info.Description       = InDescription;
    Info.SourceAsset       = InSourceAsset;

    Info.Cast = [Validator = InValidator](const FCk_Handle& InHandle) -> FCk_Handle
    {
        if (Validator && Validator(InHandle))
        {
            return InHandle;
        }
        return FCk_Handle{};
    };

    Info.CastChecked = [Validator = InValidator, TypeName = InTypeName](const FCk_Handle& InHandle) -> FCk_Handle
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

    return true;
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
    BindParentChainConversions();
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

    // Same rationale for parent-chain implicit conversions and the shared cycle/missing-parent
    // warning set: a re-walk should re-emit conversions for late-registered children and
    // re-evaluate parent-chain validity from scratch.
    Get_BoundParentConversions().Empty();
    Get_WarnedMixinTypes().Empty();
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

    // Finally bind cross-handle conversions, parent-chain implicit conversions, and mixin methods.
    // Parent-chain conversions before mixin propagation: wire the typesafe-handle lattice first,
    // then propagate methods over it.
    BindCrossHandleConversions();
    BindParentChainConversions();
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

    // Make the AS value-class resolve to a real UScriptStruct when crossed into
    // UE reflection (FInstancedStruct::Make, FAngelscriptAnyStructParameter,
    // FInstancedStruct::Get(?&out), Bind_FString.cpp's struct printer, etc.).
    // FAngelscriptManager::GetUnrealStructFromAngelscriptTypeId returns whatever
    // is stashed in asITypeInfo::plainUserData; without this, that returns null
    // for dynamic handles and the engine fork throws "Not a valid USTRUCT".
    //
    // FCk_Handle::StaticStruct() is the right target because every dynamic
    // handle is binary-identical to FCk_Handle by construction (same size, same
    // layout — the ValueClass above is even sized as sizeof(FCk_Handle)). Boxed
    // payloads round-trip correctly: callers extract an FCk_Handle and re-apply
    // .As_<TypeName>() to recover the typed view. Synthesizing a unique
    // UScriptStruct per dynamic type would add UASStruct / class-generator
    // coupling and per-type GC bookkeeping for no semantic gain — the struct
    // ops would just memcpy sizeof(FCk_Handle) bytes either way.
    Bind.SetTypeUserData(FCk_Handle::StaticStruct());

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

    // AuxData stores a pointer to the target type's TypeInfo (owned by the
    // SharedPtr in Get_RegisteredTypes()) rather than copies of the validator
    // / cast functions. This keeps the AS-bound cross-handle methods consistent
    // with the self-type IsValid binding (which already dereferences userData
    // at call time) and — load-bearing — makes runtime updates to a type's
    // validator visible to all AS-bound methods that reference it. Without
    // this indirection, replacing TypeInfo->IsValidAsType via the Update path
    // would leave the cross-handle Is_X / As_X methods stuck on the original
    // captured copies. The pointer is stable for the lifetime of the editor
    // session because the registry is append-only (no entry is ever removed).
    struct FAsMethodAuxData
    {
        FCkAngelScript_HandleTypeInfo* TargetType = nullptr;
    };

    struct FIsMethodAuxData
    {
        FCkAngelScript_HandleTypeInfo* TargetType = nullptr;
    };

    static TMap<asIScriptFunction*, FAsMethodAuxData> AsMethodAuxDataMap;
    static TMap<asIScriptFunction*, FIsMethodAuxData> IsMethodAuxDataMap;

    for (const auto& SourcePair : Types)
    {
        const auto& SourceType = SourcePair.Value;
        // Pass the FString (owned copy) — a TCHAR_TO_ANSI() temporary would dangle in FBindString and
        // corrupt later RegisterObjectMethod object-type args. See the note in BindBaseMixinMethods.
        auto SourceBind = FAngelscriptBinds::ExistingClass(SourceType->TypeName);
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
            AsAuxData.TargetType = TargetType.Get();

            SourceBind.GenericMethod(AsMethodSig.c_str(),
                [](asIScriptGeneric* InGeneric)
            {
                auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                auto Checked = *static_cast<ECk_SanityCheck*>(InGeneric->GetAddressOfArg(0));
                auto* Function = InGeneric->GetFunction();
                auto* AuxData = AsMethodAuxDataMap.Find(Function);

                if (AuxData == nullptr || AuxData->TargetType == nullptr)
                {
                    auto* ReturnLocation = InGeneric->GetAddressOfReturnLocation();
                    FMemory::Memzero(ReturnLocation, sizeof(FCk_Handle));
                    return;
                }

                // Dereference TargetType at call time so Update_ExistingType
                // mutations to TargetType->Cast / CastChecked are visible.
                auto Result = (Checked == ECk_SanityCheck::UnChecked)
                    ? AuxData->TargetType->Cast(*Handle)
                    : AuxData->TargetType->CastChecked(*Handle);

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
            IsAuxData.TargetType = TargetType.Get();

            SourceBind.GenericMethod(IsMethodSig.c_str(),
                [](asIScriptGeneric* InGeneric)
            {
                auto* Handle = static_cast<const FCk_Handle*>(InGeneric->GetObject());
                auto* Function = InGeneric->GetFunction();
                auto* AuxData = IsMethodAuxDataMap.Find(Function);

                auto Result = false;
                if (AuxData != nullptr
                    && AuxData->TargetType != nullptr
                    && AuxData->TargetType->IsValidAsType)
                {
                    // Dereference TargetType at call time so Update_ExistingType
                    // mutations to TargetType->IsValidAsType are visible.
                    Result = AuxData->TargetType->IsValidAsType(*Handle);
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

    // ----------------------------------------------------------------------------------------
    // Parent-chain construction shared by BindBaseMixinMethods and BindParentChainConversions.
    // Both passes consume the same MixinParentHandle chain (single source of truth) and the
    // same WarnedTypes dedup set so cycle / missing-parent diagnostics are emitted at most
    // once per offending type across both passes.
    // ----------------------------------------------------------------------------------------

    struct FDerivedEntry
    {
        TSharedPtr<FCkAngelScript_HandleTypeInfo> TypeInfo;
        int32 Depth = 0;
        TArray<FString> SourceChain;  // ordered: [FCk_Handle, root ancestor, ..., direct parent]
    };

    auto BuildParentChainEntries(
        const TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>& InDerivedTypes,
        TSet<FString>& InOutWarnedTypes)
        -> TArray<FDerivedEntry>
    {
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
                    if (NOT InOutWarnedTypes.Contains(InTypeInfo->TypeName))
                    {
                        UE_LOG(LogTemp, Warning,
                            TEXT("[HandleRegistry] Mixin parent chain for [%s] cycles at [%s]; treating as base-only."),
                            *InTypeInfo->TypeName, *CurrentParent);
                        InOutWarnedTypes.Add(InTypeInfo->TypeName);
                    }
                    Ancestors.Reset();
                    Entry.Depth = 0;
                    break;
                }
                Visited.Add(CurrentParent);

                const auto* ParentInfo = InDerivedTypes.Find(CurrentParent);
                if (ParentInfo == nullptr)
                {
                    if (NOT InOutWarnedTypes.Contains(InTypeInfo->TypeName))
                    {
                        UE_LOG(LogTemp, Warning,
                            TEXT("[HandleRegistry] Mixin parent [%s] of [%s] is not registered; treating as base-only."),
                            *CurrentParent, *InTypeInfo->TypeName);
                        InOutWarnedTypes.Add(InTypeInfo->TypeName);
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
        Entries.Reserve(InDerivedTypes.Num());
        for (const auto& Pair : InDerivedTypes)
        { Entries.Add(BuildEntry(Pair.Value)); }

        // Depth-sorted: parents before children, so a child's source extraction sees the
        // parent's AS type with its inherited mixins already in place.
        Entries.Sort([](const FDerivedEntry& A, const FDerivedEntry& B) { return A.Depth < B.Depth; });

        return Entries;
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
    auto& WarnedTypes = Get_WarnedMixinTypes();

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

    const auto Entries = BuildParentChainEntries(DerivedTypes, WarnedTypes);

    for (const auto& Entry : Entries)
    {
        const auto& DerivedType = Entry.TypeInfo;
        // Pass the FString so FBindString owns a copy of the name. FBindString stores a
        // `const ANSICHAR*` by raw pointer, so a TCHAR_TO_ANSI() temporary would dangle the moment this
        // statement ends — and this binder is used later in the loop. The freed slot then gets reused by
        // the TCHAR_TO_ANSI() of a method declaration, so the binder's object-type name reads back as the
        // declaration string, corrupting RegisterObjectMethod's object-type argument (asINVALID_TYPE ->
        // the engine's configFailed flag latches -> every subsequent AS registration fails). This only
        // bit packaged builds, where the freed-slot reuse is deterministic.
        auto DerivedBind = FAngelscriptBinds::ExistingClass(DerivedType->TypeName);

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

                // Skip silently when the derived type already declares this method directly (e.g. a typesafe
                // Utils class registered `Request_X(<TypedHandle>)` on FCk_Handle_Typed while the matching
                // TypeUnsafe Utils class registered `Request_X(FCk_Handle)` on FCk_Handle). Without this
                // pre-check, AS would log asALREADY_REGISTERED for every overlap during parent-chain
                // propagation. The dedup map is updated below so subsequent passes also short-circuit.
                if (DerivedTypeInfo->GetMethodByDecl(TCHAR_TO_ANSI(*MethodInfo.Declaration)) != nullptr)
                {
                    BoundMixinMethods.Add(BoundKey);
                    continue;
                }

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
// Parent-Chain Implicit Conversions
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelScript_HandleRegistry::
    BindParentChainConversions()
    -> void
{
    auto* Engine = FAngelscriptManager::Get().GetScriptEngine();
    if (Engine == nullptr)
    { return; }

    const auto& DerivedTypes = Get_RegisteredTypes();
    auto& BoundParentConversions = Get_BoundParentConversions();
    auto& WarnedTypes = Get_WarnedMixinTypes();

    const auto Entries = BuildParentChainEntries(DerivedTypes, WarnedTypes);

    for (const auto& Entry : Entries)
    {
        const auto& DerivedType = Entry.TypeInfo;
        // Pass the FString so FBindString owns a copy of the name. FBindString stores a
        // `const ANSICHAR*` by raw pointer, so a TCHAR_TO_ANSI() temporary would dangle the moment this
        // statement ends — and this binder is used later in the loop. The freed slot then gets reused by
        // the TCHAR_TO_ANSI() of a method declaration, so the binder's object-type name reads back as the
        // declaration string, corrupting RegisterObjectMethod's object-type argument (asINVALID_TYPE ->
        // the engine's configFailed flag latches -> every subsequent AS registration fails). This only
        // bit packaged builds, where the freed-slot reuse is deterministic.
        auto DerivedBind = FAngelscriptBinds::ExistingClass(DerivedType->TypeName);

        if (DerivedBind.GetTypeInfo() == nullptr)
        { continue; }

        // SourceChain[0] is "FCk_Handle" — already wired by CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION
        // (static handles) / CreateDynamicTypeValueClass (dynamic handles). Skip it; only emit
        // implicit conversions for typesafe ancestors past the universal root.
        for (int32 ChainIdx = 1; ChainIdx < Entry.SourceChain.Num(); ++ChainIdx)
        {
            const auto& Ancestor = Entry.SourceChain[ChainIdx];

            if (Ancestor == DerivedType->TypeName)
            { continue; }

            auto BoundKey = ck::Format_UE(TEXT("{}->{}"), DerivedType->TypeName, Ancestor);
            if (BoundParentConversions.Contains(BoundKey))
            { continue; }

            // Layout-compatible reinterpret: all typesafe handles are static_asserted to
            // sizeof(FCk_Handle) with no extra fields. The C++ lambda return type is
            // FCk_Handle&; AS treats the result as the ancestor type via the signature
            // string. Validation is intentionally NOT run at this boundary — see the
            // function-level doc comment in the header.
            const auto SigConvNc = ck::Format_ANSI(TEXT("{}& opImplConv()"), Ancestor);
            const auto SigConvC  = ck::Format_ANSI(TEXT("const {}& opImplConv() const"), Ancestor);

            DerivedBind.Method(SigConvNc.c_str(),
                [](FCk_Handle& Self) -> FCk_Handle& { return Self; });
            DerivedBind.Method(SigConvC.c_str(),
                [](const FCk_Handle& Self) -> const FCk_Handle& { return Self; });

            BoundParentConversions.Add(BoundKey);
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