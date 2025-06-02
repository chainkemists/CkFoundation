#include "CkReflection_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

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

        const auto& IsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const auto& IsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
        const auto& IsSettableExternally = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        if (Property->HasAnyPropertyFlags(CPF_Parm) ||
            NOT FBlueprintEditorUtils::PropertyStillExists(Property) ||
            NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible) ||
            NOT IsSettableExternally ||
            NOT IsExposedToSpawn ||
            IsDelegate)
        { continue; }

        ExposedProperties.Add(Property);
    }
#endif

    return ExposedProperties;
}

// --------------------------------------------------------------------------------------------------------------------
