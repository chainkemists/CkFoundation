#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class CKANGELSCRIPTGENERATOR_API FCkAngelscriptWrapperGenerator
{
public:
    static void GenerateAllWrappers();

private:
    static void GenerateWrapperForClass(UClass* Class);
    static FString GenerateWrapperFunction(UFunction* Function, const FString& ClassName, bool IsEditorOnly);
    static FString GenerateWrapperFunctionForMixin(UFunction* Function, const FString& ClassName, bool IsEditorOnly);
    static FString ConvertToAngelscriptType(const FString& UnrealType);
    static FString ConvertClassNameToNamespace(const FString& ClassName);
    static FString GetAngelscriptParameterDeclaration(FProperty* Property);
    static FString GetDetailedPropertyType(FProperty* Property);
    static bool ShouldGenerateWrapperForClass(UClass* Class);
    static bool IsClassInCkFoundationPlugin(UClass* Class);
    static FString GetPluginNameForClass(UClass* Class);
    static bool IsClassInEditorModule(UClass* Class);
    static bool HasInterfaceTypes(UFunction* Function);
    static bool IsInterfaceProperty(FProperty* Property);
    static bool IsScriptMixin(UFunction* Function, const FString& MixinMetadata);
    static bool IsEditorOnlyFunction(UFunction* Function);
    static FString GetDefaultValueForProperty(FProperty* Property);
    static FString ConvertDefaultValueToAngelscript(const FString& CppDefaultValue, FProperty* Property);
};

// --------------------------------------------------------------------------------------------------------------------