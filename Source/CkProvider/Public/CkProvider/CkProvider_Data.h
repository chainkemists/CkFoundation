#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include <GameplayTagContainer.h>

#include "CkProvider_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Base Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotEditInlineNew, NotBlueprintable, NotBlueprintType)
class CKPROVIDER_API UCk_Provider_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_PDA);
};

// --------------------------------------------------------------------------------------------------------------------
// Bool Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Bool_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Bool_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Bool")
    bool Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Bool_Literal_PDA : public UCk_Provider_Bool_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Bool_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> bool override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    bool _Value = false;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Bool
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Bool);

public:
    FCk_Provider_Bool() = default;
    explicit FCk_Provider_Bool(
        UCk_Provider_Bool_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Bool_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------
// Float Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Float_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Float_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Float")
    float Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Float_Literal_PDA : public UCk_Provider_Float_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Float_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> float override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    float _Value = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Float
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Float);

public:
    FCk_Provider_Float() = default;
    explicit FCk_Provider_Float(
        UCk_Provider_Float_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Float_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// Int Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Int_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Int_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Int")
    int32 Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Int_Literal_PDA : public UCk_Provider_Int_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Int_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> int32 override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    int32 _Value = 0;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Int
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Int);

public:
    FCk_Provider_Int() = default;
    explicit FCk_Provider_Int(
        UCk_Provider_Int_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Int_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// Vector3 Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Vector3_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Vector3_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Vector3")
    FVector Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Vector3_Literal_PDA : public UCk_Provider_Vector3_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Vector3_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FVector override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FVector _Value = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Vector3
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Vector3);

public:
    FCk_Provider_Vector3() = default;
    explicit FCk_Provider_Vector3(
        UCk_Provider_Vector3_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Vector3_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// Vector2 Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Vector2_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Vector2_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Vector2")
    FVector2D Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Vector2_Literal_PDA : public UCk_Provider_Vector2_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Vector2_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FVector2D override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FVector2D _Value = FVector2D::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Vector2
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Vector2);

public:
    FCk_Provider_Vector2() = default;
    explicit FCk_Provider_Vector2(
        UCk_Provider_Vector2_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Vector2_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// Rotator Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Rotator_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Rotator_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Rotator")
    FRotator Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Rotator_Literal_PDA : public UCk_Provider_Rotator_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Rotator_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FRotator override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FRotator _Value = FRotator::ZeroRotator;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Rotator
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Rotator);

public:
    FCk_Provider_Rotator() = default;
    explicit FCk_Provider_Rotator(
        UCk_Provider_Rotator_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Rotator_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// Rotator Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_Transform_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Transform_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|Transform")
    FTransform Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_Transform_Literal_PDA : public UCk_Provider_Transform_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_Transform_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FTransform override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FTransform _Value = FTransform::Identity;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_Transform
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_Transform);

public:
    FCk_Provider_Transform() = default;
    explicit FCk_Provider_Transform(
        UCk_Provider_Transform_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_Transform_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// GameplayTag Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_GameplayTag_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_GameplayTag_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|GameplayTag")
    FGameplayTag Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_GameplayTag_Literal_PDA : public UCk_Provider_GameplayTag_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_GameplayTag_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FGameplayTag override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FGameplayTag _Value = FGameplayTag::EmptyTag;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_GameplayTag
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_GameplayTag);

public:
    FCk_Provider_GameplayTag() = default;
    explicit FCk_Provider_GameplayTag(
        UCk_Provider_GameplayTag_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_GameplayTag_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------
// GameplayTagContainer Provider
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKPROVIDER_API UCk_Provider_GameplayTagContainer_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_GameplayTagContainer_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|GameplayTagContainer")
    FGameplayTagContainer Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROVIDER_API UCk_Provider_GameplayTagContainer_Literal_PDA : public UCk_Provider_GameplayTagContainer_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_GameplayTagContainer_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FGameplayTagContainer override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROVIDER_API FCk_Provider_GameplayTagContainer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_GameplayTagContainer);

public:
    FCk_Provider_GameplayTagContainer() = default;
    explicit FCk_Provider_GameplayTagContainer(
        UCk_Provider_GameplayTagContainer_PDA* InProvider);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_GameplayTagContainer_PDA>  _Provider;

public:
    CK_PROPERTY_GET(_Provider)
};

// --------------------------------------------------------------------------------------------------------------------