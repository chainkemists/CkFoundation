#include "CkDynamic/CkDynamic_FragmentDisplaySchema.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <AngelscriptCodeModule.h>
#include <AngelscriptManager.h>
#include <UObject/Class.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_fragment_display_schema_as
{
    const auto DisplayNameKey = FName{TEXT("DisplayName")};
    auto PostCompileHandle = FDelegateHandle{};

    auto MakeFragmentFallback(const UScriptStruct& InFragmentType) -> FString
    {
        return FName::NameToDisplayString(InFragmentType.GetName(), false);
    }

    auto MakePropertyFallback(const FProperty& InProperty) -> FString
    {
        const auto* Owner = InProperty.GetOwnerStruct();
        const auto AuthoredName = Owner
            ? Owner->GetAuthoredNameForField(&InProperty)
            : InProperty.GetName();

        return FName::NameToDisplayString(AuthoredName, InProperty.IsA<FBoolProperty>());
    }

    auto GetEnum(const FProperty& InProperty) -> const UEnum*
    {
        if (const auto* EnumProperty = CastField<FEnumProperty>(&InProperty))
        { return EnumProperty->GetEnum(); }

        if (const auto* ByteProperty = CastField<FByteProperty>(&InProperty))
        { return ByteProperty->Enum; }

        return nullptr;
    }

    auto FindEnumDescriptor(
        const UEnum& InEnum,
        const TArray<TSharedRef<FAngelscriptModuleDesc>>& InModules)
        -> const FAngelscriptEnumDesc*
    {
        for (const auto& Module : InModules)
        {
            for (const auto& EnumDescriptor : Module->Enums)
            {
                if (EnumDescriptor->Enum == &InEnum)
                { return &EnumDescriptor.Get(); }
            }
        }

        return nullptr;
    }

    auto AddEnumDisplayNames(
        ck::dynamic::FFragmentDisplaySchema& OutSchema,
        const UEnum& InEnum,
        const TArray<TSharedRef<FAngelscriptModuleDesc>>& InModules) -> void
    {
        const auto* EnumDescriptor = FindEnumDescriptor(InEnum, InModules);
        if (EnumDescriptor == nullptr)
        { return; }

        auto DisplayNames = TMap<int64, FString>{};
        const auto ValueCount = FMath::Min(EnumDescriptor->EnumValues.Num(), EnumDescriptor->ValueNames.Num());
        for (auto Index = 0; Index < ValueCount; ++Index)
        {
            const auto Value = int64{EnumDescriptor->EnumValues[Index]};
            if (DisplayNames.Contains(Value))
            { continue; }

            const auto MetadataKey = TPair<FName, int32>{DisplayNameKey, Index};
            if (const auto* CustomDisplayName = EnumDescriptor->Meta.Find(MetadataKey))
            {
                DisplayNames.Add(Value, *CustomDisplayName);
                continue;
            }

            const auto RawName = InEnum.GetNameStringByValue(Value);
            DisplayNames.Add(Value, FName::NameToDisplayString(RawName, false));
        }

        OutSchema.EnumValueDisplayNames.Add(InEnum.GetPathName(), MoveTemp(DisplayNames));
    }

    auto BuildSchema(
        const UScriptStruct& InFragmentType,
        const FAngelscriptClassDesc& InClassDescriptor,
        const TArray<TSharedRef<FAngelscriptModuleDesc>>& InModules)
        -> ck::dynamic::FFragmentDisplaySchema
    {
        auto Schema = ck::dynamic::FFragmentDisplaySchema{};
        if (const auto* CustomDisplayName = InClassDescriptor.Meta.Find(DisplayNameKey);
            CustomDisplayName != nullptr && NOT CustomDisplayName->IsEmpty())
        { Schema.FragmentDisplayName = *CustomDisplayName; }
        else
        { Schema.FragmentDisplayName = MakeFragmentFallback(InFragmentType); }

        for (TFieldIterator<FProperty> It{&InFragmentType}; It; ++It)
        {
            const auto* Property = *It;
            Schema.PropertyDisplayNames.Add(Property->GetFName(), MakePropertyFallback(*Property));

            if (const auto* Enum = GetEnum(*Property))
            { AddEnumDisplayNames(Schema, *Enum, InModules); }
        }

        for (const auto& PropertyDescriptor : InClassDescriptor.Properties)
        {
            const auto PropertyName = FName{*PropertyDescriptor->GetUnrealPropertyName()};
            if (NOT Schema.PropertyDisplayNames.Contains(PropertyName))
            { continue; }

            if (const auto* CustomDisplayName = PropertyDescriptor->Meta.Find(DisplayNameKey))
            { Schema.PropertyDisplayNames.Add(PropertyName, *CustomDisplayName); }
        }

        return Schema;
    }

    auto FindClassDescriptor(
        const UScriptStruct& InFragmentType,
        const TArray<TSharedRef<FAngelscriptModuleDesc>>& InModules)
        -> const FAngelscriptClassDesc*
    {
        for (const auto& Module : InModules)
        {
            for (const auto& ClassDescriptor : Module->Classes)
            {
                if (ClassDescriptor->bIsStruct && ClassDescriptor->Struct == &InFragmentType)
                { return &ClassDescriptor.Get(); }
            }
        }

        return nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    TryBuild_AngelscriptFragmentDisplaySchema(
        const UScriptStruct* InFragmentType,
        FFragmentDisplaySchema& OutSchema)
    -> bool
{
    if (ck::Is_NOT_Valid(InFragmentType))
    { return false; }

    const auto Modules = FAngelscriptManager::Get().GetActiveModules();
    const auto* ClassDescriptor = ck_dynamic_fragment_display_schema_as::FindClassDescriptor(*InFragmentType, Modules);
    if (ClassDescriptor == nullptr)
    { return false; }

    OutSchema = ck_dynamic_fragment_display_schema_as::BuildSchema(*InFragmentType, *ClassDescriptor, Modules);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Refresh_AngelscriptFragmentDisplaySchemas()
    -> bool
{
    const auto CanRefresh = IsInGameThread();
    CK_ENSURE_IF_NOT(CanRefresh,
        TEXT("AngelScript Dynamic Fragment display schemas must refresh on the game thread"))
    {}
    if (NOT CanRefresh)
    { return false; }

    const auto ObservedPaths = Get_ObservedFragmentDisplaySchemaPaths();
    const auto Modules = FAngelscriptManager::Get().GetActiveModules();
    auto Schemas = TMap<FString, FFragmentDisplaySchema>{};

    for (const auto& Module : Modules)
    {
        for (const auto& ClassDescriptor : Module->Classes)
        {
            if (NOT ClassDescriptor->bIsStruct)
            { continue; }

            const auto* FragmentType = Cast<UScriptStruct>(ClassDescriptor->Struct);
            if (ck::Is_NOT_Valid(FragmentType))
            { continue; }

            const auto FragmentTypePath = FragmentType->GetPathName();
            if (NOT ObservedPaths.Contains(FragmentTypePath))
            { continue; }

            Schemas.Add(
                FragmentTypePath,
                ck_dynamic_fragment_display_schema_as::BuildSchema(*FragmentType, ClassDescriptor.Get(), Modules));
        }
    }

    return Replace_AngelscriptFragmentDisplaySchemas(MoveTemp(Schemas));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Startup_AngelscriptFragmentDisplaySchemas()
    -> void
{
    if (ck_dynamic_fragment_display_schema_as::PostCompileHandle.IsValid())
    { return; }

    Refresh_AngelscriptFragmentDisplaySchemas();
    ck_dynamic_fragment_display_schema_as::PostCompileHandle =
        FAngelscriptCodeModule::GetPostCompile().AddLambda([]
        {
            Refresh_AngelscriptFragmentDisplaySchemas();
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Shutdown_AngelscriptFragmentDisplaySchemas()
    -> void
{
    if (NOT ck_dynamic_fragment_display_schema_as::PostCompileHandle.IsValid())
    { return; }

    FAngelscriptCodeModule::GetPostCompile().Remove(ck_dynamic_fragment_display_schema_as::PostCompileHandle);
    ck_dynamic_fragment_display_schema_as::PostCompileHandle.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK
