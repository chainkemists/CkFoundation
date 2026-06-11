#include "CkReflection_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Algo/Reverse.h>
#include <GameplayTagContainer.h>
#include <Interfaces/IPluginManager.h>
#include <Misc/PackageName.h>
#include <UObject/Package.h>

#include <Math/Rotator.h>
#include <Math/Transform.h>
#include <Math/Vector.h>

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    Get_SanitizedUserDefinedPropertyName(
        const FProperty* InProperty)
    -> FString
{
    CK_ENSURE_IF_NOT(ck::IsValid(InProperty), TEXT("Invalid Property"))
    { return {}; }

    const auto& PropertyName = SlugStringForValidName(InProperty->GetName());

    int32 LastUnderscoreIndex = INDEX_NONE;
    int32 SecondLastUnderscoreIndex = INDEX_NONE;

    if (PropertyName.FindLastChar(TEXT('_'), LastUnderscoreIndex))
    {
        if (constexpr auto GuidLength = 32;
            PropertyName.Len() - LastUnderscoreIndex - 1 == GuidLength)
        {
            if (const auto& BeforeLastUnderscore = PropertyName.Left(LastUnderscoreIndex);
                BeforeLastUnderscore.FindLastChar(TEXT('_'), SecondLastUnderscoreIndex))
            {
                return BeforeLastUnderscore.Left(SecondLastUnderscoreIndex);
            }
        }
    }

    return PropertyName;
}

auto
    UCk_Utils_Reflection_UE::
    Get_PropertyBySanitizedName(
        UObject* InObject,
        const FString& InSanitizedPropertyName)
    -> FProperty*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object"))
    { return nullptr; }

    if (InSanitizedPropertyName.IsEmpty())
    { return nullptr; }

    const UClass* ObjectClass = InObject->GetClass();

    // Iterate through all properties in the class hierarchy
    for (TFieldIterator<FProperty> PropertyIt(ObjectClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        FProperty* Property = *PropertyIt;

        if (ck::Is_NOT_Valid(Property, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        // Compare the sanitized version of this property's name with the target
        const FString SanitizedCurrentPropertyName = Get_SanitizedUserDefinedPropertyName(Property);

        if (SanitizedCurrentPropertyName.Equals(InSanitizedPropertyName, ESearchCase::IgnoreCase))
        {
            return Property;
        }
    }

    return nullptr;
}

auto
    UCk_Utils_Reflection_UE::
    Get_UserDefinedPropertyGuid(
        const FProperty* InProperty)
    -> FString
{
    CK_ENSURE_IF_NOT(ck::IsValid(InProperty), TEXT("Invalid Property"))
    { return {}; }

    const auto& PropertyName = SlugStringForValidName(InProperty->GetName());

    int32 LastUnderscoreIndex = INDEX_NONE;
    if (PropertyName.FindLastChar(TEXT('_'), LastUnderscoreIndex))
    {
        return PropertyName.Right(LastUnderscoreIndex);
    }

    return PropertyName;
}

auto
    UCk_Utils_Reflection_UE::
    Get_ArePropertiesCompatible(
        const FProperty* InPropertyA,
        const FProperty* InPropertyB)
    -> bool
{
    if (ck::Is_NOT_Valid(InPropertyA, ck::IsValid_Policy_NullptrOnly{}) ||
        ck::Is_NOT_Valid(InPropertyB, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    if (InPropertyA->GetClass() != InPropertyB->GetClass())
    { return {}; }

    if (const auto& ObjectPropA = CastField<FObjectPropertyBase>(InPropertyA))
    {
        if (const auto& ObjectPropB = CastField<FObjectPropertyBase>(InPropertyB))
        {
            return ObjectPropA->PropertyClass == ObjectPropB->PropertyClass;
        }
    }

    if (const auto& SoftObjectPropA = CastField<FSoftObjectProperty>(InPropertyA))
    {
        if (const auto& SoftObjectPropB = CastField<FSoftObjectProperty>(InPropertyB))
        {
            return SoftObjectPropA->PropertyClass == SoftObjectPropB->PropertyClass;
        }
    }

    if (const auto& WeakObjectPropA = CastField<FWeakObjectProperty>(InPropertyA))
    {
        if (const auto& WeakObjectPropB = CastField<FWeakObjectProperty>(InPropertyB))
        {
            return WeakObjectPropA->PropertyClass == WeakObjectPropB->PropertyClass;
        }
    }

    if (const auto& ClassPropA = CastField<FClassProperty>(InPropertyA))
    {
        if (const auto& ClassPropB = CastField<FClassProperty>(InPropertyB))
        {
            return ClassPropA->MetaClass == ClassPropB->MetaClass;
        }
    }

    if (const auto& SoftClassPropA = CastField<FSoftClassProperty>(InPropertyA))
    {
        if (const auto& SoftClassPropB = CastField<FSoftClassProperty>(InPropertyB))
        {
            return SoftClassPropA->MetaClass == SoftClassPropB->MetaClass;
        }
    }

    if (const auto& InterfacePropA = CastField<FInterfaceProperty>(InPropertyA))
    {
        if (const auto& InterfacePropB = CastField<FInterfaceProperty>(InPropertyB))
        {
            return InterfacePropA->InterfaceClass == InterfacePropB->InterfaceClass;
        }
    }

    if (const auto& StructPropA = CastField<FStructProperty>(InPropertyA))
    {
        if (const auto& StructPropB = CastField<FStructProperty>(InPropertyB))
        {
            return StructPropA->Struct == StructPropB->Struct;
        }
    }

    if (const auto& EnumPropA = CastField<FEnumProperty>(InPropertyA))
    {
        if (const auto& EnumPropB = CastField<FEnumProperty>(InPropertyB))
        {
            return EnumPropA->GetEnum() == EnumPropB->GetEnum();
        }
    }

    if (const auto& BytePropA = CastField<FByteProperty>(InPropertyA))
    {
        if (const auto& BytePropB = CastField<FByteProperty>(InPropertyB))
        {
            return BytePropA->Enum == BytePropB->Enum;
        }
    }

    if (const auto& ArrayPropA = CastField<FArrayProperty>(InPropertyA))
    {
        if (const auto& ArrayPropB = CastField<FArrayProperty>(InPropertyB))
        {
            return Get_ArePropertiesCompatible(ArrayPropA->Inner, ArrayPropB->Inner);
        }
    }

    if (const auto& SetPropA = CastField<FSetProperty>(InPropertyA))
    {
        if (const auto& SetPropB = CastField<FSetProperty>(InPropertyB))
        {
            return Get_ArePropertiesCompatible(SetPropA->ElementProp, SetPropB->ElementProp);
        }
    }

    if (const auto& MapPropA = CastField<FMapProperty>(InPropertyA))
    {
        if (const auto& MapPropB = CastField<FMapProperty>(InPropertyB))
        {
            return Get_ArePropertiesCompatible(MapPropA->KeyProp, MapPropB->KeyProp) &&
                   Get_ArePropertiesCompatible(MapPropA->ValueProp, MapPropB->ValueProp);
        }
    }

    if (const auto& DelegatePropA = CastField<FDelegateProperty>(InPropertyA))
    {
        if (const auto& DelegatePropB = CastField<FDelegateProperty>(InPropertyB))
        {
            return DelegatePropA->SignatureFunction == DelegatePropB->SignatureFunction;
        }
    }

    if (const auto& MulticastPropA = CastField<FMulticastDelegateProperty>(InPropertyA))
    {
        if (const auto& MulticastPropB = CastField<FMulticastDelegateProperty>(InPropertyB))
        {
            return MulticastPropA->SignatureFunction == MulticastPropB->SignatureFunction;
        }
    }

    if (const auto& MulticastSparsePropA = CastField<FMulticastSparseDelegateProperty>(InPropertyA))
    {
        if (const auto& MulticastSparsePropB = CastField<FMulticastSparseDelegateProperty>(InPropertyB))
        {
            return MulticastSparsePropA->SignatureFunction == MulticastSparsePropB->SignatureFunction;
        }
    }

    return true;
}

auto
    UCk_Utils_Reflection_UE::
    Get_ArePropertiesDifferent(
        const TArray<FProperty*>& InPropertiesA,
        const TArray<FProperty*>& InPropertiesB)
    -> bool
{
    if (InPropertiesA.Num() != InPropertiesB.Num())
    { return true; }

    auto PropertiesA_Map = TMap<FName, FProperty*>{};

    for (auto* Property : InPropertiesA)
    {
        if (ck::IsValid(Property, ck::IsValid_Policy_NullptrOnly{}))
        {
            PropertiesA_Map.Add(*Get_SanitizedUserDefinedPropertyName(Property), Property);
        }
    }

    for (const auto* NewProperty : InPropertiesB)
    {
        const auto& FoundExistingProperty = PropertiesA_Map.Find(*Get_SanitizedUserDefinedPropertyName(NewProperty));

        if (ck::Is_NOT_Valid(FoundExistingProperty, ck::IsValid_Policy_NullptrOnly{}))
        { return true; }

        if (const auto ExistingProperty = *FoundExistingProperty;
            NOT Get_ArePropertiesCompatible(ExistingProperty, NewProperty))
        { return true; }
    }

    return false;
}

auto
    UCk_Utils_Reflection_UE::
    Get_ExposedPropertiesOfClass(
        const UClass* InClass)
    -> TArray<FProperty*>
{
    auto ExposedProperties = TArray<FProperty*>{};

#if WITH_EDITOR
    // Walk the class chain base -> derived so the output reads Parent -> Child. A flat
    // Algo::Reverse on an IncludeSuper iteration would also flip the within-class declaration
    // order, which is wrong; iterating per-class with ExcludeSuper preserves it.
    auto ClassChain = TArray<const UClass*>{};
    for (const auto* Current = InClass; Current != nullptr; Current = Current->GetSuperClass())
    { ClassChain.Add(Current); }
    Algo::Reverse(ClassChain);

    for (const auto* CurrentClass : ClassChain)
    {
        for (TFieldIterator<FProperty> PropertyIt(CurrentClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
        {
            auto* Property = *PropertyIt;

            const auto& IsDelegate          = Property->IsA(FMulticastDelegateProperty::StaticClass());
            const auto& IsExposedToSpawn    = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
            const auto& IsSettableExternally = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
            const auto& IsParm              = Property->HasAnyPropertyFlags(CPF_Parm);
            const auto& StillExists         = FBlueprintEditorUtils::PropertyStillExists(Property);
            const auto& IsBpVisible         = Property->HasAllPropertyFlags(CPF_BlueprintVisible);

            const auto WouldBeIncluded = NOT IsParm && StillExists && IsBpVisible && IsSettableExternally && IsExposedToSpawn && NOT IsDelegate;
            if (NOT WouldBeIncluded)
            {
                ck::core::VeryVerbose(
                    TEXT("[ExposedProps] SKIP Class=[{}] Prop=[{}] | IsParm={} StillExists={} BpVisible={} Settable={} ExposedToSpawn={} IsDelegate={}"),
                    CurrentClass->GetName(),
                    Property->GetName(),
                    IsParm, StillExists, IsBpVisible, IsSettableExternally, IsExposedToSpawn, IsDelegate);
                continue;
            }

            ExposedProperties.Add(Property);
        }
    }
#endif

    return ExposedProperties;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    Get_IsDelegateProperty(
        const FProperty* InProperty)
    -> bool
{
    if (ck::Is_NOT_Valid(InProperty, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    // FDelegateProperty covers single-cast delegates.
    // FMulticastDelegateProperty covers all multicast variants
    //   (FMulticastInlineDelegateProperty, FMulticastSparseDelegateProperty).
    return CastField<FDelegateProperty>(InProperty) != nullptr
        || CastField<FMulticastDelegateProperty>(InProperty) != nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    Is_PlaceholderClass(
        const UClass* InClass)
    -> bool
{
    if (ck::Is_NOT_Valid(InClass))
    { return false; }

    const auto& Name = InClass->GetName();
    return Name.StartsWith(TEXT("SKEL_"))
        || Name.StartsWith(TEXT("REINST_"))
        || Name.StartsWith(TEXT("TRASHCLASS_"))
        || Name.StartsWith(TEXT("HOTRELOADED_"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    Is_EditorOnlyClass(
        const UClass* InClass)
    -> bool
{
    if (ck::Is_NOT_Valid(InClass))
    { return false; }

    const auto Package = InClass->GetOutermost();
    if (ck::Is_NOT_Valid(Package))
    { return false; }

    const auto ModuleName = FPackageName::GetShortFName(Package->GetFName());

    // Plugin module host type is authoritative when the module is described
    // by a plugin descriptor.
    for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
    {
        for (const auto& Module : Plugin->GetDescriptor().Modules)
        {
            if (Module.Name != ModuleName)
            { continue; }

            switch (Module.Type)
            {
                case EHostType::Editor:
                case EHostType::EditorNoCommandlet:
                case EHostType::EditorAndProgram:
                case EHostType::UncookedOnly:
                    return true;
                default:
                    return false;
            }
        }
    }

    // Engine editor modules whose names don't carry "Editor".
    static const auto EditorModules = TSet<FName>{
        TEXT("UnrealEd"),
        TEXT("ViewportInteraction"),
        TEXT("VREditor"),
        TEXT("Blutility"),
        TEXT("Kismet"),
        TEXT("AssetTools"),
        TEXT("PlacementMode"),
        TEXT("MaterialEditor"),
        TEXT("LandscapeEditor"),
    };

    if (EditorModules.Contains(ModuleName))
    { return true; }

    // Name heuristic LAST — only consulted for modules no plugin descriptor
    // describes (engine/native modules outside the known set).
    return ModuleName.ToString().Contains(TEXT("Editor"));
}

auto
    UCk_Utils_Reflection_UE::
    Get_EscapedStringForAngelscript(
        const FString& InRaw)
    -> FString
{
    auto Result = InRaw;
    Result.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
    Result.ReplaceInline(TEXT("\""), TEXT("\\\""));
    Result.ReplaceInline(TEXT("\n"), TEXT("\\n"));
    Result.ReplaceInline(TEXT("\r"), TEXT("\\r"));
    Result.ReplaceInline(TEXT("\t"), TEXT("\\t"));
    return Result;
}

auto
    UCk_Utils_Reflection_UE::
    Get_AngelscriptDefaultExpression(
        const FCk_PropertyDefaultValueLiteral& InLiteral)
    -> FString
{
    switch (InLiteral._Kind)
    {
        case ECk_PropertyDefaultValueKind::RawLiteral:
            return InLiteral._Value;
        case ECk_PropertyDefaultValueKind::String:
            return ck::Format_UE(TEXT("\"{}\""), Get_EscapedStringForAngelscript(InLiteral._Value));
        case ECk_PropertyDefaultValueKind::Name:
            return ck::Format_UE(TEXT("n\"{}\""), Get_EscapedStringForAngelscript(InLiteral._Value));
        case ECk_PropertyDefaultValueKind::Text:
            return ck::Format_UE(TEXT("FText::FromString(\"{}\")"), Get_EscapedStringForAngelscript(InLiteral._Value));
    }
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_reflection_detail
{
    auto MakeRaw(FString InValue) -> FCk_PropertyDefaultValueLiteral
    {
        return FCk_PropertyDefaultValueLiteral{ECk_PropertyDefaultValueKind::RawLiteral, MoveTemp(InValue)};
    }

    auto FormatDoubleLiteral(double InValue) -> FString
    {
        auto Str = ck::Format_UE(TEXT("{}"), InValue);
        if (NOT Str.Contains(TEXT(".")))
        { Str += TEXT(".0"); }
        return Str;
    }

    auto Get_StructLiteral(
        const FStructProperty* InStructProp,
        const void* InValuePtr)
        -> TOptional<FCk_PropertyDefaultValueLiteral>
    {
        auto* Struct = InStructProp->Struct.Get();
        if (ck::Is_NOT_Valid(Struct, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        // ---- Named constants and early-exit types --------------------------------
        // Types listed here either have idiomatic named constants or have a constructor
        // argument order that differs from TFieldIterator order — making the general
        // decomposition below unsafe for them. Return {} for non-constant values to
        // signal "no initializer" rather than emit a wrong expression.

        if (Struct == TBaseStructure<FTransform>::Get())
        {
            const auto& Value = *static_cast<const FTransform*>(InValuePtr);
            if (Value.Equals(FTransform::Identity))
            { return MakeRaw(TEXT("FTransform::Identity")); }
            return {}; // Non-identity: constructor arg order != field order
        }

        if (Struct == FGameplayTag::StaticStruct())
        {
            const auto& Value = *static_cast<const FGameplayTag*>(InValuePtr);
            if (NOT Value.IsValid())
            { return MakeRaw(TEXT("FGameplayTag()")); }
            return {}; // Valid tag: can't express as a constructor literal
        }

        // Named constants for types that DO fall through to general decomposition
        // for non-constant values — emitting a readable alias when available.
        if (Struct == TBaseStructure<FVector>::Get())
        {
            const auto& Value = *static_cast<const FVector*>(InValuePtr);
            if (Value.IsNearlyZero())
            { return MakeRaw(TEXT("FVector::ZeroVector")); }
            if (Value.Equals(FVector::OneVector))
            { return MakeRaw(TEXT("FVector::OneVector")); }
            // Non-constant value: fall through to general decomposition
        }
        else if (Struct == TBaseStructure<FRotator>::Get())
        {
            const auto& Value = *static_cast<const FRotator*>(InValuePtr);
            if (Value.IsNearlyZero())
            { return MakeRaw(TEXT("FRotator::ZeroRotator")); }
            // Non-constant value: fall through to general decomposition
        }

        // ---- General field-by-field decomposition --------------------------------
        // For any struct whose every non-parm UPROPERTY field produces a representable
        // literal, emit StructName(expr1, expr2, ...).  If every field equals its
        // InitializeStruct default, emit StructName() instead.
        // Returns {} if any field type is not representable — no partial expressions.

        const auto StructName = ck::Format_UE(TEXT("{}{}"), Struct->GetPrefixCPP(), Struct->GetName());

        auto DefaultBuffer = TArray<uint8>{};
        DefaultBuffer.SetNumZeroed(Struct->GetStructureSize());
        Struct->InitializeStruct(DefaultBuffer.GetData());
        ON_SCOPE_EXIT { Struct->DestroyStruct(DefaultBuffer.GetData()); };

        auto FieldExpressions = TArray<FString>{};
        auto AllAtDefault = true;

        for (TFieldIterator<FProperty> FieldIt(Struct, EFieldIteratorFlags::IncludeSuper); FieldIt; ++FieldIt)
        {
            const auto* Field = *FieldIt;
            if (Field->HasAnyPropertyFlags(CPF_Parm))
            { continue; }

            const auto* FieldValuePtr   = Field->ContainerPtrToValuePtr<void>(InValuePtr);
            const auto* DefaultValuePtr = Field->ContainerPtrToValuePtr<void>(DefaultBuffer.GetData());

            const auto MaybeLiteral = UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral(Field, InValuePtr);
            if (NOT MaybeLiteral.IsSet())
            { return {}; }

            if (NOT Field->Identical(FieldValuePtr, DefaultValuePtr))
            { AllAtDefault = false; }

            FieldExpressions.Add(UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(*MaybeLiteral));
        }

        if (AllAtDefault)
        { return MakeRaw(ck::Format_UE(TEXT("{}()"), StructName)); }

        return MakeRaw(ck::Format_UE(TEXT("{}({})"), StructName, FString::Join(FieldExpressions, TEXT(", "))));
    }
}

auto
    UCk_Utils_Reflection_UE::
    Get_PropertyDefaultValueLiteral(
        const FProperty* InProperty,
        const void* InContainer)
    -> TOptional<FCk_PropertyDefaultValueLiteral>
{
    if (ck::Is_NOT_Valid(InProperty, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    if (InContainer == nullptr)
    { return {}; }

    const auto* ValuePtr = InProperty->ContainerPtrToValuePtr<void>(InContainer);

    if (const auto* BoolProp = CastField<FBoolProperty>(InProperty))
    {
        const auto& Value = BoolProp->GetPropertyValue(ValuePtr);
        return ck_reflection_detail::MakeRaw(Value ? TEXT("true") : TEXT("false"));
    }

    if (const auto* EnumProp = CastField<FEnumProperty>(InProperty))
    {
        const auto* UnderlyingProp = EnumProp->GetUnderlyingProperty();
        const auto* Enum = EnumProp->GetEnum();
        if (ck::Is_NOT_Valid(Enum, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        const auto IntValue = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
        const auto NameIndex = Enum->GetIndexByValue(IntValue);
        if (NameIndex == INDEX_NONE)
        { return {}; }

        const auto EnumName = Enum->GetName();
        const auto EntryShort = Enum->GetNameStringByIndex(NameIndex);
        return ck_reflection_detail::MakeRaw(ck::Format_UE(TEXT("{}::{}"), EnumName, EntryShort));
    }

    if (const auto* ByteProp = CastField<FByteProperty>(InProperty))
    {
        const auto ByteValue = ByteProp->GetPropertyValue(ValuePtr);
        if (ck::IsValid(ByteProp->Enum))
        {
            const auto NameIndex = ByteProp->Enum->GetIndexByValue(ByteValue);
            if (NameIndex == INDEX_NONE)
            { return {}; }
            const auto EnumName = ByteProp->Enum->GetName();
            const auto EntryShort = ByteProp->Enum->GetNameStringByIndex(NameIndex);
            return ck_reflection_detail::MakeRaw(ck::Format_UE(TEXT("{}::{}"), EnumName, EntryShort));
        }
        return ck_reflection_detail::MakeRaw(FString::FromInt(static_cast<int32>(ByteValue)));
    }

    if (const auto* IntProp = CastField<FIntProperty>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::FromInt(IntProp->GetPropertyValue(ValuePtr)));
    }

    if (const auto* Int64Prop = CastField<FInt64Property>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::Printf(TEXT("%lld"), Int64Prop->GetPropertyValue(ValuePtr)));
    }

    if (const auto* UInt32Prop = CastField<FUInt32Property>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::Printf(TEXT("%u"), UInt32Prop->GetPropertyValue(ValuePtr)));
    }

    if (const auto* UInt64Prop = CastField<FUInt64Property>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::Printf(TEXT("%llu"), UInt64Prop->GetPropertyValue(ValuePtr)));
    }

    if (const auto* Int16Prop = CastField<FInt16Property>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::FromInt(Int16Prop->GetPropertyValue(ValuePtr)));
    }

    if (const auto* UInt16Prop = CastField<FUInt16Property>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::FromInt(UInt16Prop->GetPropertyValue(ValuePtr)));
    }

    if (const auto* Int8Prop = CastField<FInt8Property>(InProperty))
    {
        return ck_reflection_detail::MakeRaw(FString::FromInt(Int8Prop->GetPropertyValue(ValuePtr)));
    }

    if (const auto* FloatProp = CastField<FFloatProperty>(InProperty))
    {
        const auto Value = FloatProp->GetPropertyValue(ValuePtr);
        return ck_reflection_detail::MakeRaw(ck_reflection_detail::FormatDoubleLiteral(Value) + TEXT("f"));
    }

    if (const auto* DoubleProp = CastField<FDoubleProperty>(InProperty))
    {
        const auto Value = DoubleProp->GetPropertyValue(ValuePtr);
        return ck_reflection_detail::MakeRaw(ck_reflection_detail::FormatDoubleLiteral(Value));
    }

    if (const auto* StrProp = CastField<FStrProperty>(InProperty))
    {
        const auto& Value = StrProp->GetPropertyValue(ValuePtr);
        return FCk_PropertyDefaultValueLiteral{ECk_PropertyDefaultValueKind::String, Value};
    }

    if (const auto* NameProp = CastField<FNameProperty>(InProperty))
    {
        const auto& Value = NameProp->GetPropertyValue(ValuePtr);
        return FCk_PropertyDefaultValueLiteral{ECk_PropertyDefaultValueKind::Name, Value.ToString()};
    }

    if (const auto* TextProp = CastField<FTextProperty>(InProperty))
    {
        const auto& Value = TextProp->GetPropertyValue(ValuePtr);
        return FCk_PropertyDefaultValueLiteral{ECk_PropertyDefaultValueKind::Text, Value.ToString()};
    }

    if (CastField<FObjectPropertyBase>(InProperty) != nullptr
        || CastField<FSoftObjectProperty>(InProperty) != nullptr
        || CastField<FSoftClassProperty>(InProperty) != nullptr
        || CastField<FWeakObjectProperty>(InProperty) != nullptr
        || CastField<FLazyObjectProperty>(InProperty) != nullptr
        || CastField<FClassProperty>(InProperty) != nullptr
        || CastField<FInterfaceProperty>(InProperty) != nullptr)
    {
        // Earlier attempts emitted a typed-null cast (`<Class>(nullptr)`) here to
        // disambiguate positional-ctor overload resolution for plain UObject*
        // fields (the OpenSign-class deadlock — bare `nullptr` reports as
        // `<null handle>` and AS can't bind it). That fix has been reverted:
        // AngelScript rejects `<UClass>(nullptr)` in struct field-default
        // declarations with "Data type can't be '<Class>'" — UObject types
        // cannot be constructed via type-constructor syntax at all, not just
        // AActor/UActorComponent. The same emit path serves both field-default
        // and positional-ctor-arg contexts, so there's no way to apply the
        // typed cast only in the safe context without splitting the literal
        // representation. A proper fix lives in Fix #2 from
        // `codegen-bug-positional-ctor-null-uobject.md` (field-assignment-
        // style emit instead of positional ctor for structs with UObject*
        // fields); until that lands we accept the latent risk that adding
        // a `default Params.X = Y` override on an entity-script subclass
        // whose Params struct contains a UObject* field will bring the
        // OpenSign deadlock back.
        return ck_reflection_detail::MakeRaw(TEXT("nullptr"));
    }

    if (const auto* StructProp = CastField<FStructProperty>(InProperty))
    {
        return ck_reflection_detail::Get_StructLiteral(StructProp, ValuePtr);
    }

    // Containers (FArrayProperty / FMapProperty / FSetProperty / FOptionalProperty) and anything
    // else we don't explicitly handle -> no literal. Caller will omit the initializer.
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_reflection_detail
{
    auto Has_UObjectPointerField_Recursive(const UScriptStruct* InStruct) -> bool
    {
        if (ck::Is_NOT_Valid(InStruct))
        { return false; }

        for (TFieldIterator<FProperty> FieldIt(InStruct, EFieldIteratorFlags::IncludeSuper); FieldIt; ++FieldIt)
        {
            const auto* Field = *FieldIt;
            if (Field->HasAnyPropertyFlags(CPF_Parm))
            { continue; }

            if (CastField<FObjectPropertyBase>(Field) != nullptr
                || CastField<FInterfaceProperty>(Field) != nullptr)
            { return true; }

            if (const auto* NestedStructProp = CastField<FStructProperty>(Field))
            {
                if (Has_UObjectPointerField_Recursive(NestedStructProp->Struct.Get()))
                { return true; }
            }
        }
        return false;
    }

    auto Collect_StructFieldOverrides(
        const UScriptStruct* InStruct,
        const void*          InValuePtr,
        const FString&       InPathPrefix,
        TArray<FCk_StructFieldOverride>& OutOverrides) -> void
    {
        if (ck::Is_NOT_Valid(InStruct) || InValuePtr == nullptr)
        { return; }

        auto DefaultBuffer = TArray<uint8>{};
        DefaultBuffer.SetNumZeroed(InStruct->GetStructureSize());
        InStruct->InitializeStruct(DefaultBuffer.GetData());
        ON_SCOPE_EXIT { InStruct->DestroyStruct(DefaultBuffer.GetData()); };

        for (TFieldIterator<FProperty> FieldIt(InStruct, EFieldIteratorFlags::IncludeSuper); FieldIt; ++FieldIt)
        {
            const auto* Field = *FieldIt;
            if (Field->HasAnyPropertyFlags(CPF_Parm))
            { continue; }

            const auto* FieldValuePtr   = Field->ContainerPtrToValuePtr<void>(InValuePtr);
            const auto* DefaultValuePtr = Field->ContainerPtrToValuePtr<void>(DefaultBuffer.GetData());

            if (Field->Identical(FieldValuePtr, DefaultValuePtr))
            { continue; }

            const auto FieldName = Field->GetName();
            const auto NextPath  = InPathPrefix.IsEmpty()
                ? FieldName
                : (InPathPrefix + TEXT(".") + FieldName);

            // Recurse into nested structs that themselves contain UObject* fields — their
            // positional ctor would carry the same `<null handle>` hazard, so we keep
            // emitting dotted-path assignments instead.
            if (const auto* NestedStructProp = CastField<FStructProperty>(Field))
            {
                if (auto* NestedStruct = NestedStructProp->Struct.Get();
                    Has_UObjectPointerField_Recursive(NestedStruct))
                {
                    Collect_StructFieldOverrides(NestedStruct, FieldValuePtr, NextPath, OutOverrides);
                    continue;
                }
                // Falls through: nested struct has diffs but no UObject* — emit its
                // positional ctor expression at this path; safe in statement context.
            }

            const auto MaybeLiteral = UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral(Field, InValuePtr);
            if (NOT MaybeLiteral.IsSet())
            { continue; }

            auto Entry = FCk_StructFieldOverride{};
            Entry._DottedFieldPath = NextPath;
            Entry._Literal         = *MaybeLiteral;
            OutOverrides.Add(MoveTemp(Entry));
        }
    }
}

auto
    UCk_Utils_Reflection_UE::
    Has_UObjectPointerField(
        const UScriptStruct* InStruct)
    -> bool
{
    return ck_reflection_detail::Has_UObjectPointerField_Recursive(InStruct);
}

auto
    UCk_Utils_Reflection_UE::
    Get_StructFieldOverrides(
        const FStructProperty* InStructProperty,
        const void*            InContainer)
    -> TArray<FCk_StructFieldOverride>
{
    auto Out = TArray<FCk_StructFieldOverride>{};

    if (ck::Is_NOT_Valid(InStructProperty, ck::IsValid_Policy_NullptrOnly{}) || InContainer == nullptr)
    { return Out; }

    auto* Struct = InStructProperty->Struct.Get();
    if (ck::Is_NOT_Valid(Struct))
    { return Out; }

    const auto* StructValuePtr = InStructProperty->ContainerPtrToValuePtr<void>(InContainer);
    ck_reflection_detail::Collect_StructFieldOverrides(Struct, StructValuePtr, FString{}, Out);
    return Out;
}

// --------------------------------------------------------------------------------------------------------------------
