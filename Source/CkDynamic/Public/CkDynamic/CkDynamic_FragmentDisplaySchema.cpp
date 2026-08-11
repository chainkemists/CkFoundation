#include "CkDynamic/CkDynamic_FragmentDisplaySchema.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <UObject/Class.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_fragment_display_schema
{
    struct FRegistry
    {
        TMap<FString, ck::dynamic::FFragmentDisplaySchema> NativeSchemas;
        TMap<FString, ck::dynamic::FFragmentDisplaySchema> AngelscriptSchemas;
        TSet<FString> ObservedFragmentTypePaths;
    };

    auto GetRegistry() -> FRegistry&
    {
        static auto Registry = FRegistry{};
        return Registry;
    }

    auto IsSchemaValid(
        const FString& InFragmentTypePath,
        const ck::dynamic::FFragmentDisplaySchema& InSchema) -> bool
    {
        if (InFragmentTypePath.IsEmpty() || InSchema.FragmentDisplayName.IsEmpty())
        { return false; }

        for (const auto& [PropertyName, DisplayName] : InSchema.PropertyDisplayNames)
        {
            (void)DisplayName;
            if (PropertyName.IsNone())
            { return false; }
        }

        for (const auto& [EnumTypePath, ValueDisplayNames] : InSchema.EnumValueDisplayNames)
        {
            (void)ValueDisplayNames;
            if (EnumTypePath.IsEmpty())
            { return false; }
        }

        return true;
    }

    auto FindSchema(const FString& InFragmentTypePath) -> const ck::dynamic::FFragmentDisplaySchema*
    {
        const auto& Registry = GetRegistry();
        if (const auto* Native = Registry.NativeSchemas.Find(InFragmentTypePath))
        { return Native; }

        return Registry.AngelscriptSchemas.Find(InFragmentTypePath);
    }

    auto MakePropertyFallback(const FProperty& InProperty) -> FString
    {
        const auto* Owner = InProperty.GetOwnerStruct();
        const auto AuthoredName = Owner
            ? Owner->GetAuthoredNameForField(&InProperty)
            : InProperty.GetName();

        return FName::NameToDisplayString(AuthoredName, InProperty.IsA<FBoolProperty>());
    }

    auto MakeEnumValueFallback(const UEnum& InEnum, int64 InValue) -> FString
    {
        const auto RawName = InEnum.GetNameStringByValue(InValue);
        return RawName.IsEmpty()
            ? FString{}
            : FName::NameToDisplayString(RawName, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Register_NativeFragmentDisplaySchema(
        FString InFragmentTypePath,
        FFragmentDisplaySchema InSchema)
    -> bool
{
    const auto IsGameThreadRegistration = IsInGameThread();
    const auto InputsAreValid = IsGameThreadRegistration &&
        ck_dynamic_fragment_display_schema::IsSchemaValid(InFragmentTypePath, InSchema);

    CK_ENSURE_IF_NOT(InputsAreValid,
        TEXT("Invalid native Dynamic Fragment display schema registration for [{}]"), InFragmentTypePath)
    {}
    if (NOT InputsAreValid)
    { return false; }

    ck_dynamic_fragment_display_schema::GetRegistry().NativeSchemas.Add(
        MoveTemp(InFragmentTypePath), MoveTemp(InSchema));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Unregister_NativeFragmentDisplaySchema(
        const FString& InFragmentTypePath)
    -> bool
{
    const auto InputsAreValid = IsInGameThread() && NOT InFragmentTypePath.IsEmpty();
    CK_ENSURE_IF_NOT(InputsAreValid,
        TEXT("Invalid native Dynamic Fragment display schema unregistration for [{}]"), InFragmentTypePath)
    {}
    if (NOT InputsAreValid)
    { return false; }

    return ck_dynamic_fragment_display_schema::GetRegistry().NativeSchemas.Remove(InFragmentTypePath) > 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    TryGet_NativeFragmentDisplaySchema(
        const FString& InFragmentTypePath,
        FFragmentDisplaySchema& OutSchema)
    -> bool
{
    if (NOT IsInGameThread() || InFragmentTypePath.IsEmpty())
    { return false; }

    const auto* Found = ck_dynamic_fragment_display_schema::GetRegistry().NativeSchemas.Find(InFragmentTypePath);
    if (Found == nullptr)
    { return false; }

    OutSchema = *Found;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    TryGet_FragmentDisplaySchema(
        const FString& InFragmentTypePath,
        FFragmentDisplaySchema& OutSchema)
    -> bool
{
    if (NOT IsInGameThread() || InFragmentTypePath.IsEmpty())
    { return false; }

    const auto* Found = ck_dynamic_fragment_display_schema::FindSchema(InFragmentTypePath);
    if (Found == nullptr)
    { return false; }

    OutSchema = *Found;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Observe_FragmentDisplaySchema(
        const UScriptStruct* InFragmentType)
    -> bool
{
    const auto TypeIsValid = ck::IsValid(InFragmentType);
    const auto CanObserve = TypeIsValid && IsInGameThread();

    CK_ENSURE_IF_NOT(CanObserve, TEXT("Invalid Dynamic Fragment display-schema observation"))
    {}
    if (NOT CanObserve)
    { return false; }

    const auto FragmentTypePath = InFragmentType->GetPathName();
    auto& Registry = ck_dynamic_fragment_display_schema::GetRegistry();
    const auto WasAlreadyObserved = Registry.ObservedFragmentTypePaths.Contains(FragmentTypePath);
    Registry.ObservedFragmentTypePaths.Add(FragmentTypePath);

#if WITH_ANGELSCRIPT_CK
    if (NOT WasAlreadyObserved && NOT Registry.NativeSchemas.Contains(FragmentTypePath))
    {
        auto Schema = FFragmentDisplaySchema{};
        if (TryBuild_AngelscriptFragmentDisplaySchema(InFragmentType, Schema))
        {
            const auto SchemaIsValid = ck_dynamic_fragment_display_schema::IsSchemaValid(FragmentTypePath, Schema);
            CK_ENSURE_IF_NOT(SchemaIsValid,
                TEXT("AngelScript produced an invalid Dynamic Fragment display schema for [{}]"), FragmentTypePath)
            {}
            if (NOT SchemaIsValid)
            { return false; }

            Registry.AngelscriptSchemas.Add(FragmentTypePath, MoveTemp(Schema));
        }
    }
#else
    (void)WasAlreadyObserved;
#endif

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Resolve_FragmentDisplayName(
        const UScriptStruct* InFragmentType)
    -> FString
{
    if (NOT IsInGameThread() || ck::Is_NOT_Valid(InFragmentType))
    { return {}; }

    const auto FragmentTypePath = InFragmentType->GetPathName();
    if (const auto* Schema = ck_dynamic_fragment_display_schema::FindSchema(FragmentTypePath))
    { return Schema->FragmentDisplayName; }

    return FName::NameToDisplayString(InFragmentType->GetName(), false);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Resolve_PropertyDisplayName(
        const UScriptStruct* InFragmentType,
        const FProperty* InProperty)
    -> FString
{
    if (NOT IsInGameThread() || ck::Is_NOT_Valid(InFragmentType) || InProperty == nullptr)
    { return {}; }

    const auto FragmentTypePath = InFragmentType->GetPathName();
    if (const auto* Schema = ck_dynamic_fragment_display_schema::FindSchema(FragmentTypePath))
    {
        if (const auto* DisplayName = Schema->PropertyDisplayNames.Find(InProperty->GetFName()))
        { return *DisplayName; }
    }

    return ck_dynamic_fragment_display_schema::MakePropertyFallback(*InProperty);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Resolve_EnumValueDisplayName(
        const UScriptStruct* InFragmentType,
        const UEnum* InEnum,
        int64 InValue)
    -> FString
{
    if (NOT IsInGameThread() || ck::Is_NOT_Valid(InFragmentType) || ck::Is_NOT_Valid(InEnum))
    { return {}; }

    const auto FragmentTypePath = InFragmentType->GetPathName();
    if (const auto* Schema = ck_dynamic_fragment_display_schema::FindSchema(FragmentTypePath))
    {
        if (const auto* EnumDisplayNames = Schema->EnumValueDisplayNames.Find(InEnum->GetPathName()))
        {
            if (const auto* DisplayName = EnumDisplayNames->Find(InValue))
            { return *DisplayName; }
        }
    }

    return ck_dynamic_fragment_display_schema::MakeEnumValueFallback(*InEnum, InValue);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Replace_AngelscriptFragmentDisplaySchemas(
        TMap<FString, FFragmentDisplaySchema> InSchemas)
    -> bool
{
    const auto IsGameThreadReplacement = IsInGameThread();
    auto InputsAreValid = IsGameThreadReplacement;
    for (const auto& [FragmentTypePath, Schema] : InSchemas)
    {
        InputsAreValid = InputsAreValid &&
            ck_dynamic_fragment_display_schema::IsSchemaValid(FragmentTypePath, Schema);
    }

    CK_ENSURE_IF_NOT(InputsAreValid, TEXT("Invalid AngelScript Dynamic Fragment display-schema replacement"))
    {}
    if (NOT InputsAreValid)
    { return false; }

    // One move is the publication point. Explicit native registrations live in a different map and cannot be erased.
    ck_dynamic_fragment_display_schema::GetRegistry().AngelscriptSchemas = MoveTemp(InSchemas);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Get_ObservedFragmentDisplaySchemaPaths()
    -> TSet<FString>
{
    if (NOT IsInGameThread())
    { return {}; }

    return ck_dynamic_fragment_display_schema::GetRegistry().ObservedFragmentTypePaths;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Get_AngelscriptFragmentDisplaySchemas()
    -> TMap<FString, FFragmentDisplaySchema>
{
    if (NOT IsInGameThread())
    { return {}; }

    return ck_dynamic_fragment_display_schema::GetRegistry().AngelscriptSchemas;
}

// --------------------------------------------------------------------------------------------------------------------
