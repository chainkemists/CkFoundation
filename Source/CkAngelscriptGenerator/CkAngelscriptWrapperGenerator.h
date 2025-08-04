#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// --------------------------------------------------------------------------------------------------------------------

class CKANGELSCRIPTGENERATOR_API FCkAngelscriptWrapperGenerator
{
public:
    static auto
        GenerateAllWrappers()
        -> void;

private:
    static auto
        Request_GenerateWrapperForClass(
            UClass* Class)
        -> void;

    static auto
        Get_GeneratedWrapperFunction(
            UFunction* Function,
            const FString& ClassName,
            bool IsEditorOnly)
        -> FString;

    static auto
        Get_GeneratedWrapperFunctionForMixin(
            UFunction* Function,
            const FString& ClassName,
            bool IsEditorOnly)
        -> FString;

    static auto
        Get_ConvertedToAngelscriptType(
            const FString& UnrealType)
        -> FString;

    static auto
        Get_ConvertedClassNameToNamespace(
            const FString& ClassName)
        -> FString;

    static auto
        Get_AngelscriptParameterDeclaration(
            FProperty* Property)
        -> FString;

    static auto
        Get_DetailedPropertyType(
            FProperty* Property)
        -> FString;

    static auto
        Request_ShouldGenerateWrapperForClass(
            UClass* Class)
        -> bool;

    static auto
        Request_IsClassInCkFoundationPlugin(
            UClass* Class)
        -> bool;

    static auto
        Get_PluginNameForClass(
            UClass* Class)
        -> FString;

    static auto
        Request_IsClassInEditorModule(
            UClass* Class)
        -> bool;

    static auto
        Has_InterfaceTypes(
            UFunction* Function)
        -> bool;

    static auto
        Request_IsInterfaceProperty(
            FProperty* Property)
        -> bool;

    static auto
        Request_IsScriptMixin(
            UFunction* Function,
            const FString& MixinMetadata)
        -> bool;

    static auto
        Request_IsEditorOnlyFunction(
            UFunction* Function)
        -> bool;

    static auto
        Get_DefaultValueForProperty(
            FProperty* Property)
        -> FString;

    static auto
        Get_ConvertedDefaultValueToAngelscript(
            const FString& CppDefaultValue,
            FProperty* Property)
        -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
