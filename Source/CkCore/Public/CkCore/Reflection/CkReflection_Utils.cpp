#include "CkReflection_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <GameplayTagContainer.h>

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
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
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
            ck::core::Warning(
                TEXT("[ExposedProps] SKIP Class=[{}] Prop=[{}] | IsParm={} StillExists={} BpVisible={} Settable={} ExposedToSpawn={} IsDelegate={}"),
                InClass->GetName(),
                Property->GetName(),
                IsParm, StillExists, IsBpVisible, IsSettableExternally, IsExposedToSpawn, IsDelegate);
            continue;
        }

        ExposedProperties.Add(Property);
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

    auto EscapeForAngelscript(const FString& InRaw) -> FString
    {
        auto Result = InRaw;
        Result.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
        Result.ReplaceInline(TEXT("\""),  TEXT("\\\""));
        Result.ReplaceInline(TEXT("\n"),  TEXT("\\n"));
        Result.ReplaceInline(TEXT("\r"),  TEXT("\\r"));
        Result.ReplaceInline(TEXT("\t"),  TEXT("\\t"));
        return Result;
    }

    // Converts a FCk_PropertyDefaultValueLiteral to its final AngelScript expression string,
    // applying language-specific quoting for String / Name / Text kinds.
    auto ToASExpression(const FCk_PropertyDefaultValueLiteral& InLiteral) -> FString
    {
        switch (InLiteral._Kind)
        {
            case ECk_PropertyDefaultValueKind::RawLiteral:
                return InLiteral._Value;
            case ECk_PropertyDefaultValueKind::String:
                return ck::Format_UE(TEXT("\"{}\""), EscapeForAngelscript(InLiteral._Value));
            case ECk_PropertyDefaultValueKind::Name:
                return ck::Format_UE(TEXT("n\"{}\""), EscapeForAngelscript(InLiteral._Value));
            case ECk_PropertyDefaultValueKind::Text:
                return ck::Format_UE(TEXT("FText::FromString(\"{}\")"), EscapeForAngelscript(InLiteral._Value));
        }
        return {};
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

            FieldExpressions.Add(ToASExpression(*MaybeLiteral));
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
        auto ValueStr = ck::Format_UE(TEXT("{}"), Value);
        if (NOT ValueStr.Contains(TEXT(".")))
        {
            ValueStr += TEXT(".0");
        }
        return ck_reflection_detail::MakeRaw(ValueStr + TEXT("f"));
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
