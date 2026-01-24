#include "CkDynamic_AngelScriptType.h"

#if WITH_ANGELSCRIPT_CK

#include "CkEcs/Handle/CkHandle_AngelScript_Registry.h"

#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>

// --------------------------------------------------------------------------------------------------------------------
// FCkDynamic_AngelscriptType
// --------------------------------------------------------------------------------------------------------------------

FCkDynamic_AngelscriptType::FCkDynamic_AngelscriptType(
    const FString& InTypeName,
    const FString& InShortName)
    : TypeName(InTypeName)
    , ShortName(InShortName)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    GetAngelscriptTypeName() const
    -> FString
{
    return TypeName;
}

auto
    FCkDynamic_AngelscriptType::
    GetAngelscriptTypeName(
        const FAngelscriptTypeUsage& Usage) const
    -> FString
{
    return TypeName;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    GetClass(
        const FAngelscriptTypeUsage& Usage) const
    -> UClass*
{
    return nullptr;
}

auto
    FCkDynamic_AngelscriptType::
    GetUnrealStruct(
        const FAngelscriptTypeUsage& Usage) const
    -> UStruct*
{
    return FCk_Handle::StaticStruct();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    MatchesProperty(
        const FAngelscriptTypeUsage& Usage,
        const FProperty* Property,
        EPropertyMatchType MatchType) const
    -> bool
{
    const auto* StructProperty = CastField<FStructProperty>(Property);
    if (StructProperty == nullptr)
    {
        return false;
    }

    return StructProperty->Struct == FCk_Handle::StaticStruct();
}

auto
    FCkDynamic_AngelscriptType::
    CanCreateProperty(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    CreateProperty(
        const FAngelscriptTypeUsage& Usage,
        const FPropertyParams& Params) const
    -> FProperty*
{
    auto* NewProperty = new FStructProperty(
        Params.Outer,
        Params.PropertyName,
        RF_Public);

    NewProperty->Struct = FCk_Handle::StaticStruct();
    NewProperty->SetElementSize(FCk_Handle::StaticStruct()->GetStructureSize());

    return NewProperty;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    HasReferences(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return false;
}

auto
    FCkDynamic_AngelscriptType::
    NeverRequiresGC(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    CanCopy(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    NeedCopy(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return false;
}

auto
    FCkDynamic_AngelscriptType::
    CopyValue(
        const FAngelscriptTypeUsage& Usage,
        void* SourcePtr,
        void* DestinationPtr) const
    -> void
{
    *static_cast<FCk_Handle*>(DestinationPtr) = *static_cast<FCk_Handle*>(SourcePtr);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    CanCompare(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    IsValueEqual(
        const FAngelscriptTypeUsage& Usage,
        void* SourcePtr,
        void* DestinationPtr) const
    -> bool
{
    return *static_cast<FCk_Handle*>(SourcePtr) == *static_cast<FCk_Handle*>(DestinationPtr);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    CanConstruct(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    NeedConstruct(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    ConstructValue(
        const FAngelscriptTypeUsage& Usage,
        void* DestinationPtr) const
    -> void
{
    new(DestinationPtr) FCk_Handle();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    CanDestruct(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    NeedDestruct(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    DestructValue(
        const FAngelscriptTypeUsage& Usage,
        void* DestinationPtr) const
    -> void
{
    static_cast<FCk_Handle*>(DestinationPtr)->~FCk_Handle();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    GetValueSize(
        const FAngelscriptTypeUsage& Usage) const
    -> int32
{
    return sizeof(FCk_Handle);
}

auto
    FCkDynamic_AngelscriptType::
    GetValueAlignment(
        const FAngelscriptTypeUsage& Usage) const
    -> int32
{
    return alignof(FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    CanBeArgument(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    SetArgument(
        const FAngelscriptTypeUsage& Usage,
        int32 ArgumentIndex,
        asIScriptContext* Context,
        FFrame& Stack,
        const FArgData& Data) const
    -> void
{
    auto* Handle = static_cast<FCk_Handle*>(Data.StackPtr);
    Context->SetArgObject(ArgumentIndex, Handle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    CanBeReturned(
        const FAngelscriptTypeUsage& Usage) const
    -> bool
{
    return true;
}

auto
    FCkDynamic_AngelscriptType::
    GetReturnValue(
        const FAngelscriptTypeUsage& Usage,
        asIScriptContext* Context,
        void* Destination) const
    -> void
{
    auto* ReturnAddress = Context->GetReturnAddress();
    if (ReturnAddress != nullptr)
    {
        *static_cast<FCk_Handle*>(Destination) = *static_cast<FCk_Handle*>(ReturnAddress);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptType::
    GetDebuggerValue(
        const FAngelscriptTypeUsage& Usage,
        void* Address,
        FDebuggerValue& Value) const
    -> bool
{
    auto* Handle = static_cast<FCk_Handle*>(Address);
    Value.Value = Handle->ToString();
    Value.Type = TypeName;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// FCkDynamic_AngelscriptTypeRegistry
// --------------------------------------------------------------------------------------------------------------------

auto
    FCkDynamic_AngelscriptTypeRegistry::
    Get_RegisteredTypes()
    -> TMap<FString, TSharedRef<FCkDynamic_AngelscriptType>>&
{
    static TMap<FString, TSharedRef<FCkDynamic_AngelscriptType>> RegisteredTypes;
    return RegisteredTypes;
}

auto
    FCkDynamic_AngelscriptTypeRegistry::
    Initialize()
    -> void
{
    if (_Initialized)
    {
        return;
    }

    FCkAngelScript_HandleRegistry::SetDynamicHandleTypeFactory(
        [](const FString& InTypeName, const FString& InShortName)
        {
            RegisterType(InTypeName, InShortName);
        });

    _Initialized = true;
}

auto
    FCkDynamic_AngelscriptTypeRegistry::
    RegisterType(
        const FString& InTypeName,
        const FString& InShortName)
    -> TSharedRef<FCkDynamic_AngelscriptType>
{
    auto& Types = Get_RegisteredTypes();

    if (const auto* Existing = Types.Find(InTypeName))
    {
        return *Existing;
    }

    auto NewType = MakeShared<FCkDynamic_AngelscriptType>(InTypeName, InShortName);

    FAngelscriptType::Register(NewType);

    Types.Add(InTypeName, NewType);

    return NewType;
}

auto
    FCkDynamic_AngelscriptTypeRegistry::
    GetType(
        const FString& InTypeName)
    -> TSharedPtr<FCkDynamic_AngelscriptType>
{
    const auto* Found = Get_RegisteredTypes().Find(InTypeName);
    if (Found == nullptr)
    {
        return nullptr;
    }
    return *Found;
}

auto
    FCkDynamic_AngelscriptTypeRegistry::
    IsTypeRegistered(
        const FString& InTypeName)
    -> bool
{
    return Get_RegisteredTypes().Contains(InTypeName);
}

// --------------------------------------------------------------------------------------------------------------------
// Static Initialization
// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_DynamicAngelscriptType_Init(
    FAngelscriptBinds::EOrder::Early, []
{
    FCkDynamic_AngelscriptTypeRegistry::Initialize();
});

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK