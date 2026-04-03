#include "CkAngelscriptWrapperGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCVar/CkCVar_Data.h"
#include "CkCVar/Settings/CkCVar_Settings.h"

#include <Blueprint/BlueprintSupport.h>

#include <Engine/BlueprintGeneratedClass.h>

#include <HAL/FileManager.h>

#include <Interfaces/IPluginManager.h>

#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <UObject/UObjectIterator.h>
#include <UObject/PropertyOptional.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    GenerateAllWrappers()
    -> void
{
    ck::angelscriptgenerator::Log(TEXT("=== Generating Angelscript Wrappers from Reflection ==="));

    // Get plugin directory
    auto PluginDir = FPaths::ProjectPluginsDir() / TEXT("CkFoundation");
    auto ScriptDir = PluginDir / TEXT("Script");
    auto GeneratedDir = ScriptDir / TEXT("Generated");

    // Create directories
    IFileManager::Get().MakeDirectory(*ScriptDir, true);
    IFileManager::Get().MakeDirectory(*GeneratedDir, true);

    // Clean old generated files
    auto ExistingFiles = TArray<FString>{};
    IFileManager::Get().FindFiles(ExistingFiles, *GeneratedDir, TEXT("*.as"));
    for (const auto& File : ExistingFiles)
    {
        IFileManager::Get().Delete(*(GeneratedDir / File));
    }
    ck::angelscriptgenerator::Log(TEXT("Cleaned {} existing generated files"), ExistingFiles.Num());

    auto GeneratedFiles = TArray<FString>{};

    // Iterate through all Blueprint Function Library classes
    auto ProcessedCount = int32{0};
    auto SkippedCount = int32{0};

    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto Class = *ClassIterator;

        // Debug: Log all Blueprint Function Libraries we find
        if (Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()) &&
            NOT Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            auto PluginName = Get_PluginNameForClass(Class);
            if (Request_ShouldGenerateWrapperForClass(Class))
            {
                // Debug editor detection here
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

        // Generate wrapper for this class
        Request_GenerateWrapperForClass(Class);

        // Add to generated files list
        auto NamespaceName = Get_ConvertedClassNameToNamespace(Class->GetName());
        GeneratedFiles.Add(NamespaceName + TEXT(".as"));
    }

    ck::angelscriptgenerator::Log(TEXT("Processed {} classes, skipped {} classes"), ProcessedCount, SkippedCount);

    // Generate CVar constants
    Request_GenerateCVarConstants(GeneratedDir);
    GeneratedFiles.Add(TEXT("cvar.as"));

    // Generate master index
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
        FFileHelper::SaveStringToFile(IndexContent, *IndexPath);

        ck::angelscriptgenerator::Log(TEXT("Generated {} Angelscript wrapper files"), GeneratedFiles.Num());
    }
    else
    {
        ck::angelscriptgenerator::Warning(TEXT("No Blueprint Function Libraries found to wrap"));
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

    // Must be a Blueprint Function Library
    if (NOT Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
    { return false; }

    // Skip Blueprint-generated classes - use class flags instead
    if (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
    { return false; }

    // Skip abstract classes
    if (Class->HasAnyClassFlags(CLASS_Abstract))
    { return false; }

    // Only process classes from our CkFoundation plugin
    if (NOT Request_IsClassInCkFoundationPlugin(Class))
    { return false; }

    // Check if this class has any static UFUNCTIONs
    auto HasStaticUFunctions = false;
    for (TFieldIterator<UFunction> FunctionIterator(Class); FunctionIterator; ++FunctionIterator)
    {
        auto Function = *FunctionIterator;
        if (Function->HasAnyFunctionFlags(FUNC_Static))
        {
            HasStaticUFunctions = true;
            break;
        }
    }

    return HasStaticUFunctions;
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

    // Get the package name (e.g., "/Script/CkECS" or "/Script/CkFoundation")
    auto PackageName = Class->GetOutermost()->GetName();

    // Extract module name from package path
    auto ModuleName = FString{};
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return false; // Not a script package
    }

    // Check if this module belongs to CkFoundation plugin
    auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        // Get all modules in the CkFoundation plugin
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

    // Get the package name (e.g., "/Script/CkECS" or "/Script/Engine")
    auto PackageName = Class->GetOutermost()->GetName();

    // Extract module name from package path
    auto ModuleName = FString{};
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return TEXT("Unknown"); // Not a script package
    }

    // Check all plugins to find which one contains this module
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

    // Get the package name (e.g., "/Script/CkEditorGraph")
    auto PackageName = Class->GetOutermost()->GetName();

    // Extract module name from package path
    auto ModuleName = FString{};
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return false;
    }

    // Check if module name contains "Editor" (since your module is "CkEditorGraph")
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

    // Get plugin directory
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

    // Iterate through all functions in the class
    for (TFieldIterator<UFunction> FunctionIterator(Class); FunctionIterator; ++FunctionIterator)
    {
        auto Function = *FunctionIterator;

        // Only process static functions with UFUNCTION meta
        if (NOT Function->HasAnyFunctionFlags(FUNC_Static))
        { continue; }

        // Skip deprecated functions
        if (Function->HasMetaData(TEXT("DeprecatedFunction")))
        {
            ck::angelscriptgenerator::Log(TEXT("    Skipping deprecated function: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        // Skip Blueprint internal use only functions
        if (Function->HasMetaData(TEXT("BlueprintInternalUseOnly")))
        {
            ck::angelscriptgenerator::Log(TEXT("    Skipping internal function: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        // Skip functions with interface parameters or return types
        if (Has_InterfaceTypes(Function))
        {
            ck::angelscriptgenerator::Log(TEXT("    Skipping function with interface types: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        // Check if this function is editor-only
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

        // c++ operator name to Angelscript operator function name (ex. ++ to opPostInc)
        auto OperatorAlias = TMap<FString, FString>{};
    }

    Content += TEXT("}\n");

    if (FunctionCount > 0)
    {
        FFileHelper::SaveStringToFile(Content, *OutputPath);
        ck::angelscriptgenerator::Log(TEXT("    Generated: {}.as with {} functions ({} skipped)"),
                                     NamespaceName, FunctionCount, SkippedFunctionCount);
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
    if (ck::Is_NOT_Valid(Function))
    { return FString{}; }

    auto FunctionName = Function->GetName();

    // Build parameter list
    auto Parameters = TArray<FString>{};
    auto CallParameters = TArray<FString>{};
    auto LocalVariableDeclarations = TArray<FString>{};

    // Check if this function has a WorldContext parameter that should be omitted
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

        // Skip return property
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
        { continue; }

        // Skip WorldContext parameter in Angelscript wrapper
        if (HasWorldContextParam && ParamName == WorldContextParamName)
        {
            // Do NOT add to call parameters - Angelscript handles this automatically
            continue;
        }

        // Check if this is a handle parameter that needs special handling
        auto PropertyType = Get_DetailedPropertyType(Property);
        auto IsNonConstHandleReference = false;

        // Check if this is ANY FCk_Handle type that is a non-const reference
        if (PropertyType.StartsWith(TEXT("FCk_Handle")) &&
            Property->HasAnyPropertyFlags(CPF_ReferenceParm) &&
            NOT Property->HasAnyPropertyFlags(CPF_ConstParm))
        {
            IsNonConstHandleReference = true;
        }

        // Check if this uses ExpandEnumAsExecs of type ECk_SucceededFailed, which we can return as a TOptional instead
        auto IsSucceededFailedEnumExpanded =
            PropertyType == TEXT("ECk_SucceededFailed") &&
            ParamName == ExpandAsEnumsName &&
            NOT ExpandAsEnumsName.IsEmpty();

        if (IsNonConstHandleReference)
        {
            // For non-const handle references, pass by value in Angelscript
            auto LocalVarName = TEXT("_") + ParamName;

            // Get default value if present
            auto DefaultValue = Get_DefaultValueForProperty(Property);
            auto ParamDeclaration = FString::Printf(TEXT("%s %s"), *PropertyType, *ParamName);
            if (NOT DefaultValue.IsEmpty())
            {
                ParamDeclaration += TEXT(" = ") + DefaultValue;
            }

            // Angelscript parameter (by value)
            Parameters.Add(ParamDeclaration);

            // Local variable declaration
            LocalVariableDeclarations.Add(FString::Printf(TEXT("        auto %s = %s;"), *LocalVarName, *ParamName));

            // Use local variable in C++ call
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
            // Normal parameter handling
            auto ParamDeclaration = Get_AngelscriptParameterDeclaration(Property);
            if (NOT ParamDeclaration.IsEmpty())
            {
                Parameters.Add(ParamDeclaration);
                CallParameters.Add(Property->GetName());
            }
        }
    }

    // Get return type - use more detailed type extraction
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
            // Add const if it's a const reference
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

    // Generate the wrapper function
    auto Result = FString{};

    if (IsEditorOnly)
    {
        Result += TEXT("#if editor\n");
    }

    Result += ck::Format_UE(TEXT("    {}\n"), ReturnType);
    Result += ck::Format_UE(TEXT("    {}("), FunctionName);

    if (Parameters.Num() > 0)
    {
        Result += FString::Join(Parameters, TEXT(", "));
    }

    Result += TEXT(")\n    {\n");

    // Add local variable declarations for handle conversions
    for (const auto& LocalVar : LocalVariableDeclarations)
    {
        Result += LocalVar + TEXT("\n");
    }

    // Function body
    Result += TEXT("        ");
    if (UsingSucceededFailedEnumExpanded)
    {
        Result += TEXT("auto ReturnVal = ");
    }
    else if (ReturnType != TEXT("void"))
    {
        Result += TEXT("return ");
    }

    // Ensure class name starts with U for the C++ call
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

        Result += ck::Format_UE(TEXT("{}.{}("), CallParameters[0], FunctionName);
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
    if (ck::Is_NOT_Valid(Property))
    { return TEXT("void"); }

    // Handle TArray properties specifically
    if (auto ArrayProp = CastField<FArrayProperty>(Property))
    {
        auto InnerType = Get_DetailedPropertyType(ArrayProp->Inner);
        return ck::Format_UE(TEXT("TArray<{}>"), InnerType);
    }

    // Handle TMap properties
    if (auto MapProp = CastField<FMapProperty>(Property))
    {
        auto KeyType = Get_DetailedPropertyType(MapProp->KeyProp);
        auto ValueType = Get_DetailedPropertyType(MapProp->ValueProp);
        return ck::Format_UE(TEXT("TMap<{}, {}>"), KeyType, ValueType);
    }

    // Handle TSet properties
    if (auto SetProp = CastField<FSetProperty>(Property))
    {
        auto ElementType = Get_DetailedPropertyType(SetProp->ElementProp);
        return ck::Format_UE(TEXT("TSet<{}>"), ElementType);
    }

    if (auto OptionalProp = CastField<FOptionalProperty>(Property))
    {
        auto ValueType = Get_DetailedPropertyType(OptionalProp->GetValueProperty());
        return ck::Format_UE(TEXT("TOptional<{}>"), ValueType);
    }

    // Handle enum properties (including TEnumAsByte)
    if (auto EnumProp = CastField<FEnumProperty>(Property))
    {
        return EnumProp->GetEnum()->GetName();
    }

    if (auto ByteProp = CastField<FByteProperty>(Property))
    {
        if (ck::IsValid(ByteProp->Enum))
        {
            return ByteProp->Enum->GetName();
        }
    }

    // Handle float properties - convert to float32 for Angelscript
    if (auto FloatProp = CastField<FFloatProperty>(Property))
    {
        return TEXT("float32");
    }

    // Handle double properties - convert to float64 for Angelscript
    if (auto DoubleProp = CastField<FDoubleProperty>(Property))
    {
        return TEXT("float64");
    }

    // Fall back to the CPP type and clean it
    auto CppType = Property->GetCPPType();
    return Get_ConvertedToAngelscriptType(CppType);
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

    // Handle const
    if (Property->HasAnyPropertyFlags(CPF_ConstParm))
    {
        Result += TEXT("const ");
    }

    // Get detailed type
    auto AsType = Get_DetailedPropertyType(Property);
    Result += AsType;

    // Handle references (UPARAM(ref) or reference parameters)
    if (Property->HasAnyPropertyFlags(CPF_ReferenceParm | CPF_OutParm))
    {
        Result += TEXT(" &in ");
    }
    else
    {
        Result += TEXT(" ");
    }

    Result += Property->GetName();

    // Add default value if present
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
    auto Result = UnrealType;

    // Handle TArray<Type> - preserve the full template type
    if (Result.StartsWith(TEXT("TArray<")) && Result.EndsWith(TEXT(">")))
    {
        // Keep TArray<Type> as is, but clean up the inner type
        auto StartIndex = int32{7}; // Length of "TArray<"
        auto EndIndex = Result.Len() - 1; // Remove the closing ">"
        auto InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Recursively clean the inner type
        auto CleanInnerType = Get_ConvertedToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TArray<{}>"), CleanInnerType);
        return Result;
    }

    // Handle TMap<KeyType, ValueType> - preserve the full template type
    if (Result.StartsWith(TEXT("TMap<")) && Result.EndsWith(TEXT(">")))
    {
        // Keep TMap<KeyType, ValueType> as is, but clean up the inner types
        auto StartIndex = int32{5}; // Length of "TMap<"
        auto EndIndex = Result.Len() - 1; // Remove the closing ">"
        auto InnerTypes = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Split by comma to get key and value types
        auto TypeParts = TArray<FString>{};
        auto CommaIndex = InnerTypes.Find(TEXT(","));
        if (CommaIndex != INDEX_NONE)
        {
            auto KeyType = InnerTypes.Left(CommaIndex).TrimStartAndEnd();
            auto ValueType = InnerTypes.Mid(CommaIndex + 1).TrimStartAndEnd();

            // Recursively clean both types
            auto CleanKeyType = Get_ConvertedToAngelscriptType(KeyType);
            auto CleanValueType = Get_ConvertedToAngelscriptType(ValueType);
            Result = ck::Format_UE(TEXT("TMap<{}, {}>"), CleanKeyType, CleanValueType);
            return Result;
        }
    }

    // Handle TSet<Type> - preserve the full template type
    if (Result.StartsWith(TEXT("TSet<")) && Result.EndsWith(TEXT(">")))
    {
        auto StartIndex = int32{5}; // Length of "TSet<"
        auto EndIndex = Result.Len() - 1; // Remove the closing ">"
        auto InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Recursively clean the inner type
        auto CleanInnerType = Get_ConvertedToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TSet<{}>"), CleanInnerType);
        return Result;
    }

    // Remove TEnumAsByte wrapper - extract the enum type
    if (Result.StartsWith(TEXT("TEnumAsByte<")) && Result.EndsWith(TEXT(">")))
    {
        // Extract the enum type from TEnumAsByte<EnumType>
        Result = Result.Mid(12); // Remove "TEnumAsByte<"
        Result = Result.Left(Result.Len() - 1); // Remove ">"
    }

    if (Result.StartsWith(TEXT("TOptional<")) && Result.EndsWith(TEXT(">")))
    {
        auto StartIndex = int32{10}; // Length of "TOptional<"
        auto EndIndex = Result.Len() - 1; // Remove the closing ">"
        auto InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Recursively clean the inner type
        auto CleanInnerType = Get_ConvertedToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TOptional<{}>"), CleanInnerType);
        return Result;
    }

    // Remove all pointers - Angelscript doesn't have pointers
    Result = Result.Replace(TEXT("*"), TEXT(""));

    // Remove const at the end
    if (Result.EndsWith(TEXT(" const")))
    {
        Result = Result.Left(Result.Len() - 6);
    }

    // Clean up whitespace
    Result = Result.TrimStartAndEnd();

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptWrapperGenerator::
    Get_ConvertedClassNameToNamespace(
        const FString& ClassName)
    -> FString
{
    auto Result = ClassName;

    // Remove U prefix if present
    if (Result.StartsWith(TEXT("U")))
    {
        Result = Result.Mid(1);
    }

    // Remove common prefixes
    if (Result.StartsWith(TEXT("Ck_")))
    {
        Result = Result.Mid(3);
    }

    // Remove common suffixes
    if (Result.EndsWith(TEXT("_UE")))
    {
        Result = Result.Left(Result.Len() - 3);
    }
    else if (Result.EndsWith(TEXT("FunctionLibrary")))
    {
        Result = Result.Left(Result.Len() - 15);
    }

    // Convert to snake_case - but be careful about existing underscores
    auto Converted = FString{};
    for (auto I = int32{0}; I < Result.Len(); I++)
    {
        auto Char = Result[I];

        // If this is an underscore, just add it
        if (Char == TEXT('_'))
        {
            Converted += TEXT("_");
        }
        // If this is an uppercase letter and we're not at the start
        else if (I > 0 && FChar::IsUpper(Char))
        {
            // Only add underscore if the previous character wasn't already an underscore
            if (Result[I - 1] != TEXT('_'))
            {
                Converted += TEXT("_");
            }
            Converted += FChar::ToLower(Char);
        }
        // Regular character
        else
        {
            Converted += FChar::ToLower(Char);
        }
    }

    return Converted;
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

    // Check return type
    if (auto ReturnProp = Function->GetReturnProperty())
    {
        if (Request_IsInterfaceProperty(ReturnProp))
        { return true; }
    }

    // Check all parameters
    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        auto Property = *PropertyIterator;

        // Skip return property (already checked)
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

    // Check for interface property
    if (auto InterfaceProp = CastField<FInterfaceProperty>(Property))
    {
        return true;
    }

    // Check for object property that references an interface
    if (auto ObjectProp = CastField<FObjectProperty>(Property))
    {
        if (ck::IsValid(ObjectProp->PropertyClass) && ObjectProp->PropertyClass->HasAnyClassFlags(CLASS_Interface))
        {
            return true;
        }
    }

    // Check array inner properties
    if (auto ArrayProp = CastField<FArrayProperty>(Property))
    {
        return Request_IsInterfaceProperty(ArrayProp->Inner);
    }

    // Check map key/value properties
    if (auto MapProp = CastField<FMapProperty>(Property))
    {
        return Request_IsInterfaceProperty(MapProp->KeyProp) || Request_IsInterfaceProperty(MapProp->ValueProp);
    }

    // Check set element properties
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
            // Check for struct type
            if (auto StructProp = CastField<FStructProperty>(Param))
            {
                const auto& PropertyName = StructProp->Struct->GetFName();
                if (StructProp->Struct && MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            // Check for class type (UObject-derived)
            if (auto ObjectProp = CastField<FObjectPropertyBase>(Param))
            {
                const auto& PropertyName = ObjectProp->PropertyClass->GetFName();
                if (ObjectProp->PropertyClass && MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            // FString check
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

    // Check various metadata that might indicate editor-only functions
    if (Function->HasMetaData(TEXT("EditorOnly")))
    { return true; }

    if (Function->HasMetaData(TEXT("WithEditor")))
    { return true; }

    // Check function flags for editor-specific flags
    if (Function->HasAnyFunctionFlags(FUNC_EditorOnly))
    { return true; }

    // Check if any parameter or return type contains "Editor" in the name
    // This is a heuristic - editor types often have "Editor" in their name
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

    // Check if the function is in an editor module as a fallback
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

    // Get the function that owns this property
    auto OwnerStruct = Property->GetOwnerStruct();
    auto OwnerFunction = Cast<UFunction>(OwnerStruct);
    if (ck::Is_NOT_Valid(OwnerFunction))
    { return FString{}; }

    // Check if this property has a default value in the function metadata
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

    // Handle nullptr/NULL -> nullptr
    if (Result == TEXT("nullptr") || Result == TEXT("NULL"))
    {
        return TEXT("nullptr");
    }

    // Handle "None" for object types -> nullptr
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

    // Handle empty struct construction () -> StructType()
    if (Result == TEXT("()"))
    {
        if (CastField<FStructProperty>(Property))
        {
            const auto& StructTypeName = Get_DetailedPropertyType(Property);
            return ck::Format_UE(TEXT("{}()"), StructTypeName);
        }
    }

    // Handle boolean values
    if (Result == TEXT("true") || Result == TEXT("false"))
    {
        return Result;
    }

    // Handle FName properties - must use n"string" syntax in AngelScript
    if (CastField<FNameProperty>(Property))
    {
        // "None" for FName should be NAME_None
        if (Result == TEXT("None") || Result == TEXT("\"None\""))
        {
            return TEXT("NAME_None");
        }

        // Remove existing quotes if present
        if (Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\"")))
        {
            Result = Result.Mid(1, Result.Len() - 2);
        }

        // FName literals in AngelScript use n"string" syntax
        return ck::Format_UE(TEXT("n\"{}\""), Result);
    }

    // Handle TEXT() macro wrapper for strings
    if (Result.StartsWith(TEXT("TEXT(\"")) && Result.EndsWith(TEXT("\")")))
    {
        auto StringContent = Result.Mid(6, Result.Len() - 8);
        return ck::Format_UE(TEXT("\"{}\""), StringContent);
    }

    // Handle string literals that already have quotes
    if (Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\"")))
    {
        return Result;
    }

    // Handle string properties that don't have quotes but should
    if (CastField<FStrProperty>(Property))
    {
        if (NOT Result.StartsWith(TEXT("\"")))
        {
            return ck::Format_UE(TEXT("\"{}\""), Result);
        }
    }

    // Handle FText properties
    if (CastField<FTextProperty>(Property))
    {
        if (NOT Result.StartsWith(TEXT("\"")))
        {
            return ck::Format_UE(TEXT("\"{}\""), Result);
        }
    }

    // Handle enum values - need to keep full type name for Angelscript
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

    // Handle byte enum values
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

    // Handle float values - ensure they have 'f' suffix for float32
    if (CastField<FFloatProperty>(Property))
    {
        if (NOT Result.EndsWith(TEXT("f")) && NOT Result.EndsWith(TEXT("F")))
        {
            Result += TEXT("f");
        }
        return Result;
    }

    // Handle struct default values with named parameters like (R=1.0,G=1.0,B=1.0,A=1.0)
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

    // Handle struct constructors like FVector(1,2,3) or FVector::ZeroVector
    if (Result.StartsWith(TEXT("F")) && Result.Contains(TEXT("(")))
    {
        return Result;
    }

    // Handle static members like FVector::ZeroVector
    if (Result.Contains(TEXT("::")))
    {
        return Result;
    }

    // Handle numeric values
    if (Result.IsNumeric() || (Result.StartsWith(TEXT("-")) && Result.Mid(1).IsNumeric()))
    {
        return Result;
    }

    // Handle simple comma-separated values like "0.000000,1.000000,0.000000"
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
    auto* Settings = UCk_CVar_Settings_UE::Get();
    if (ck::Is_NOT_Valid(Settings, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::angelscriptgenerator::Warning(TEXT("Cannot generate CVar constants — UCk_CVar_Settings_UE not available"));
        return;
    }

    const auto& RegisteredCVars = Settings->Get_RegisteredCVars();
    if (RegisteredCVars.IsEmpty())
    {
        ck::angelscriptgenerator::Log(TEXT("No registered CVars found — skipping cvar.as generation"));
        return;
    }

    auto SortedCVars = RegisteredCVars;
    SortedCVars.Sort([](const FCk_CVarDefinition& A, const FCk_CVarDefinition& B)
    {
        return A.Get_Name().LexicalLess(B.Get_Name());
    });

    auto Content = FString{TEXT("// Auto-generated CVar constants\n")};
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated during editor startup\n");
    Content += TEXT("// Source: UCk_CVar_Settings_UE registered CVars\n\n");
    Content += TEXT("namespace ck::cvar\n{\n");

    for (const auto& Definition : SortedCVars)
    {
        const auto OriginalName = Definition.Get_Name().ToString();

        auto Identifier = OriginalName;
        Identifier.ReplaceCharInline(TEXT('.'), TEXT('_'));
        Identifier.ReplaceCharInline(TEXT('-'), TEXT('_'));

        auto TypeString = FString{};
        switch (Definition.Get_Type())
        {
            case ECk_CVarType::Int32:   TypeString = TEXT("ECk_CVarType::Int32");   break;
            case ECk_CVarType::Float:   TypeString = TEXT("ECk_CVarType::Float");   break;
            case ECk_CVarType::Bool:    TypeString = TEXT("ECk_CVarType::Bool");    break;
            case ECk_CVarType::String:  TypeString = TEXT("ECk_CVarType::String");  break;
            case ECk_CVarType::Command: TypeString = TEXT("ECk_CVarType::Command"); break;
        }

        Content += ck::Format_UE(
            TEXT("    const FCk_CVarRef {} = FCk_CVarRef(FName(\"{}\"), {});\n"),
            Identifier, OriginalName, TypeString);
    }

    Content += TEXT("}\n");

    const auto FilePath = GeneratedDir / TEXT("cvar.as");
    FFileHelper::SaveStringToFile(Content, *FilePath);

    ck::angelscriptgenerator::Log(TEXT("Generated {} CVar constants in cvar.as"), SortedCVars.Num());
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA && WITH_ANGELSCRIPT_CK

#include "AngelscriptBinds.h"

AS_FORCE_LINK const FAngelscriptBinds::FBind GenerateAllFiles(FAngelscriptBinds::EOrder::Early, []
{
    FCkAngelscriptWrapperGenerator::GenerateAllWrappers();
});

#endif
