#include "CkAngelscriptWrapperGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_SharedUtils.h"

#include "CkCVar/CkCVar_Data.h"
#include "CkCVar/Settings/CkCVar_Settings.h"
#include "CkCVar/Utils/CkCVar_Utils.h"

#include <HAL/IConsoleManager.h>

#include <Blueprint/BlueprintSupport.h>

#include <Engine/BlueprintGeneratedClass.h>
#include <Engine/CollisionProfile.h>

#include <PhysicsEngine/PhysicsSettings.h>

#include <HAL/FileManager.h>

#include <Interfaces/IPluginManager.h>
#include <ModuleDescriptor.h>

#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <UObject/UObjectIterator.h>
#include <UObject/PropertyOptional.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

// Identical writes must be skipped: the AS hot-reload checker baselines mtimes, so rewriting
// unchanged bytes costs a soft-reload sweep of every wrapper module after engine init.
static auto
SaveWrapperFile_IfChanged(
    const FString& InContent,
    const FString& InPath) -> bool
{
    auto ExistingContent = FString{};
    if (FFileHelper::LoadFileToString(ExistingContent, *InPath))
    {
        auto ContentForCompare = InContent;
        ContentForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        ExistingContent.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

        if (ExistingContent.Equals(ContentForCompare, ESearchCase::CaseSensitive))
        { return false; }
    }

    return FFileHelper::SaveStringToFile(InContent, *InPath);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    GenerateAllWrappers()
    -> void
{
    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("WrapperGenerator.GenerateAllWrappers")))
    { return; }

    ck::angelscriptgenerator::Log(TEXT("=== Generating Angelscript Wrappers from Reflection ==="));

    auto PluginDir = FPaths::ProjectPluginsDir() / TEXT("CkFoundation");
    auto ScriptDir = PluginDir / TEXT("Script");
    auto GeneratedDir = ScriptDir / TEXT("Generated");

    IFileManager::Get().MakeDirectory(*ScriptDir, true);
    IFileManager::Get().MakeDirectory(*GeneratedDir, true);

    // The `_index.as` manifest scopes stale-cleanup to files THIS generator wrote, and that
    // scoping is load-bearing: this directory is shared with other generators' canonicals, which
    // run AFTER the initial AS compile while we run before it. Never blanket-delete here.
    const auto PreviouslyGeneratedFiles = [&GeneratedDir]() -> TArray<FString>
    {
        auto Result = TArray<FString>{};
        auto IndexContent = FString{};
        if (NOT FFileHelper::LoadFileToString(IndexContent, *(GeneratedDir / TEXT("_index.as"))))
        { return Result; }

        auto IndexLines = TArray<FString>{};
        IndexContent.ParseIntoArrayLines(IndexLines);
        for (auto& Line : IndexLines)
        {
            Line.TrimStartAndEndInline();
            if (Line.RemoveFromStart(TEXT("- ")))
            {
                Result.Add(Line + TEXT(".as"));
            }
        }
        return Result;
    }();

    auto GeneratedFiles = TArray<FString>{};

    auto ProcessedCount = int32{0};
    auto SkippedCount = int32{0};

    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto Class = *ClassIterator;

        if (Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()) &&
            NOT Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            auto PluginName = Get_PluginNameForClass(Class);
            if (Request_ShouldGenerateWrapperForClass(Class))
            {
                auto PackageName = Class->GetOutermost()->GetName();
                auto ModuleName = FString{};
                if (PackageName.StartsWith(TEXT("/Script/")))
                {
                    ModuleName = PackageName.Mid(8);
                }
                auto IsEditor = Request_IsClassInEditorModule(Class);

                ck::angelscriptgenerator::Log(TEXT("  Processing class: {} (Plugin: {}, Module: {}, IsEditor: {})"),
                       Class, PluginName, ModuleName, IsEditor ? TEXT("YES") : TEXT("NO"));
                ProcessedCount++;
            }
            else
            {
                ck::angelscriptgenerator::Log(TEXT("  Skipping class: {} (Plugin: {})"), Class, PluginName);
                SkippedCount++;
            }
        }

        if (NOT Request_ShouldGenerateWrapperForClass(Class))
        { continue; }

        Request_GenerateWrapperForClass(Class);

        auto NamespaceName = Get_ConvertedClassNameToNamespace(Class->GetName());
        GeneratedFiles.Add(NamespaceName + TEXT(".as"));
    }

    ck::angelscriptgenerator::Log(TEXT("Processed {} classes, skipped {} classes"), ProcessedCount, SkippedCount);

    Request_GenerateCVarConstants(GeneratedDir);
    GeneratedFiles.Add(TEXT("cvar.as"));

    Request_GenerateCollisionConstants(GeneratedDir);
    GeneratedFiles.Add(TEXT("collision.as"));

    Request_GeneratePhysicalSurfaceConstants(GeneratedDir);
    GeneratedFiles.Add(TEXT("physicalsurface.as"));

    // TObjectIterator order varies per boot; an order-churned _index.as is an mtime change.
    GeneratedFiles.Sort();

    if (GeneratedFiles.Num() > 0)
    {
        auto IndexContent = FString{TEXT("// Auto-generated index of all Blueprint Function Library wrappers\n")};
        IndexContent += TEXT("// This file lists all available namespaces\n\n");
        IndexContent += TEXT("/*\nAvailable namespaces:\n");

        for (const auto& File : GeneratedFiles)
        {
            auto NamespaceName = FPaths::GetBaseFilename(File);
            IndexContent += ck::Format_UE(TEXT("  - {}\n"), NamespaceName);
        }

        IndexContent += TEXT("*/\n");

        auto IndexPath = GeneratedDir / TEXT("_index.as");
        SaveWrapperFile_IfChanged(IndexContent, IndexPath);

        ck::angelscriptgenerator::Log(TEXT("Generated {} Angelscript wrapper files"), GeneratedFiles.Num());
    }
    else
    {
        ck::angelscriptgenerator::Warning(TEXT("No Blueprint Function Libraries found to wrap"));
    }

    // Files the previous run's manifest claims we own but this run no longer produced. Foreign
    // generators' files never enter the manifest, so they are structurally unreachable here.
    for (const auto& StaleFile : PreviouslyGeneratedFiles)
    {
        if (GeneratedFiles.Contains(StaleFile))
        { continue; }

        if (IFileManager::Get().Delete(*(GeneratedDir / StaleFile), /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true))
        {
            ck::angelscriptgenerator::Log(TEXT("Deleted stale wrapper file [{}] (no longer generated)"), StaleFile);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_ShouldGenerateWrapperForClass(
        UClass* Class)
    -> bool
{
    if (ck::Is_NOT_Valid(Class))
    { return false; }

    if (NOT Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
    { return false; }

    if (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
    { return false; }

    if (Class->HasAnyClassFlags(CLASS_Abstract))
    { return false; }

    if (NOT Request_IsClassInCkFoundationPlugin(Class))
    { return false; }

    auto HasIncludedStaticUFunctions = false;
    for (TFieldIterator<UFunction> FunctionIterator(Class); FunctionIterator; ++FunctionIterator)
    {
        auto Function = *FunctionIterator;
        if (Request_ShouldGenerateWrapperForFunction(Function))
        {
            HasIncludedStaticUFunctions = true;
            break;
        }
    }

    return HasIncludedStaticUFunctions;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_ShouldGenerateWrapperForFunction(
        UFunction* Function)
    -> bool
{
    if (ck::Is_NOT_Valid(Function))
    { return false; }

    if (NOT Function->HasAnyFunctionFlags(FUNC_Static))
    { return false; }

    if (Function->HasMetaData(TEXT("DeprecatedFunction")))
    { return false; }

    if (Function->HasMetaData(TEXT("BlueprintInternalUseOnly")))
    { return false; }

    if (Function->HasMetaData(TEXT("NotInAngelscript")))
    { return false; }

    if (Has_InterfaceTypes(Function))
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_IsClassInCkFoundationPlugin(
        UClass* Class)
    -> bool
{
    if (ck::Is_NOT_Valid(Class))
    { return false; }

    auto PackageName = Class->GetOutermost()->GetName();

    auto ModuleName = FString{};
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return false; // Not a script package
    }

    auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        const auto& Descriptor = Plugin->GetDescriptor();
        for (const auto& ModuleDesc : Descriptor.Modules)
        {
            if (ModuleDesc.Name.ToString() == ModuleName)
            {
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_PluginNameForClass(
        UClass* Class)
    -> FString
{
    if (ck::Is_NOT_Valid(Class))
    { return TEXT("Unknown"); }

    auto PackageName = Class->GetOutermost()->GetName();

    auto ModuleName = FString{};
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return TEXT("Unknown"); // Not a script package
    }

    auto AllPlugins = IPluginManager::Get().GetDiscoveredPlugins();
    for (const auto& Plugin : AllPlugins)
    {
        const auto& Descriptor = Plugin->GetDescriptor();
        for (const auto& ModuleDesc : Descriptor.Modules)
        {
            if (ModuleDesc.Name.ToString() == ModuleName)
            {
                return Plugin->GetName();
            }
        }
    }

    return TEXT("Engine"); // Default to Engine if not found in any plugin
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_IsClassInEditorModule(
        UClass* Class)
    -> bool
{
    if (ck::Is_NOT_Valid(Class))
    { return false; }

    auto PackageName = Class->GetOutermost()->GetName();

    auto ModuleName = FString{};
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return false;
    }

    // A module whose descriptor Type is editor-only is absent from a cooked game, so its wrappers
    // must be `#if EDITOR`-guarded. The name-substring heuristic below misses e.g. "CkFooTooling".
    const auto IsEditorOnlyHostType = [](EHostType::Type InType) -> bool
    {
        switch (InType)
        {
            case EHostType::Editor:
            case EHostType::EditorNoCommandlet:
            case EHostType::EditorAndProgram:
            case EHostType::UncookedOnly:
            case EHostType::DeveloperTool:
            case EHostType::Developer:
            case EHostType::Program:
                return true;
            default:
                return false;
        }
    };

    for (const auto& Plugin : IPluginManager::Get().GetDiscoveredPlugins())
    {
        for (const auto& ModuleDesc : Plugin->GetDescriptor().Modules)
        {
            if (ModuleDesc.Name.ToString() == ModuleName)
            { return IsEditorOnlyHostType(ModuleDesc.Type); }
        }
    }

    // Fallback for a class whose module is in no plugin descriptor (game/engine module).
    return ModuleName.Contains(TEXT("Editor"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_GenerateWrapperForClass(
        UClass* Class)
    -> void
{
    auto ClassName = Class->GetName();
    auto NamespaceName = Get_ConvertedClassNameToNamespace(ClassName);

    auto PluginDir = FPaths::ProjectPluginsDir() / TEXT("CkFoundation");
    auto GeneratedDir = PluginDir / TEXT("Script") / TEXT("Generated");
    auto OutputPath = GeneratedDir / (NamespaceName + TEXT(".as"));

    auto Content = FString{};
    Content += TEXT("// Auto-generated Angelscript wrapper\n");
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated during editor startup\n");
    Content += ck::Format_UE(TEXT("// Source class: {}\n\n"), ClassName);

    Content += ck::Format_UE(TEXT("namespace {}\n{{\n"), NamespaceName);

    auto FunctionCount = int32{0};
    auto SkippedFunctionCount = int32{0};

    const auto& ScriptMixinMetaData = Class->GetMetaData(TEXT("ScriptMixin"));

    for (TFieldIterator<UFunction> FunctionIterator(Class); FunctionIterator; ++FunctionIterator)
    {
        auto Function = *FunctionIterator;

        if (NOT Request_ShouldGenerateWrapperForFunction(Function))
        {
            ck::angelscriptgenerator::Log(TEXT("    Skipping function excluded from wrapper generation: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        auto IsEditorOnly = Request_IsEditorOnlyFunction(Function);
        if (IsEditorOnly)
        {
            ck::angelscriptgenerator::Log(TEXT("    Editor-only function: {}"), Function->GetName());
        }

        auto WrapperFunction = FString{};

        auto IsMixin = Request_IsScriptMixin(Function, ScriptMixinMetaData);

        WrapperFunction = Get_GeneratedWrapperFunction(Function, ClassName, IsEditorOnly, IsMixin);

        if (NOT WrapperFunction.IsEmpty())
        {
            Content += WrapperFunction;
            FunctionCount++;
        }

        auto OperatorAlias = TMap<FString, FString>{};
    }

    Content += TEXT("}\n");

    if (FunctionCount > 0)
    {
        if (SaveWrapperFile_IfChanged(Content, OutputPath))
        {
            ck::angelscriptgenerator::Log(TEXT("    Generated: {}.as with {} functions ({} skipped)"),
                                         NamespaceName, FunctionCount, SkippedFunctionCount);
        }
    }
    else
    {
        ck::angelscriptgenerator::Log(TEXT("    Skipped: {} (no functions found)"), NamespaceName);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_GeneratedWrapperFunction(
        UFunction* Function,
        const FString& ClassName,
        bool IsEditorOnly,
        bool IsMixin)
    -> FString
{
    if (NOT Request_ShouldGenerateWrapperForFunction(Function))
    { return FString{}; }

    auto FunctionName = Function->GetName();

    auto Parameters = TArray<FString>{};
    auto CallParameters = TArray<FString>{};
    auto LocalVariableDeclarations = TArray<FString>{};

    auto HasWorldContextParam = Function->HasMetaData(TEXT("WorldContext"));
    auto WorldContextParamName = FString{};
    if (HasWorldContextParam)
    {
        WorldContextParamName = Function->GetMetaData(TEXT("WorldContext"));
    }

    const auto& ExpandAsEnumsName = [&]()
    {
        if (Function->HasMetaData(TEXT("ExpandEnumAsExecs")))
        {
            return Function->GetMetaData(TEXT("ExpandEnumAsExecs"));
        }
        return FString{};
    }();

    auto UsingSucceededFailedEnumExpanded = false;

    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        auto Property = *PropertyIterator;

        auto ParamName = Property->GetName();

        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        { continue; }

        // Dropped entirely, not forwarded — AngelScript supplies world context automatically.
        if (HasWorldContextParam && ParamName == WorldContextParamName)
        {
            continue;
        }

        auto PropertyType = Get_DetailedPropertyType(Property);
        auto IsNonConstHandleReference = false;

        if (PropertyType.StartsWith(TEXT("FCk_Handle")) &&
            Property->HasAnyPropertyFlags(CPF_ReferenceParm) &&
            NOT Property->HasAnyPropertyFlags(CPF_ConstParm))
        {
            IsNonConstHandleReference = true;
        }

        // An ECk_SucceededFailed exec-expansion becomes a TOptional return on the AS side.
        auto IsSucceededFailedEnumExpanded =
            PropertyType == TEXT("ECk_SucceededFailed") &&
            ParamName == ExpandAsEnumsName &&
            NOT ExpandAsEnumsName.IsEmpty();

        if (IsNonConstHandleReference)
        {
            // Taken by value on the AS side, then re-bound to a local so the C++ call has an
            // lvalue for its non-const reference parameter.
            auto LocalVarName = TEXT("_") + ParamName;

            auto DefaultValue = Get_DefaultValueForProperty(Property);
            auto ParamDeclaration = FString::Printf(TEXT("%s %s"), *PropertyType, *ParamName);
            if (NOT DefaultValue.IsEmpty())
            {
                ParamDeclaration += TEXT(" = ") + DefaultValue;
            }

            Parameters.Add(ParamDeclaration);
            LocalVariableDeclarations.Add(FString::Printf(TEXT("        auto %s = %s;"), *LocalVarName, *ParamName));
            CallParameters.Add(LocalVarName);
        }
        else if (IsSucceededFailedEnumExpanded)
        {
            CallParameters.Add(Property->GetName());
            UsingSucceededFailedEnumExpanded = true;
            LocalVariableDeclarations.Add(ck::Format_UE(TEXT("        auto {} = ECk_SucceededFailed::Failed;"), ExpandAsEnumsName));
        }
        else
        {
            auto ParamDeclaration = Get_AngelscriptParameterDeclaration(Property);
            if (NOT ParamDeclaration.IsEmpty())
            {
                Parameters.Add(ParamDeclaration);
                CallParameters.Add(Property->GetName());
            }
        }
    }

    auto ReturnProperty = Function->GetReturnProperty();
    auto ReturnType = FString{TEXT("void")};
    if (ck::IsValid(ReturnProperty))
    {
        ReturnType = Get_DetailedPropertyType(ReturnProperty);

        if (UsingSucceededFailedEnumExpanded)
        {
            // TOptional in AS does not work with const ref types
            ReturnType = ck::Format_UE(TEXT("TOptional<{}>"), ReturnType);
        }
        else if (ReturnProperty->HasAnyPropertyFlags(CPF_ReferenceParm))
        {
            if (ReturnProperty->HasAnyPropertyFlags(CPF_ConstParm))
            {
                ReturnType = TEXT("const ") + ReturnType + TEXT("&");
            }
            else
            {
                ReturnType = ReturnType + TEXT("&");
            }
        }
        else if (ReturnProperty->HasAnyPropertyFlags(CPF_ConstParm))
        {
            ReturnType = TEXT("const ") + ReturnType;
        }
    }

    auto Result = FString{};

    if (IsEditorOnly)
    {
        Result += TEXT("#if EDITOR\n");
    }

    Result += ck::Format_UE(TEXT("    {}\n"), ReturnType);
    Result += ck::Format_UE(TEXT("    {}("), FunctionName);

    if (Parameters.Num() > 0)
    {
        Result += FString::Join(Parameters, TEXT(", "));
    }

    Result += TEXT(")\n    {\n");

    for (const auto& LocalVar : LocalVariableDeclarations)
    {
        Result += LocalVar + TEXT("\n");
    }

    Result += TEXT("        ");
    if (UsingSucceededFailedEnumExpanded)
    {
        Result += TEXT("auto ReturnVal = ");
    }
    else if (ReturnType != TEXT("void"))
    {
        Result += TEXT("return ");
    }

    auto CppClassName = ClassName;
    if (NOT CppClassName.StartsWith(TEXT("U")))
    {
        CppClassName = TEXT("U") + CppClassName;
    }

    if (IsMixin)
    {
        CK_ENSURE_IF_NOT(CallParameters.Num() > 0,
            TEXT("Mixin for function [{}] has NO call parameters, this should not be possible!"),
            Function)
        { return {}; }

        // Hazelight's binder names the bound AS method from ScriptName, falling back to
        // ScriptMethod — the call expression must match or the emitted .as references nothing.
        auto MixinCallName = FunctionName;
        if (const auto* ScriptName = Function->FindMetaData(TEXT("ScriptName"));
            ScriptName != nullptr && NOT ScriptName->IsEmpty())
        {
            MixinCallName = *ScriptName;
        }
        else if (const auto* ScriptMethod = Function->FindMetaData(TEXT("ScriptMethod"));
            ScriptMethod != nullptr && NOT ScriptMethod->IsEmpty())
        {
            MixinCallName = *ScriptMethod;
        }

        Result += ck::Format_UE(TEXT("{}.{}("), CallParameters[0], MixinCallName);
    }
    else
    {
        Result += ck::Format_UE(TEXT("{}::{}("), CppClassName, FunctionName);
    }

    if (IsMixin)
    {
        CallParameters.RemoveAt(0);
    }

    if (CallParameters.Num() > 0)
    {
        Result += FString::Join(CallParameters, TEXT(", "));
    }

    Result += TEXT(");\n");

    if (UsingSucceededFailedEnumExpanded)
    {
        Result += ck::Format_UE(TEXT("        return {} == ECk_SucceededFailed::Succeeded ? {}(ReturnVal) : {}();\n"), ExpandAsEnumsName, ReturnType, ReturnType);
    }

    Result += TEXT("    }\n");

    if (IsEditorOnly)
    {
        Result += TEXT("#endif\n");
    }

    Result += TEXT("\n");

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_DetailedPropertyType(
        FProperty* Property)
    -> FString
{
    return FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(Property);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_AngelscriptParameterDeclaration(
        FProperty* Property)
    -> FString
{
    if (ck::Is_NOT_Valid(Property))
    { return FString{}; }

    auto Result = FString{};

    if (Property->HasAnyPropertyFlags(CPF_ConstParm))
    {
        Result += TEXT("const ");
    }

    auto AsType = Get_DetailedPropertyType(Property);
    Result += AsType;

    if (Property->HasAnyPropertyFlags(CPF_ReferenceParm | CPF_OutParm))
    {
        Result += TEXT(" &in ");
    }
    else
    {
        Result += TEXT(" ");
    }

    Result += Property->GetName();

    auto DefaultValue = Get_DefaultValueForProperty(Property);
    if (NOT DefaultValue.IsEmpty())
    {
        Result += TEXT(" = ") + DefaultValue;
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_ConvertedToAngelscriptType(
        const FString& UnrealType)
    -> FString
{
    return FCkAngelscriptGenerator_SharedUtils::Get_ConvertedToAngelscriptType(UnrealType);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_ConvertedClassNameToNamespace(
        const FString& ClassName)
    -> FString
{
    return FCkAngelscriptGenerator_SharedUtils::Get_ConvertedClassNameToNamespace(ClassName);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Has_InterfaceTypes(
        UFunction* Function)
    -> bool
{
    if (ck::Is_NOT_Valid(Function))
    { return false; }

    if (auto ReturnProp = Function->GetReturnProperty())
    {
        if (Request_IsInterfaceProperty(ReturnProp))
        { return true; }
    }

    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        auto Property = *PropertyIterator;

        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        { continue; }

        if (Request_IsInterfaceProperty(Property))
        { return true; }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_IsInterfaceProperty(
        FProperty* Property)
    -> bool
{
    if (ck::Is_NOT_Valid(Property))
    { return false; }

    if (auto InterfaceProp = CastField<FInterfaceProperty>(Property))
    {
        return true;
    }

    if (auto ObjectProp = CastField<FObjectProperty>(Property))
    {
        if (ck::IsValid(ObjectProp->PropertyClass) && ObjectProp->PropertyClass->HasAnyClassFlags(CLASS_Interface))
        {
            return true;
        }
    }

    if (auto ArrayProp = CastField<FArrayProperty>(Property))
    {
        return Request_IsInterfaceProperty(ArrayProp->Inner);
    }

    if (auto MapProp = CastField<FMapProperty>(Property))
    {
        return Request_IsInterfaceProperty(MapProp->KeyProp) || Request_IsInterfaceProperty(MapProp->ValueProp);
    }

    if (auto SetProp = CastField<FSetProperty>(Property))
    {
        return Request_IsInterfaceProperty(SetProp->ElementProp);
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_IsScriptMixin(
        UFunction* Function,
        const FString& MixinMetadata)
    -> bool
{
    if (ck::Is_NOT_Valid(Function))
    { return false; }

    if (MixinMetadata.IsEmpty())
    { return false; }

    auto MixinNames = TArray<FString>{};
    MixinMetadata.ParseIntoArray(MixinNames, TEXT(" "), true);
    for (auto& Name : MixinNames)
    {
        Name = Name.RightChop(1);
    }

    CK_ENSURE_IF_NOT(NOT MixinNames.Contains("GameplayTag"),
        TEXT("Trying to use FGameplayTag as a script mixin for function [{}] but that type is not currently supported!"),
        Function)
    { return false; }

    for (TFieldIterator<FProperty> It(Function); It; ++It)
    {
        auto Param = *It;
        if (Param->HasAnyPropertyFlags(CPF_Parm) &&
            NOT Param->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            if (auto StructProp = CastField<FStructProperty>(Param))
            {
                const auto& PropertyName = StructProp->Struct->GetFName();
                if (StructProp->Struct && MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            if (auto ObjectProp = CastField<FObjectPropertyBase>(Param))
            {
                const auto& PropertyName = ObjectProp->PropertyClass->GetFName();
                if (ObjectProp->PropertyClass && MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            if (auto StrProp = CastField<FStrProperty>(Param))
            {
                const auto& PropertyName = TEXT("String");
                if (MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            // Only check the first parameter
            break;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_IsEditorOnlyFunction(
        UFunction* Function)
    -> bool
{
    if (ck::Is_NOT_Valid(Function))
    { return false; }

    if (Function->HasMetaData(TEXT("EditorOnly")))
    { return true; }

    if (Function->HasMetaData(TEXT("WithEditor")))
    { return true; }

    if (Function->HasAnyFunctionFlags(FUNC_EditorOnly))
    { return true; }

    // Heuristic: editor types conventionally carry "Editor" in their type name.
    if (auto ReturnProp = Function->GetReturnProperty())
    {
        auto ReturnType = ReturnProp->GetCPPType();
        if (ReturnType.Contains(TEXT("Editor")))
        { return true; }
    }

    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        auto Property = *PropertyIterator;
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        { continue; }

        auto PropertyType = Property->GetCPPType();
        if (PropertyType.Contains(TEXT("Editor")))
        { return true; }
    }

    return Request_IsClassInEditorModule(Function->GetOwnerClass());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_DefaultValueForProperty(
        FProperty* Property)
    -> FString
{
    if (ck::Is_NOT_Valid(Property))
    { return FString{}; }

    auto OwnerStruct = Property->GetOwnerStruct();
    auto OwnerFunction = Cast<UFunction>(OwnerStruct);
    if (ck::Is_NOT_Valid(OwnerFunction))
    { return FString{}; }

    auto MetaKey = ck::Format_UE(TEXT("CPP_Default_{}"), Property->GetName());
    if (OwnerFunction->HasMetaData(*MetaKey))
    {
        auto DefaultValue = OwnerFunction->GetMetaData(*MetaKey);
        return Get_ConvertedDefaultValueToAngelscript(DefaultValue, Property);
    }

    return FString{};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_ConvertedDefaultValueToAngelscript(
        const FString& CppDefaultValue,
        FProperty* Property)
    -> FString
{
    if (CppDefaultValue.IsEmpty())
    { return FString{}; }

    auto Result = CppDefaultValue;

    if (Result == TEXT("nullptr") || Result == TEXT("NULL"))
    {
        return TEXT("nullptr");
    }

    if (Result == TEXT("None"))
    {
        if (CastField<FObjectPropertyBase>(Property) ||
            CastField<FSoftObjectProperty>(Property) ||
            CastField<FClassProperty>(Property) ||
            CastField<FSoftClassProperty>(Property))
        {
            return TEXT("nullptr");
        }
    }

    if (Result == TEXT("()"))
    {
        if (CastField<FStructProperty>(Property))
        {
            const auto& StructTypeName = Get_DetailedPropertyType(Property);
            return ck::Format_UE(TEXT("{}()"), StructTypeName);
        }
    }

    if (Result == TEXT("true") || Result == TEXT("false"))
    {
        return Result;
    }

    if (CastField<FNameProperty>(Property))
    {
        if (Result == TEXT("None") || Result == TEXT("\"None\""))
        {
            return TEXT("NAME_None");
        }

        if (Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\"")))
        {
            Result = Result.Mid(1, Result.Len() - 2);
        }

        // FName literals in AngelScript use n"string" syntax.
        return ck::Format_UE(TEXT("n\"{}\""), Result);
    }

    if (Result.StartsWith(TEXT("TEXT(\"")) && Result.EndsWith(TEXT("\")")))
    {
        auto StringContent = Result.Mid(6, Result.Len() - 8);
        return ck::Format_UE(TEXT("\"{}\""), StringContent);
    }

    if (Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\"")))
    {
        return Result;
    }

    if (CastField<FStrProperty>(Property))
    {
        if (NOT Result.StartsWith(TEXT("\"")))
        {
            return ck::Format_UE(TEXT("\"{}\""), Result);
        }
    }

    if (CastField<FTextProperty>(Property))
    {
        if (NOT Result.StartsWith(TEXT("\"")))
        {
            return ck::Format_UE(TEXT("\"{}\""), Result);
        }
    }

    // AngelScript needs the full enum type name on every enumerator.
    if (const auto EnumProp = CastField<FEnumProperty>(Property))
    {
        const auto& EnumTypeName = EnumProp->GetEnum()->GetName();

        if (Result.Contains(TEXT("::")))
        {
            const auto ScopeIndex = Result.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            if (ScopeIndex != INDEX_NONE)
            {
                const auto& EnumValue = Result.Mid(ScopeIndex + 2).TrimStartAndEnd();
                return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, EnumValue);
            }
        }
        else
        {
            return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, Result.TrimStartAndEnd());
        }
    }

    if (const auto ByteProp = CastField<FByteProperty>(Property))
    {
        if (ck::IsValid(ByteProp->Enum))
        {
            const auto& EnumTypeName = ByteProp->Enum->GetName();

            if (Result.Contains(TEXT("::")))
            {
                const auto ScopeIndex = Result.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
                if (ScopeIndex != INDEX_NONE)
                {
                    const auto& EnumValue = Result.Mid(ScopeIndex + 2).TrimStartAndEnd();
                    return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, EnumValue);
                }
            }
            else
            {
                return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, Result.TrimStartAndEnd());
            }
        }
    }

    // float32 literals need the 'f' suffix in AngelScript.
    if (CastField<FFloatProperty>(Property))
    {
        if (NOT Result.EndsWith(TEXT("f")) && NOT Result.EndsWith(TEXT("F")))
        {
            Result += TEXT("f");
        }
        return Result;
    }

    // UHT named-parameter struct defaults, e.g. "(R=1.0,G=1.0,B=1.0,A=1.0)".
    if (Result.StartsWith(TEXT("(")) && Result.EndsWith(TEXT(")")) && Result.Contains(TEXT("=")))
    {
        const auto& PropertyTypeName = Get_DetailedPropertyType(Property);

        auto InnerValues = Result.Mid(1, Result.Len() - 2);

        auto NamedParams = TArray<FString>{};
        InnerValues.ParseIntoArray(NamedParams, TEXT(","), true);

        auto Values = TArray<FString>{};
        for (const auto& NamedParam : NamedParams)
        {
            const auto& TrimmedParam = NamedParam.TrimStartAndEnd();
            const auto EqualsIndex = TrimmedParam.Find(TEXT("="));
            if (EqualsIndex != INDEX_NONE)
            {
                const auto& Value = TrimmedParam.Mid(EqualsIndex + 1).TrimStartAndEnd();
                Values.Add(Value);
            }
        }

        if (Values.Num() > 0)
        {
            return ck::Format_UE(TEXT("{}({})"), PropertyTypeName, FString::Join(Values, TEXT(",")));
        }
    }

    // Struct constructors and static members alike (FVector(1,2,3), FVector::ZeroVector) are
    // already valid AngelScript.
    if (Result.StartsWith(TEXT("F")) && Result.Contains(TEXT("(")))
    {
        return Result;
    }

    if (Result.Contains(TEXT("::")))
    {
        return Result;
    }

    if (Result.IsNumeric() || (Result.StartsWith(TEXT("-")) && Result.Mid(1).IsNumeric()))
    {
        return Result;
    }

    // Bare component lists, e.g. "0.000000,1.000000,0.000000".
    if (Result.Contains(TEXT(",")) && NOT Result.Contains(TEXT("(")) && NOT Result.Contains(TEXT("=")))
    {
        const auto& PropertyTypeName = Get_DetailedPropertyType(Property);
        return ck::Format_UE(TEXT("{}({})"), PropertyTypeName, Result);
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_GenerateCVarConstants(
        const FString& GeneratedDir)
    -> void
{
    // ---- Collect all CVar names from both engine and custom sources ----

    auto AllNames = TSet<FString>{};

    IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(
        FConsoleObjectVisitor::CreateLambda(
            [&AllNames](const TCHAR* InName, IConsoleObject* InObj)
            {
                if (InObj == nullptr || InObj->TestFlags(ECVF_Unregistered))
                {
                    return;
                }

                if (InObj->AsVariable() == nullptr && InObj->AsCommand() == nullptr)
                {
                    return;
                }

                AllNames.Add(FString{InName});
            }),
        TEXT(""));

    auto* Settings = UCk_CVar_Settings_UE::Get();
    if (ck::IsValid(Settings, ck::IsValid_Policy_NullptrOnly{}))
    {
        for (const auto& Name : Settings->GetAllRegisteredNames())
        {
            AllNames.Add(Name.ToString());
        }
    }

    if (AllNames.IsEmpty())
    {
        ck::angelscriptgenerator::Log(TEXT("No CVars found — skipping cvar.as generation"));
        return;
    }

    // ---- Sort and generate ----

    auto SortedNames = AllNames.Array();
    SortedNames.Sort();

    auto Content = FString{TEXT("// Auto-generated CVar constants\n")};
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated during editor startup\n");
    Content += TEXT("// Source: IConsoleManager + UCk_CVar_Settings_UE registered CVars\n\n");
    Content += TEXT("namespace cvar\n{\n");

    auto GeneratedCount = int32{0};

    for (const auto& OriginalName : SortedNames)
    {
        if (OriginalName.Contains(TEXT("deprecated"), ESearchCase::IgnoreCase))
        { continue; }

        const auto DetectedType = UCk_Utils_CVar_UE::DetectCVarType(FName{*OriginalName});
        if (NOT DetectedType.IsSet())
        { continue; }

        // A command has no value to read or write, so there is nothing to emit a CVarRef for.
        if (DetectedType.GetValue() == ECk_CVarType::Command)
        { continue; }

        auto Identifier = OriginalName;
        for (auto& Char : Identifier)
        {
            if (NOT FChar::IsAlnum(Char))
            {
                Char = TEXT('_');
            }
        }

        if (Identifier.Len() > 0 && FChar::IsDigit(Identifier[0]))
        { continue; }

        auto TypeString = FString{};
        switch (DetectedType.GetValue())
        {
            case ECk_CVarType::Int32:   TypeString = TEXT("ECk_CVarType::Int32");   break;
            case ECk_CVarType::Float:   TypeString = TEXT("ECk_CVarType::Float");   break;
            case ECk_CVarType::Bool:    TypeString = TEXT("ECk_CVarType::Bool");    break;
            case ECk_CVarType::String:  TypeString = TEXT("ECk_CVarType::String");  break;
            default: continue;
        }

        Content += ck::Format_UE(
            TEXT("    const FCk_CVarRef {} = FCk_CVarRef(FName(\"{}\"), {});\n"),
            Identifier, OriginalName, TypeString);

        ++GeneratedCount;
    }

    Content += TEXT("}\n");

    const auto FilePath = GeneratedDir / TEXT("cvar.as");
    if (SaveWrapperFile_IfChanged(Content, FilePath))
    {
        ck::angelscriptgenerator::Log(TEXT("Generated {} CVar constants in cvar.as"), GeneratedCount);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_GenerateCollisionConstants(
        const FString& GeneratedDir)
    -> void
{
    auto* CollisionProfile = UCollisionProfile::Get();
    if (ck::Is_NOT_Valid(CollisionProfile, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::angelscriptgenerator::Warning(TEXT("Cannot generate collision constants — UCollisionProfile not available"));
        return;
    }

    // ---- Helpers ----

    constexpr auto SanitizeIdentifier = [](const FString& InName) -> FString
    {
        auto Identifier = InName;
        for (auto& Char : Identifier)
        {
            if (NOT FChar::IsAlnum(Char))
            {
                Char = TEXT('_');
            }
        }
        return Identifier;
    };

    constexpr auto IsValidIdentifier = [](const FString& InIdentifier) -> bool
    {
        return InIdentifier.Len() > 0 && NOT FChar::IsDigit(InIdentifier[0]);
    };

    // ---- Map of ECollisionChannel enum names by index ----

    static const auto ChannelEnumNames = TMap<int32, FString>{
        {0,  TEXT("ECC_WorldStatic")},
        {1,  TEXT("ECC_WorldDynamic")},
        {2,  TEXT("ECC_Pawn")},
        {3,  TEXT("ECC_Visibility")},
        {4,  TEXT("ECC_Camera")},
        {5,  TEXT("ECC_PhysicsBody")},
        {6,  TEXT("ECC_Vehicle")},
        {7,  TEXT("ECC_Destructible")},
        {8,  TEXT("ECC_EngineTraceChannel1")},
        {9,  TEXT("ECC_EngineTraceChannel2")},
        {10, TEXT("ECC_EngineTraceChannel3")},
        {11, TEXT("ECC_EngineTraceChannel4")},
        {12, TEXT("ECC_EngineTraceChannel5")},
        {13, TEXT("ECC_EngineTraceChannel6")},
        {14, TEXT("ECC_GameTraceChannel1")},
        {15, TEXT("ECC_GameTraceChannel2")},
        {16, TEXT("ECC_GameTraceChannel3")},
        {17, TEXT("ECC_GameTraceChannel4")},
        {18, TEXT("ECC_GameTraceChannel5")},
        {19, TEXT("ECC_GameTraceChannel6")},
        {20, TEXT("ECC_GameTraceChannel7")},
        {21, TEXT("ECC_GameTraceChannel8")},
        {22, TEXT("ECC_GameTraceChannel9")},
        {23, TEXT("ECC_GameTraceChannel10")},
        {24, TEXT("ECC_GameTraceChannel11")},
        {25, TEXT("ECC_GameTraceChannel12")},
        {26, TEXT("ECC_GameTraceChannel13")},
        {27, TEXT("ECC_GameTraceChannel14")},
        {28, TEXT("ECC_GameTraceChannel15")},
        {29, TEXT("ECC_GameTraceChannel16")},
        {30, TEXT("ECC_GameTraceChannel17")},
        {31, TEXT("ECC_GameTraceChannel18")},
    };

    // ---- Determine which channels are trace channels ----

    auto TraceChannelIndices = TSet<int32>{};

    constexpr auto VisibilityChannelIndex = 3;
    constexpr auto CameraChannelIndex     = 4;
    TraceChannelIndices.Add(VisibilityChannelIndex);
    TraceChannelIndices.Add(CameraChannelIndex);

    for (const auto& CustomChannel : CollisionProfile->DefaultChannelResponses)
    {
        if (CustomChannel.bTraceType)
        {
            TraceChannelIndices.Add(static_cast<int32>(CustomChannel.Channel.GetValue()));
        }
    }

    // ---- Collect all active channels ----

    struct FChannelEntry
    {
        FString Name;
        int32 Index;
        bool IsTrace;
    };

    auto Channels = TArray<FChannelEntry>{};

    constexpr auto MaxChannelIndex = 32;
    for (auto Index = 0; Index < MaxChannelIndex; ++Index)
    {
        const auto ChannelName = CollisionProfile->ReturnChannelNameFromContainerIndex(Index);
        if (ChannelName == NAME_None || ChannelName.ToString().IsEmpty())
        { continue; }

        Channels.Add(FChannelEntry{
            ChannelName.ToString(),
            Index,
            TraceChannelIndices.Contains(Index)
        });
    }

    // ---- Collect all profiles ----

    auto ProfileNames = TArray<TSharedPtr<FName>>{};
    UCollisionProfile::GetProfileNames(ProfileNames);

    // ---- Generate file ----

    auto Content = FString{TEXT("// Auto-generated collision constants\n")};
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated during editor startup\n");
    Content += TEXT("// Source: UCollisionProfile channels and profiles\n\n");

    // ---- collision::channel namespace (all channels: FName + ECollisionChannel) ----

    Content += TEXT("namespace collision::channel\n{\n");
    for (const auto& Channel : Channels)
    {
        const auto Identifier = SanitizeIdentifier(Channel.Name);
        if (NOT IsValidIdentifier(Identifier))
        { continue; }

        Content += ck::Format_UE(
            TEXT("    const FName {} = FName(\"{}\");\n"),
            Identifier, Channel.Name);

        if (const auto* EnumName = ChannelEnumNames.Find(Channel.Index))
        {
            Content += ck::Format_UE(
                TEXT("    const ECollisionChannel {}_Channel = ECollisionChannel::{};\n"),
                Identifier, *EnumName);
        }
    }
    Content += TEXT("}\n\n");

    // ---- collision::trace namespace (trace channels only) ----

    Content += TEXT("namespace collision::trace\n{\n");
    for (const auto& Channel : Channels)
    {
        if (NOT Channel.IsTrace)
        { continue; }

        const auto Identifier = SanitizeIdentifier(Channel.Name);
        if (NOT IsValidIdentifier(Identifier))
        { continue; }

        Content += ck::Format_UE(
            TEXT("    const FName {} = FName(\"{}\");\n"),
            Identifier, Channel.Name);

        if (const auto* EnumName = ChannelEnumNames.Find(Channel.Index))
        {
            Content += ck::Format_UE(
                TEXT("    const ECollisionChannel {}_Channel = ECollisionChannel::{};\n"),
                Identifier, *EnumName);
        }
    }
    Content += TEXT("}\n\n");

    // ---- collision::profile namespace ----

    Content += TEXT("namespace collision::profile\n{\n");

    auto SortedProfiles = TArray<FName>{};
    for (const auto& NamePtr : ProfileNames)
    {
        if (ck::IsValid(NamePtr) && NOT NamePtr->IsNone())
        {
            SortedProfiles.Add(*NamePtr);
        }
    }
    SortedProfiles.Sort(FNameLexicalLess{});

    for (const auto& ProfileName : SortedProfiles)
    {
        const auto Identifier = SanitizeIdentifier(ProfileName.ToString());
        if (NOT IsValidIdentifier(Identifier))
        { continue; }

        Content += ck::Format_UE(
            TEXT("    const FName {} = FName(\"{}\");\n"),
            Identifier, ProfileName);
    }
    Content += TEXT("}\n");

    // ---- Write file ----

    const auto FilePath = GeneratedDir / TEXT("collision.as");
    if (SaveWrapperFile_IfChanged(Content, FilePath))
    {
        ck::angelscriptgenerator::Log(TEXT("Generated collision constants in collision.as ({} channels, {} profiles)"),
            Channels.Num(), SortedProfiles.Num());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Request_GeneratePhysicalSurfaceConstants(
        const FString& GeneratedDir)
    -> void
{
    auto* PhysicsSettings = UPhysicsSettings::Get();
    if (ck::Is_NOT_Valid(PhysicsSettings, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::angelscriptgenerator::Warning(TEXT("Cannot generate physical surface constants — UPhysicsSettings not available"));
        return;
    }

    // ---- Helpers ----

    constexpr auto SanitizeIdentifier = [](const FString& InName) -> FString
    {
        auto Identifier = InName;
        for (auto& Char : Identifier)
        {
            if (NOT FChar::IsAlnum(Char))
            {
                Char = TEXT('_');
            }
        }
        return Identifier;
    };

    constexpr auto IsValidIdentifier = [](const FString& InIdentifier) -> bool
    {
        return InIdentifier.Len() > 0 && NOT FChar::IsDigit(InIdentifier[0]);
    };

    constexpr auto SurfaceTypeEnumName = [](int32 InIndex) -> FString
    {
        if (InIndex == 0)
        { return TEXT("SurfaceType_Default"); }
        return ck::Format_UE(TEXT("SurfaceType{}"), InIndex);
    };

    // ---- Collect all configured surfaces ----

    struct FSurfaceEntry
    {
        FString Name;
        int32 Index;
    };

    auto Surfaces = TArray<FSurfaceEntry>{};

    constexpr auto DefaultSurfaceIndex = 0;
    Surfaces.Add(FSurfaceEntry{TEXT("Default"), DefaultSurfaceIndex});

    for (const auto& ConfiguredSurface : PhysicsSettings->PhysicalSurfaces)
    {
        const auto SurfaceName = ConfiguredSurface.Name.ToString();
        if (SurfaceName.IsEmpty())
        { continue; }

        Surfaces.Add(FSurfaceEntry{
            SurfaceName,
            static_cast<int32>(ConfiguredSurface.Type.GetValue())
        });
    }

    // ---- Generate file ----

    auto Content = FString{TEXT("// Auto-generated physical surface constants\n")};
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated during editor startup\n");
    Content += TEXT("// Source: UPhysicsSettings::PhysicalSurfaces\n\n");

    // ---- physical_surface namespace (FName + EPhysicalSurface) ----

    Content += TEXT("namespace physical_surface\n{\n");
    for (const auto& Surface : Surfaces)
    {
        const auto Identifier = SanitizeIdentifier(Surface.Name);
        if (NOT IsValidIdentifier(Identifier))
        { continue; }

        Content += ck::Format_UE(
            TEXT("    const FName {} = FName(\"{}\");\n"),
            Identifier, Surface.Name);

        Content += ck::Format_UE(
            TEXT("    const EPhysicalSurface {}_Type = EPhysicalSurface::{};\n"),
            Identifier, SurfaceTypeEnumName(Surface.Index));
    }
    Content += TEXT("}\n");

    // ---- Write file ----

    const auto FilePath = GeneratedDir / TEXT("physicalsurface.as");
    if (SaveWrapperFile_IfChanged(Content, FilePath))
    {
        ck::angelscriptgenerator::Log(TEXT("Generated physical surface constants in physicalsurface.as ({} surfaces)"),
            Surfaces.Num());
    }
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA && WITH_ANGELSCRIPT_CK

#include "AngelscriptBinds.h"

AS_FORCE_LINK const FAngelscriptBinds::FBind GenerateAllFiles(FAngelscriptBinds::EOrder::Early, []
{
    FCkAngelscriptWrapperGenerator::GenerateAllWrappers();
});

#endif
