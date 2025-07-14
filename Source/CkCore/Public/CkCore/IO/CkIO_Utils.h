#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include <Engine/Font.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <Internationalization/Internationalization.h>

#include "CkIO_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Engine_TextFontSize : uint8
{
    Subtitle,
    Tiny,
    Small,
    Medium,
    Large
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Engine_TextFontSize);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AssetLocalRootType : uint8
{
    Project,
    Engine,
    ProjectPlugin,
    EnginePlugin,
    Invalid
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AssetLocalRootType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class ECk_AssetSearchScope : uint8
{
    None    = 0 UMETA(Hidden),
    Game    = 1 << 0,
    Plugins = 1 << 1,
    Engine  = 1 << 2,
    All     = Game | Plugins | Engine
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AssetSearchScope);
ENABLE_ENUM_BITWISE_OPERATORS(ECk_AssetSearchScope);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AssetSearchStrategy : uint8
{
    ExactOnly,
    FuzzyOnly,
    ExactThenFuzzy,
    Both
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AssetSearchStrategy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AssetMatchType : uint8
{
    ExactMatch,
    FuzzyMatch
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AssetMatchType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Utils_IO_AssetInfoFromPath_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_IO_AssetInfoFromPath_Result);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool        _AssetFound = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FAssetData  _AssetData;

public:
    CK_PROPERTY(_AssetFound);
    CK_PROPERTY(_AssetData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Object_AssetSearchResult_Single
{
    GENERATED_BODY()

    friend class UCk_Utils_IO_UE;

public:
    CK_GENERATED_BODY(FCk_Utils_Object_AssetSearchResult_Single);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Asset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FString _AssetName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FString _AssetPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Unique _UniqueAsset = ECk_Unique::DoesNotExist;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_AssetMatchType _MatchType = ECk_AssetMatchType::ExactMatch;

public:
    CK_PROPERTY_GET(_Asset);
    CK_PROPERTY_GET(_AssetName);
    CK_PROPERTY_GET(_AssetPath);
    CK_PROPERTY_GET(_UniqueAsset);
    CK_PROPERTY_GET(_MatchType);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Object_AssetSearchResult_Array
{
    GENERATED_BODY()

    friend class UCk_Utils_IO_UE;

public:
    CK_GENERATED_BODY(FCk_Utils_Object_AssetSearchResult_Array);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FCk_Utils_Object_AssetSearchResult_Single> _Results;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _ExactMatchCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _FuzzyMatchCount = 0;

public:
    CK_PROPERTY_GET(_Results);
    CK_PROPERTY_GET(_ExactMatchCount);
    CK_PROPERTY_GET(_FuzzyMatchCount);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_Utils_IO_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IO_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Engine Default Text Font")
    static UFont*
    Get_Engine_DefaultTextFont(
        ECk_Engine_TextFontSize InFontSize);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Version")
    static FString
    Get_ProjectVersion();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Directory")
    static FString
    Get_ProjectDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Plugins Directory")
    static FString
    Get_PluginsDir(
        const FString& InPluginName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Content Directory")
    static FString
    Get_ProjectContentDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Plugins Directory")
    static FString
    Get_ProjectPluginsDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Engine Plugins Directory")
    static FString
    Get_EnginePluginsDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Asset Info From Path")
    static FCk_Utils_IO_AssetInfoFromPath_Result
    Get_AssetInfoFromPath(
        const FString& InAssetPath);

    UFUNCTION(BlueprintPure)
    static ECk_AssetLocalRootType
    Get_AssetLocalRoot(
        const FString& InAssetPath);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Extract Path")
    static FString
    Get_ExtractPath(
        const FString& InFullPath);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Soft Object Asset Name")
    static FString
    Get_SoftObjectAssetName(
        const TSoftObjectPtr<UObject>& InSoftObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Soft Object Asset Path")
    static FString
    Get_SoftObjectAssetPath(
        const TSoftObjectPtr<UObject>& InSoftObject);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Load Assets By Name",
              Category = "Ck|Utils|Object|Assets")
    static FCk_Utils_Object_AssetSearchResult_Array
    LoadAssetsByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game,
        ECk_AssetSearchStrategy SearchStrategy = ECk_AssetSearchStrategy::ExactThenFuzzy);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Load Assets By Name (With Class Filter)",
              Category = "Ck|Utils|Object|Assets")
    static FCk_Utils_Object_AssetSearchResult_Array
    LoadAssetsByName_FilterByClass(
        const FString& AssetName,
        TSubclassOf<UObject> AssetClass,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game,
        ECk_AssetSearchStrategy SearchStrategy = ECk_AssetSearchStrategy::ExactThenFuzzy);

    // UPDATED SINGLE FUNCTIONS - Now return struct and call array functions
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Load Asset By Name",
              Category = "Ck|Utils|Object|Assets")
    static FCk_Utils_Object_AssetSearchResult_Single
    LoadAssetByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game,
        ECk_AssetSearchStrategy SearchStrategy = ECk_AssetSearchStrategy::ExactThenFuzzy);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Load Asset By Name (With Class Filter)",
              Category = "Ck|Utils|Object|Assets")
    static FCk_Utils_Object_AssetSearchResult_Single
    LoadAssetByName_FilterByClass(
        const FString& AssetName,
        TSubclassOf<UObject> AssetClass,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game,
        ECk_AssetSearchStrategy SearchStrategy = ECk_AssetSearchStrategy::ExactThenFuzzy);

    // Find all assets matching a name without loading them
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Find Assets By Name",
              Category = "Ck|Utils|Object|Assets")
    static TArray<FString>
    FindAssetsByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game);

    // Template versions for C++ usage
    template<typename T>
    static FCk_Utils_Object_AssetSearchResult_Single
    LoadAssetByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game,
        ECk_AssetSearchStrategy SearchStrategy = ECk_AssetSearchStrategy::ExactThenFuzzy);

    template<typename T>
    static FCk_Utils_Object_AssetSearchResult_Array
    LoadAssetsByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope = ECk_AssetSearchScope::Game,
        ECk_AssetSearchStrategy SearchStrategy = ECk_AssetSearchStrategy::ExactThenFuzzy);

private:
    static FCk_Utils_Object_AssetSearchResult_Array
    DoLoadAssetsByName(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy);

    static FCk_Utils_Object_AssetSearchResult_Array
    DoFastExactLookup(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope);

    // Fuzzy search for ExactThenFuzzy fallback
    static FCk_Utils_Object_AssetSearchResult_Array
    DoFuzzySearch(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope);

    // Original full-scan approach for FuzzyOnly and Both strategies
    static FCk_Utils_Object_AssetSearchResult_Array
    DoFullAssetScan(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy);

    static auto
    Get_SearchPaths(
        ECk_AssetSearchScope SearchScope) -> TArray<FName>;

public:
    template <typename T_Char>
    constexpr T_Char
    static Get_FileName(T_Char InPath)
    {
        T_Char File = InPath;
        while (*InPath)
        {
            if (*InPath++ == '/')
            {
                File = InPath;
            }
        }
        return File;
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<typename T>
auto
    UCk_Utils_IO_UE::
    LoadAssetByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Single
{
    static_assert(std::is_base_of_v<UObject, T>, "T must be derived from UObject");

    return LoadAssetByName_FilterByClass(AssetName, T::StaticClass(), SearchScope, SearchStrategy);
}

template<typename T>
auto
    UCk_Utils_IO_UE::
    LoadAssetsByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    static_assert(std::is_base_of_v<UObject, T>, "T must be derived from UObject");

    return LoadAssetsByName_FilterByClass(AssetName, T::StaticClass(), SearchScope, SearchStrategy);
}

// --------------------------------------------------------------------------------------------------------------------

// char to wchar_t trick taken from: https://stackoverflow.com/a/14421702
#define CK_WIDE2(x) L##x
#define CK_WIDE1(x) CK_WIDE2(x)
#define WFILE CK_WIDE1(__FILE__)

#define CK_UTILS_IO_GET_FILENAME() UCk_Utils_IO_UE::Get_FileName(WFILE)

#define CK_UTILS_IO_GET_LOCTEXT(InKey, InText)\
    FText::AsLocalizable_Advanced(CK_UTILS_IO_GET_FILENAME(), InKey, InText)

// --------------------------------------------------------------------------------------------------------------------
