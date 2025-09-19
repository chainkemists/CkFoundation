namespace utils_i_o
{
    UPaperTileMap LoadAssetByName_TileMap(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UPaperTileMap>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UMaterialInterface LoadAssetByName_Material(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UMaterialInterface>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    USkeletalMesh LoadAssetByName_SkeletalMesh(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<USkeletalMesh>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UStaticMesh LoadAssetByName_StaticMesh(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UStaticMesh>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    ULevel LoadAssetByName_Level(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<ULevel>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UNiagaraSystem LoadAssetByName_NiagaraSystem(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UNiagaraSystem>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UNiagaraDataChannelAsset LoadAssetByName_NiagaraDataChannelAsset(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UNiagaraDataChannelAsset>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    USoundCue LoadAssetByName_SoundCue(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<USoundCue>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    USoundAttenuation LoadAssetByName_SoundAttenuation(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<USoundAttenuation>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    USoundConcurrency LoadAssetByName_SoundConcurrency(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<USoundConcurrency>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    USoundClass LoadAssetByName_SoundClass(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<USoundClass>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UGeometryCollection LoadAssetByName_GeometryCollection(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UGeometryCollection>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UAnimInstance LoadAssetByName_AnimInstance(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UAnimInstance>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UAnimMontage LoadAssetByName_AnimMontage(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UAnimMontage>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }

    UEnvQuery LoadAssetByName_EnvQuery(FString InAssetName, ECk_AssetSearchScope InSearchScope = ECk_AssetSearchScope::Game)
    { return Cast<UEnvQuery>(utils_i_o::LoadAssetByName(InAssetName, InSearchScope).Get_Asset().Get()); }
}