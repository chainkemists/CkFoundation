#include "CkAngelscriptWrapperGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include <Blueprint/BlueprintSupport.h>

#include <Engine/BlueprintGeneratedClass.h>

#include <HAL/FileManager.h>

#include <Interfaces/IPluginManager.h>

#include <Misc/FileHelper.h>
#include <Misc/Paths.h>

#include <UObject/UObjectIterator.h>

// --------------------------------------------------------------------------------------------------------------------

void FCkAngelscriptWrapperGenerator::GenerateAllWrappers()
{
    ck::angelscriptgenerator::Log(TEXT("=== Generating Angelscript Wrappers from Reflection ==="));

    // Get plugin directory
    FString PluginDir = FPaths::ProjectPluginsDir() / TEXT("CkFoundation");
    FString ScriptDir = PluginDir / TEXT("Script");
    FString GeneratedDir = ScriptDir / TEXT("Generated");

    // Create directories
    IFileManager::Get().MakeDirectory(*ScriptDir, true);
    IFileManager::Get().MakeDirectory(*GeneratedDir, true);

    // Clean old generated files
    TArray<FString> ExistingFiles;
    IFileManager::Get().FindFiles(ExistingFiles, *GeneratedDir, TEXT("*.as"));
    for (const FString& File : ExistingFiles)
    {
        IFileManager::Get().Delete(*(GeneratedDir / File));
    }
    ck::angelscriptgenerator::Log(TEXT("Cleaned {} existing generated files"), ExistingFiles.Num());

    TArray<FString> GeneratedFiles;

    // Iterate through all Blueprint Function Library classes
    int32 ProcessedCount = 0;
    int32 SkippedCount = 0;

    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        UClass* Class = *ClassIterator;

        // Debug: Log all Blueprint Function Libraries we find
        if (Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()) &&
            !Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            FString PluginName = GetPluginNameForClass(Class);
            if (ShouldGenerateWrapperForClass(Class))
            {
                // Debug editor detection here
                FString PackageName = Class->GetOutermost()->GetName();
                FString ModuleName;
                if (PackageName.StartsWith(TEXT("/Script/")))
                {
                    ModuleName = PackageName.Mid(8);
                }
                bool IsEditor = IsClassInEditorModule(Class);

                ck::angelscriptgenerator::Log(TEXT("  Processing class: {} (Plugin: {}, Module: {}, IsEditor: {})"),
                       Class, PluginName, ModuleName, IsEditor ? TEXT("YES") : TEXT("NO"));
                ProcessedCount++;
            }
            else
            {
                ck::angelscriptgenerator::Warning(TEXT("  Skipping class: {} (Plugin: {})"), Class, PluginName);
                SkippedCount++;
            }
        }

        if (!ShouldGenerateWrapperForClass(Class))
            continue;

        // Generate wrapper for this class
        GenerateWrapperForClass(Class);

        // Add to generated files list
        FString NamespaceName = ConvertClassNameToNamespace(Class->GetName());
        GeneratedFiles.Add(NamespaceName + TEXT(".as"));
    }

    ck::angelscriptgenerator::Log(TEXT("Processed {} classes, skipped {} classes"), ProcessedCount, SkippedCount);

    // Generate master index
    if (GeneratedFiles.Num() > 0)
    {
        FString IndexContent = TEXT("// Auto-generated index of all Blueprint Function Library wrappers\n");
        IndexContent += TEXT("// This file lists all available namespaces\n\n");
        IndexContent += TEXT("/*\nAvailable namespaces:\n");

        for (const FString& File : GeneratedFiles)
        {
            FString NamespaceName = FPaths::GetBaseFilename(File);
            IndexContent += ck::Format_UE(TEXT("  - {}\n"), NamespaceName);
        }

        IndexContent += TEXT("*/\n");

        FString IndexPath = GeneratedDir / TEXT("_index.as");
        FFileHelper::SaveStringToFile(IndexContent, *IndexPath);

        ck::angelscriptgenerator::Log(TEXT("Generated {} Angelscript wrapper files"), GeneratedFiles.Num());
    }
    else
    {
        ck::angelscriptgenerator::Warning(TEXT("No Blueprint Function Libraries found to wrap"));
    }
}

bool FCkAngelscriptWrapperGenerator::ShouldGenerateWrapperForClass(UClass* Class)
{
    if (!Class)
        return false;

    // Must be a Blueprint Function Library
    if (!Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
        return false;

    // Skip Blueprint-generated classes - use class flags instead
    if (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        return false;

    // Skip abstract classes
    if (Class->HasAnyClassFlags(CLASS_Abstract))
        return false;

    // Only process classes from our CkFoundation plugin
    if (!IsClassInCkFoundationPlugin(Class))
        return false;

    // Check if this class has any static UFUNCTIONs
    bool HasStaticUFunctions = false;
    for (TFieldIterator<UFunction> FunctionIterator(Class); FunctionIterator; ++FunctionIterator)
    {
        UFunction* Function = *FunctionIterator;
        if (Function->HasAnyFunctionFlags(FUNC_Static))
        {
            HasStaticUFunctions = true;
            break;
        }
    }

    return HasStaticUFunctions;
}

bool FCkAngelscriptWrapperGenerator::IsClassInCkFoundationPlugin(UClass* Class)
{
    if (!Class)
        return false;

    // Get the package name (e.g., "/Script/CkECS" or "/Script/CkFoundation")
    FString PackageName = Class->GetOutermost()->GetName();

    // Extract module name from package path
    FString ModuleName;
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return false; // Not a script package
    }

    // Check if this module belongs to CkFoundation plugin
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (Plugin.IsValid())
    {
        // Get all modules in the CkFoundation plugin
        const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
        for (const FModuleDescriptor& ModuleDesc : Descriptor.Modules)
        {
            if (ModuleDesc.Name.ToString() == ModuleName)
            {
                return true;
            }
        }
    }

    return false;
}

FString FCkAngelscriptWrapperGenerator::GetPluginNameForClass(UClass* Class)
{
    if (!Class)
        return TEXT("Unknown");

    // Get the package name (e.g., "/Script/CkECS" or "/Script/Engine")
    FString PackageName = Class->GetOutermost()->GetName();

    // Extract module name from package path
    FString ModuleName;
    if (PackageName.StartsWith(TEXT("/Script/")))
    {
        ModuleName = PackageName.Mid(8); // Remove "/Script/" prefix
    }
    else
    {
        return TEXT("Unknown"); // Not a script package
    }

    // Check all plugins to find which one contains this module
    TArray<TSharedRef<IPlugin>> AllPlugins = IPluginManager::Get().GetDiscoveredPlugins();
    for (const TSharedRef<IPlugin>& Plugin : AllPlugins)
    {
        const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
        for (const FModuleDescriptor& ModuleDesc : Descriptor.Modules)
        {
            if (ModuleDesc.Name.ToString() == ModuleName)
            {
                return Plugin->GetName();
            }
        }
    }

    return TEXT("Engine"); // Default to Engine if not found in any plugin
}

bool FCkAngelscriptWrapperGenerator::IsClassInEditorModule(UClass* Class)
{
    if (!Class)
        return false;

    // Get the package name (e.g., "/Script/CkEditorGraph")
    FString PackageName = Class->GetOutermost()->GetName();

    // Extract module name from package path
    FString ModuleName;
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

void FCkAngelscriptWrapperGenerator::GenerateWrapperForClass(UClass* Class)
{
    FString ClassName = Class->GetName();
    FString NamespaceName = ConvertClassNameToNamespace(ClassName);

    // Get plugin directory
    FString PluginDir = FPaths::ProjectPluginsDir() / TEXT("CkFoundation");
    FString GeneratedDir = PluginDir / TEXT("Script") / TEXT("Generated");
    FString OutputPath = GeneratedDir / (NamespaceName + TEXT(".as"));

    FString Content;
    Content += TEXT("// Auto-generated Angelscript wrapper\n");
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated during editor startup\n");
    Content += ck::Format_UE(TEXT("// Source class: {}\n\n"), ClassName);

    Content += ck::Format_UE(TEXT("namespace {}\n{{\n"), NamespaceName);

    int32 FunctionCount = 0;
    int32 SkippedFunctionCount = 0;

    const auto& ScriptMixinMetaData = Class->GetMetaData(TEXT("ScriptMixin"));

    // Iterate through all functions in the class
    for (TFieldIterator<UFunction> FunctionIterator(Class); FunctionIterator; ++FunctionIterator)
    {
        UFunction* Function = *FunctionIterator;

        // Only process static functions with UFUNCTION meta
        if (!Function->HasAnyFunctionFlags(FUNC_Static))
            continue;

        // Skip deprecated functions
        if (Function->HasMetaData(TEXT("DeprecatedFunction")))
        {
            ck::angelscriptgenerator::Warning(TEXT("    Skipping deprecated function: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        // Skip Blueprint internal use only functions
        if (Function->HasMetaData(TEXT("BlueprintInternalUseOnly")))
        {
            ck::angelscriptgenerator::Warning(TEXT("    Skipping internal function: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        // Skip functions with interface parameters or return types
        if (HasInterfaceTypes(Function))
        {
            ck::angelscriptgenerator::Warning(TEXT("    Skipping function with interface types: {}"), Function->GetName());
            SkippedFunctionCount++;
            continue;
        }

        // Check if this function is editor-only
        bool IsEditorOnly = IsEditorOnlyFunction(Function);
        if (IsEditorOnly)
        {
            ck::angelscriptgenerator::Log(TEXT("    Editor-only function: {}"), Function->GetName());
        }

        FString WrapperFunction;

        // Skip functions that already generate script mixins
        if (IsScriptMixin(Function, ScriptMixinMetaData))
        {
            WrapperFunction = GenerateWrapperFunctionForMixin(Function, ClassName, IsEditorOnly);
        }
        else
        {
            WrapperFunction = GenerateWrapperFunction(Function, ClassName, IsEditorOnly);
        }

        if (!WrapperFunction.IsEmpty())
        {
            Content += WrapperFunction;
            FunctionCount++;
        }

        // c++ operator name to Angelscript operator function name (ex. ++ to opPostInc)
        TMap<FString, FString> OperatorAlias;
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
        ck::angelscriptgenerator::Warning(TEXT("    Skipped: {} (no functions found)"), NamespaceName);
    }
}

FString FCkAngelscriptWrapperGenerator::GenerateWrapperFunction(UFunction* Function, const FString& ClassName, bool IsEditorOnly)
{
    if (!Function)
        return FString();

    FString FunctionName = Function->GetName();

    // Get return type - use more detailed type extraction
    FProperty* ReturnProperty = Function->GetReturnProperty();
    FString ReturnType = TEXT("void");
    if (ReturnProperty)
    {
        ReturnType = GetDetailedPropertyType(ReturnProperty);

        if (ReturnProperty->HasAnyPropertyFlags(CPF_ReferenceParm))
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
    }

    // Build parameter list
    TArray<FString> Parameters;
    TArray<FString> CallParameters;
    TArray<FString> LocalVariableDeclarations;

    // Check if this function has a WorldContext parameter that should be omitted
    bool HasWorldContextParam = Function->HasMetaData(TEXT("WorldContext"));
    FString WorldContextParamName;
    if (HasWorldContextParam)
    {
        WorldContextParamName = Function->GetMetaData(TEXT("WorldContext"));
    }

    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        FProperty* Property = *PropertyIterator;

        // Skip return property
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
            continue;

        // Skip WorldContext parameter in Angelscript wrapper
        if (HasWorldContextParam && Property->GetName() == WorldContextParamName)
        {
            // Do NOT add to call parameters - Angelscript handles this automatically
            continue;
        }

        // Check if this is a handle parameter that needs special handling
        FString PropertyType = GetDetailedPropertyType(Property);
        bool IsNonConstHandleReference = false;

        // Check if this is ANY FCk_Handle type that is a non-const reference
        if (PropertyType.StartsWith(TEXT("FCk_Handle")) &&
            Property->HasAnyPropertyFlags(CPF_ReferenceParm) &&
            !Property->HasAnyPropertyFlags(CPF_ConstParm))
        {
            IsNonConstHandleReference = true;
        }

        if (IsNonConstHandleReference)
        {
            // For non-const handle references, pass by value in Angelscript
            FString ParamName = Property->GetName();
            FString LocalVarName = TEXT("_") + ParamName;

            // Get default value if present
            FString DefaultValue = GetDefaultValueForProperty(Property);
            FString ParamDeclaration = FString::Printf(TEXT("%s %s"), *PropertyType, *ParamName);
            if (!DefaultValue.IsEmpty())
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
        else
        {
            // Normal parameter handling
            FString ParamDeclaration = GetAngelscriptParameterDeclaration(Property);
            if (!ParamDeclaration.IsEmpty())
            {
                Parameters.Add(ParamDeclaration);
                CallParameters.Add(Property->GetName());
            }
        }
    }

    // Generate the wrapper function
    FString Result;

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
    for (const FString& LocalVar : LocalVariableDeclarations)
    {
        Result += LocalVar + TEXT("\n");
    }

    // Function body
    Result += TEXT("        ");
    if (ReturnType != TEXT("void"))
    {
        Result += TEXT("return ");
    }

    // Ensure class name starts with U for the C++ call
    FString CppClassName = ClassName;
    if (!CppClassName.StartsWith(TEXT("U")))
    {
        CppClassName = TEXT("U") + CppClassName;
    }

    Result += ck::Format_UE(TEXT("{}::{}("), CppClassName, FunctionName);

    if (CallParameters.Num() > 0)
    {
        Result += FString::Join(CallParameters, TEXT(", "));
    }

    Result += TEXT(");\n");
    Result += TEXT("    }\n");

    if (IsEditorOnly)
    {
        Result += TEXT("#endif\n");
    }

    Result += TEXT("\n");

    return Result;
}


FString FCkAngelscriptWrapperGenerator::GenerateWrapperFunctionForMixin(UFunction* Function, const FString& ClassName, bool IsEditorOnly)
{
    if (!Function)
        return FString();

    FString FunctionName = Function->GetName();

    // Get return type - use more detailed type extraction
    FProperty* ReturnProperty = Function->GetReturnProperty();
    FString ReturnType = TEXT("void");
    if (ReturnProperty)
    {
        ReturnType = GetDetailedPropertyType(ReturnProperty);

        if (ReturnProperty->HasAnyPropertyFlags(CPF_ReferenceParm))
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
    }

    // Build parameter list
    TArray<FString> Parameters;
    TArray<FString> CallParameters;
    TArray<FString> LocalVariableDeclarations;

    // Check if this function has a WorldContext parameter that should be omitted
    bool HasWorldContextParam = Function->HasMetaData(TEXT("WorldContext"));
    FString WorldContextParamName;
    if (HasWorldContextParam)
    {
        WorldContextParamName = Function->GetMetaData(TEXT("WorldContext"));
    }

    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        FProperty* Property = *PropertyIterator;

        // Skip return property
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
            continue;

        // Skip WorldContext parameter in Angelscript wrapper
        if (HasWorldContextParam && Property->GetName() == WorldContextParamName)
        {
            // Do NOT add to call parameters - Angelscript handles this automatically
            continue;
        }

        // Check if this is a handle parameter that needs special handling
        FString PropertyType = GetDetailedPropertyType(Property);
        bool IsNonConstHandleReference = false;

        // Check if this is ANY FCk_Handle type that is a non-const reference
        if (PropertyType.StartsWith(TEXT("FCk_Handle")) &&
            Property->HasAnyPropertyFlags(CPF_ReferenceParm) &&
            !Property->HasAnyPropertyFlags(CPF_ConstParm))
        {
            IsNonConstHandleReference = true;
        }

        if (IsNonConstHandleReference)
        {
            // For non-const handle references, pass by value in Angelscript
            FString ParamName = Property->GetName();
            FString LocalVarName = TEXT("_") + ParamName;

            // Get default value if present
            FString DefaultValue = GetDefaultValueForProperty(Property);
            FString ParamDeclaration = FString::Printf(TEXT("%s %s"), *PropertyType, *ParamName);
            if (!DefaultValue.IsEmpty())
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
        else
        {
            // Normal parameter handling
            FString ParamDeclaration = GetAngelscriptParameterDeclaration(Property);
            if (!ParamDeclaration.IsEmpty())
            {
                Parameters.Add(ParamDeclaration);
                CallParameters.Add(Property->GetName());
            }
        }
    }

    // Generate the wrapper function
    FString Result;

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
    for (const FString& LocalVar : LocalVariableDeclarations)
    {
        Result += LocalVar + TEXT("\n");
    }

    // Function body
    Result += TEXT("        ");
    if (ReturnType != TEXT("void"))
    {
        Result += TEXT("return ");
    }


    CK_ENSURE_IF_NOT(CallParameters.Num() > 0,
        TEXT("Mixin for function [{}] has NO call parameters, this should not be possible!"),
        Function)
    { return {}; }

    Result += ck::Format_UE(TEXT("{}.{}("), CallParameters[0], FunctionName);

    CallParameters.RemoveAt(0);
    Result += FString::Join(CallParameters, TEXT(", "));

    Result += TEXT(");\n");
    Result += TEXT("    }\n");

    if (IsEditorOnly)
    {
        Result += TEXT("#endif\n");
    }

    Result += TEXT("\n");

    return Result;
}

FString FCkAngelscriptWrapperGenerator::GetDetailedPropertyType(FProperty* Property)
{
    if (!Property)
    { return TEXT("void"); }

    // Handle TArray properties specifically
    if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        FString InnerType = GetDetailedPropertyType(ArrayProp->Inner);
        return ck::Format_UE(TEXT("TArray<{}>"), InnerType);
    }

    // Handle TMap properties
    if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        FString KeyType = GetDetailedPropertyType(MapProp->KeyProp);
        FString ValueType = GetDetailedPropertyType(MapProp->ValueProp);
        return ck::Format_UE(TEXT("TMap<{}, {}>"), KeyType, ValueType);
    }

    // Handle TSet properties
    if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        FString ElementType = GetDetailedPropertyType(SetProp->ElementProp);
        return ck::Format_UE(TEXT("TSet<{}>"), ElementType);
    }

    // Handle enum properties (including TEnumAsByte)
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        return EnumProp->GetEnum()->GetName();
    }

    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (ByteProp->Enum)
        {
            return ByteProp->Enum->GetName();
        }
    }

    // Handle float properties - convert to float32 for Angelscript
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        return TEXT("float32");
    }

    // Handle double properties - convert to float64 for Angelscript
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        return TEXT("float64");
    }

    // Fall back to the CPP type and clean it
    FString CppType = Property->GetCPPType();
    return ConvertToAngelscriptType(CppType);
}

FString FCkAngelscriptWrapperGenerator::GetAngelscriptParameterDeclaration(FProperty* Property)
{
    if (!Property)
        return FString();

    FString Result;

    // Handle const
    if (Property->HasAnyPropertyFlags(CPF_ConstParm))
    {
        Result += TEXT("const ");
    }

    // Get detailed type
    FString AsType = GetDetailedPropertyType(Property);
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
    FString DefaultValue = GetDefaultValueForProperty(Property);
    if (!DefaultValue.IsEmpty())
    {
        Result += TEXT(" = ") + DefaultValue;
    }

    return Result;
}

FString FCkAngelscriptWrapperGenerator::ConvertToAngelscriptType(const FString& UnrealType)
{
    FString Result = UnrealType;

    // Handle TArray<Type> - preserve the full template type
    if (Result.StartsWith(TEXT("TArray<")) && Result.EndsWith(TEXT(">")))
    {
        // Keep TArray<Type> as is, but clean up the inner type
        int32 StartIndex = 7; // Length of "TArray<"
        int32 EndIndex = Result.Len() - 1; // Remove the closing ">"
        FString InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Recursively clean the inner type
        FString CleanInnerType = ConvertToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TArray<{}>"), CleanInnerType);
        return Result;
    }

    // Handle TMap<KeyType, ValueType> - preserve the full template type
    if (Result.StartsWith(TEXT("TMap<")) && Result.EndsWith(TEXT(">")))
    {
        // Keep TMap<KeyType, ValueType> as is, but clean up the inner types
        int32 StartIndex = 5; // Length of "TMap<"
        int32 EndIndex = Result.Len() - 1; // Remove the closing ">"
        FString InnerTypes = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Split by comma to get key and value types
        TArray<FString> TypeParts;
        int32 CommaIndex = InnerTypes.Find(TEXT(","));
        if (CommaIndex != INDEX_NONE)
        {
            FString KeyType = InnerTypes.Left(CommaIndex).TrimStartAndEnd();
            FString ValueType = InnerTypes.Mid(CommaIndex + 1).TrimStartAndEnd();

            // Recursively clean both types
            FString CleanKeyType = ConvertToAngelscriptType(KeyType);
            FString CleanValueType = ConvertToAngelscriptType(ValueType);
            Result = ck::Format_UE(TEXT("TMap<{}, {}>"), CleanKeyType, CleanValueType);
            return Result;
        }
    }

    // Handle TSet<Type> - preserve the full template type
    if (Result.StartsWith(TEXT("TSet<")) && Result.EndsWith(TEXT(">")))
    {
        int32 StartIndex = 5; // Length of "TSet<"
        int32 EndIndex = Result.Len() - 1; // Remove the closing ">"
        FString InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        // Recursively clean the inner type
        FString CleanInnerType = ConvertToAngelscriptType(InnerType);
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

FString FCkAngelscriptWrapperGenerator::ConvertClassNameToNamespace(const FString& ClassName)
{
    FString Result = ClassName;

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
    FString Converted;
    for (int32 I = 0; I < Result.Len(); I++)
    {
        TCHAR Char = Result[I];

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

bool FCkAngelscriptWrapperGenerator::HasInterfaceTypes(UFunction* Function)
{
    if (!Function)
        return false;

    // Check return type
    if (FProperty* ReturnProp = Function->GetReturnProperty())
    {
        if (IsInterfaceProperty(ReturnProp))
            return true;
    }

    // Check all parameters
    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        FProperty* Property = *PropertyIterator;

        // Skip return property (already checked)
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
            continue;

        if (IsInterfaceProperty(Property))
            return true;
    }

    return false;
}

bool FCkAngelscriptWrapperGenerator::IsInterfaceProperty(FProperty* Property)
{
    if (!Property)
        return false;

    // Check for interface property
    if (FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(Property))
    {
        return true;
    }

    // Check for object property that references an interface
    if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        if (ObjectProp->PropertyClass && ObjectProp->PropertyClass->HasAnyClassFlags(CLASS_Interface))
        {
            return true;
        }
    }

    // Check array inner properties
    if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        return IsInterfaceProperty(ArrayProp->Inner);
    }

    // Check map key/value properties
    if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        return IsInterfaceProperty(MapProp->KeyProp) || IsInterfaceProperty(MapProp->ValueProp);
    }

    // Check set element properties
    if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        return IsInterfaceProperty(SetProp->ElementProp);
    }

    return false;
}

auto
    FCkAngelscriptWrapperGenerator::
    IsScriptMixin(
        UFunction* Function,
        const FString& MixinMetadata)
    -> bool
{
    if (!Function)
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
        FProperty* Param = *It;
        if (Param->HasAnyPropertyFlags(CPF_Parm) &&
            NOT Param->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            // Check for struct type
            if (FStructProperty* StructProp = CastField<FStructProperty>(Param))
            {
                const auto& PropertyName = StructProp->Struct->GetFName();
                if (StructProp->Struct && MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            // Check for class type (UObject-derived)
            if (FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(Param))
            {
                const auto& PropertyName = ObjectProp->PropertyClass->GetFName();
                if (ObjectProp->PropertyClass && MixinNames.Contains(PropertyName))
                {
                    return true;
                }
            }

            // FString check
            if (FStrProperty* StrProp = CastField<FStrProperty>(Param))
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

bool FCkAngelscriptWrapperGenerator::IsEditorOnlyFunction(UFunction* Function)
{
    if (!Function)
        return false;

    // Check various metadata that might indicate editor-only functions
    if (Function->HasMetaData(TEXT("EditorOnly")))
        return true;

    if (Function->HasMetaData(TEXT("WithEditor")))
        return true;

    // Check function flags for editor-specific flags
    if (Function->HasAnyFunctionFlags(FUNC_EditorOnly))
        return true;

    // Check if any parameter or return type contains "Editor" in the name
    // This is a heuristic - editor types often have "Editor" in their name
    if (FProperty* ReturnProp = Function->GetReturnProperty())
    {
        FString ReturnType = ReturnProp->GetCPPType();
        if (ReturnType.Contains(TEXT("Editor")))
            return true;
    }

    for (TFieldIterator<FProperty> PropertyIterator(Function); PropertyIterator; ++PropertyIterator)
    {
        FProperty* Property = *PropertyIterator;
        if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
            continue;

        FString PropertyType = Property->GetCPPType();
        if (PropertyType.Contains(TEXT("Editor")))
            return true;
    }

    // Check if the function is in an editor module as a fallback
    return IsClassInEditorModule(Function->GetOwnerClass());
}

FString FCkAngelscriptWrapperGenerator::GetDefaultValueForProperty(FProperty* Property)
{
    if (!Property)
        return FString();

    // Get the function that owns this property
    UStruct* OwnerStruct = Property->GetOwnerStruct();
    UFunction* OwnerFunction = Cast<UFunction>(OwnerStruct);
    if (!OwnerFunction)
        return FString();

    // Check if this property has a default value in the function metadata
    FString MetaKey = ck::Format_UE(TEXT("CPP_Default_{}"), Property->GetName());
    if (OwnerFunction->HasMetaData(*MetaKey))
    {
        FString DefaultValue = OwnerFunction->GetMetaData(*MetaKey);
        return ConvertDefaultValueToAngelscript(DefaultValue, Property);
    }

    return FString();
}

FString FCkAngelscriptWrapperGenerator::ConvertDefaultValueToAngelscript(const FString& CppDefaultValue, FProperty* Property)
{
    if (CppDefaultValue.IsEmpty())
        return FString();

    FString Result = CppDefaultValue;

    // Handle common C++ default value patterns

    // Handle nullptr -> null
    if (Result == TEXT("nullptr") || Result == TEXT("NULL"))
    {
        return TEXT("null");
    }

    // Handle boolean values
    if (Result == TEXT("true") || Result == TEXT("false"))
    {
        return Result; // Same in Angelscript
    }

    // Handle TEXT() macro wrapper for strings
    if (Result.StartsWith(TEXT("TEXT(\"")) && Result.EndsWith(TEXT("\")")))
    {
        // Extract the string content from TEXT("content")
        FString StringContent = Result.Mid(6, Result.Len() - 8); // Remove TEXT(" and ")
        return FString::Printf(TEXT("\"%s\""), *StringContent);
    }

    // Handle string literals that already have quotes
    if (Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\"")))
    {
        return Result; // String literals are the same
    }

    // Handle string properties that don't have quotes but should
    if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        // If it's a string property but doesn't have quotes, add them
        if (!Result.StartsWith(TEXT("\"")) && !Result.EndsWith(TEXT("\"")))
        {
            return FString::Printf(TEXT("\"%s\""), *Result);
        }
    }

    // Handle FName properties
    if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        // FName can be constructed from string literals
        if (!Result.StartsWith(TEXT("\"")) && !Result.EndsWith(TEXT("\"")))
        {
            return FString::Printf(TEXT("\"%s\""), *Result);
        }
    }

    // Handle FText properties
    if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        // FText can be constructed from string literals
        if (!Result.StartsWith(TEXT("\"")) && !Result.EndsWith(TEXT("\"")))
        {
            return FString::Printf(TEXT("\"%s\""), *Result);
        }
    }

    // Handle enum values - need to keep full type name for Angelscript
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        FString EnumTypeName = EnumProp->GetEnum()->GetName();

        // If it's a scoped enum like EMyEnum::Value, convert to EnumType::Value
        if (Result.Contains(TEXT("::")))
        {
            int32 ScopeIndex = Result.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            if (ScopeIndex != INDEX_NONE)
            {
                FString EnumValue = Result.Mid(ScopeIndex + 2);
                return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, EnumValue);
            }
        }
        else
        {
            // If no scope, assume it's just the enum value name
            return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, Result);
        }
    }

    // Handle byte enum values
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (ByteProp->Enum)
        {
            FString EnumTypeName = ByteProp->Enum->GetName();

            // Same enum handling as above
            if (Result.Contains(TEXT("::")))
            {
                int32 ScopeIndex = Result.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
                if (ScopeIndex != INDEX_NONE)
                {
                    FString EnumValue = Result.Mid(ScopeIndex + 2);
                    return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, EnumValue);
                }
            }
            else
            {
                // If no scope, assume it's just the enum value name
                return ck::Format_UE(TEXT("{}::{}"), EnumTypeName, Result);
            }
        }
    }

    // Handle float values - ensure they have 'f' suffix for float32
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        if (!Result.EndsWith(TEXT("f")) && !Result.EndsWith(TEXT("F")))
        {
            Result += TEXT("f");
        }
        return Result;
    }

    // Handle struct default values with named parameters like (R=1.0,G=1.0,B=1.0,A=1.0)
    if (Result.StartsWith(TEXT("(")) && Result.EndsWith(TEXT(")")) && Result.Contains(TEXT("=")))
    {
        // This is Unreal's internal struct representation, convert to constructor call
        FString PropertyTypeName = GetDetailedPropertyType(Property);

        // Extract the values from the named parameter format
        FString InnerValues = Result.Mid(1, Result.Len() - 2); // Remove outer parentheses

        // Convert named parameters to positional parameters
        TArray<FString> NamedParams;
        InnerValues.ParseIntoArray(NamedParams, TEXT(","), true);

        TArray<FString> Values;
        for (const FString& NamedParam : NamedParams)
        {
            FString TrimmedParam = NamedParam.TrimStartAndEnd();
            int32 EqualsIndex = TrimmedParam.Find(TEXT("="));
            if (EqualsIndex != INDEX_NONE)
            {
                FString Value = TrimmedParam.Mid(EqualsIndex + 1).TrimStartAndEnd();
                Values.Add(Value);
            }
        }

        // Create constructor call
        if (Values.Num() > 0)
        {
            return ck::Format_UE(TEXT("{}({})"), PropertyTypeName, FString::Join(Values, TEXT(",")));
        }
    }

    // Handle struct constructors like FVector(1,2,3) or FVector::ZeroVector
    if (Result.StartsWith(TEXT("F")) && Result.Contains(TEXT("(")))
    {
        // Keep as-is - already in constructor format
        return Result;
    }

    // Handle static members like FVector::ZeroVector
    if (Result.Contains(TEXT("::")))
    {
        return Result; // Keep as-is
    }

    // Handle numeric values (int, float, etc.)
    if (Result.IsNumeric() || (Result.StartsWith(TEXT("-")) && Result.Mid(1).IsNumeric()))
    {
        return Result;
    }

    // Handle simple comma-separated values like "0.000000,1.000000,0.000000"
    if (Result.Contains(TEXT(",")) && !Result.Contains(TEXT("(")) && !Result.Contains(TEXT("=")))
    {
        // This might be a struct with comma-separated values, wrap in constructor
        FString PropertyTypeName = GetDetailedPropertyType(Property);
        return ck::Format_UE(TEXT("{}({})"), PropertyTypeName, Result);
    }

    // Default: return as-is
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

#include "AngelscriptBinds.h"

#if WITH_EDITORONLY_DATA
AS_FORCE_LINK const FAngelscriptBinds::FBind GenerateAllFiles(FAngelscriptBinds::EOrder::Early, []
{
    FCkAngelscriptWrapperGenerator::GenerateAllWrappers();
});

#endif