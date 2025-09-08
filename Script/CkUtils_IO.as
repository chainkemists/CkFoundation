namespace utils_i_o
{
    UPaperTileMap LoadAssetByName_TileMap(FString InAssetName) { return Cast<UPaperTileMap>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UMaterialInterface LoadAssetByName_Material(FString InAssetName) { return Cast<UMaterialInterface>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    USkeletalMesh LoadAssetByName_SkeletalMesh(FString InAssetName) { return Cast<USkeletalMesh>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UStaticMesh LoadAssetByName_StaticMesh(FString InAssetName) { return Cast<UStaticMesh>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    ULevel LoadAssetByName_Level(FString InAssetName) { return Cast<ULevel>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UNiagaraSystem LoadAssetByName_NiagaraSystem(FString InAssetName) { return Cast<UNiagaraSystem>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UNiagaraDataChannelAsset LoadAssetByName_NiagaraDataChannelAsset(FString InAssetName) { return Cast<UNiagaraDataChannelAsset>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    USoundCue LoadAssetByName_SoundCue(FString InAssetName) { return Cast<USoundCue>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    USoundAttenuation LoadAssetByName_SoundAttenuation(FString InAssetName) { return Cast<USoundAttenuation>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    USoundConcurrency LoadAssetByName_SoundConcurrency(FString InAssetName) { return Cast<USoundConcurrency>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    USoundClass LoadAssetByName_SoundClass(FString InAssetName) { return Cast<USoundClass>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UGeometryCollection LoadAssetByName_GeometryCollection(FString InAssetName) { return Cast<UGeometryCollection>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UAnimInstance LoadAssetByName_AnimInstance(FString InAssetName) { return Cast<UAnimInstance>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
    UAnimMontage LoadAssetByName_AnimMontage(FString InAssetName) { return Cast<UAnimMontage>(utils_i_o::LoadAssetByName(InAssetName).Get_Asset().Get()); }
}