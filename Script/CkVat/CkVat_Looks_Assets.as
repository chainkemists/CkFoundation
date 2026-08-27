// CkVat look definitions - decode shaders in Source/CkVat/Shaders/CkVat/Vat.ush.
//
// Param ORDER is load-bearing twice over: the validator matches it positionally against the HLSL
// entry points, and the 12 per-instance scalars map to custom-data slots in DECLARATION ORDER
// (the CkVat runtime writes floats [0..11] in exactly this order - contract:
// Source/CkVat/Plan/Gate_02_Material.md). Do not reorder.
namespace CkVat
{
    asset VatVertex of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkVat/Vat.ush";
        _UshFunctionName = n"CkUsf_Look_VatVertex";
        _WpoFunctionName = n"CkUsf_Look_VatVertex_WPO";
        // The PIXEL entry reads In.UV1.x (the lookup column) to decode the tangent-space normal texture.
        _PixelDataChannels = true;
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _LookName        = n"VatVertex";

        // ---- uniforms (per-collection, set on the shared MID) ----
        FCk_Usf_ParamDesc BaseColorTex;
        BaseColorTex._Name = n"BaseColorTex";
        BaseColorTex._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(BaseColorTex);

        FCk_Usf_ParamDesc PosVat;
        PosVat._Name = n"PosVat";
        PosVat._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(PosVat);

        // Tangent-space normal atlas (uniform, not per-instance - inserting it here does NOT shift
        // the per-instance slots below: slots accrue over _PerInstance params only).
        FCk_Usf_ParamDesc NrmVat;
        NrmVat._Name = n"NrmVat";
        NrmVat._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(NrmVat);

        FCk_Usf_ParamDesc SampleFrequency;
        SampleFrequency._Name = n"SampleFrequency";
        SampleFrequency._Type = ECk_Usf_ParamType::Scalar;
        SampleFrequency._DefaultScalar = 30.0;
        _Parameters.Add(SampleFrequency);

        FCk_Usf_ParamDesc TotalRows;
        TotalRows._Name = n"TotalRows";
        TotalRows._Type = ECk_Usf_ParamType::Scalar;
        TotalRows._DefaultScalar = 1.0;
        _Parameters.Add(TotalRows);

        FCk_Usf_ParamDesc TexelCount;
        TexelCount._Name = n"TexelCount";
        TexelCount._Type = ECk_Usf_ParamType::Scalar;
        TexelCount._DefaultScalar = 1.0;
        _Parameters.Add(TexelCount);

        FCk_Usf_ParamDesc BoundsMin;
        BoundsMin._Name = n"BoundsMin";
        BoundsMin._Type = ECk_Usf_ParamType::Vector;
        BoundsMin._DefaultVector = FLinearColor(0.0, 0.0, 0.0, 1.0);
        _Parameters.Add(BoundsMin);

        FCk_Usf_ParamDesc BoundsMax;
        BoundsMax._Name = n"BoundsMax";
        BoundsMax._Type = ECk_Usf_ParamType::Vector;
        BoundsMax._DefaultVector = FLinearColor(0.0, 0.0, 0.0, 1.0);
        _Parameters.Add(BoundsMax);

        FCk_Usf_ParamDesc DecodeNormalized;
        DecodeNormalized._Name = n"DecodeNormalized";
        DecodeNormalized._Type = ECk_Usf_ParamType::Scalar;
        DecodeNormalized._DefaultScalar = 0.0;
        _Parameters.Add(DecodeNormalized);

        // ---- per-instance playback state (slots 0..11, written by the CkVat runtime) ----
        FCk_Usf_ParamDesc RowStartA;
        RowStartA._Name = n"RowStartA";
        RowStartA._Type = ECk_Usf_ParamType::Scalar;
        RowStartA._PerInstance = true;
        _Parameters.Add(RowStartA);

        FCk_Usf_ParamDesc RowCountA;
        RowCountA._Name = n"RowCountA";
        RowCountA._Type = ECk_Usf_ParamType::Scalar;
        RowCountA._PerInstance = true;
        _Parameters.Add(RowCountA);

        FCk_Usf_ParamDesc TimeA;
        TimeA._Name = n"TimeA";
        TimeA._Type = ECk_Usf_ParamType::Scalar;
        TimeA._PerInstance = true;
        _Parameters.Add(TimeA);

        FCk_Usf_ParamDesc RateA;
        RateA._Name = n"RateA";
        RateA._Type = ECk_Usf_ParamType::Scalar;
        RateA._PerInstance = true;
        _Parameters.Add(RateA);

        FCk_Usf_ParamDesc RowStartB;
        RowStartB._Name = n"RowStartB";
        RowStartB._Type = ECk_Usf_ParamType::Scalar;
        RowStartB._PerInstance = true;
        _Parameters.Add(RowStartB);

        FCk_Usf_ParamDesc RowCountB;
        RowCountB._Name = n"RowCountB";
        RowCountB._Type = ECk_Usf_ParamType::Scalar;
        RowCountB._PerInstance = true;
        _Parameters.Add(RowCountB);

        FCk_Usf_ParamDesc TimeB;
        TimeB._Name = n"TimeB";
        TimeB._Type = ECk_Usf_ParamType::Scalar;
        TimeB._PerInstance = true;
        _Parameters.Add(TimeB);

        FCk_Usf_ParamDesc RateB;
        RateB._Name = n"RateB";
        RateB._Type = ECk_Usf_ParamType::Scalar;
        RateB._PerInstance = true;
        _Parameters.Add(RateB);

        FCk_Usf_ParamDesc TransStart;
        TransStart._Name = n"TransStart";
        TransStart._Type = ECk_Usf_ParamType::Scalar;
        TransStart._PerInstance = true;
        _Parameters.Add(TransStart);

        FCk_Usf_ParamDesc TransDuration;
        TransDuration._Name = n"TransDuration";
        TransDuration._Type = ECk_Usf_ParamType::Scalar;
        TransDuration._PerInstance = true;
        _Parameters.Add(TransDuration);

        FCk_Usf_ParamDesc LoopA;
        LoopA._Name = n"LoopA";
        LoopA._Type = ECk_Usf_ParamType::Scalar;
        LoopA._PerInstance = true;
        _Parameters.Add(LoopA);

        FCk_Usf_ParamDesc LoopB;
        LoopB._Name = n"LoopB";
        LoopB._Type = ECk_Usf_ParamType::Scalar;
        LoopB._PerInstance = true;
        _Parameters.Add(LoopB);
    }

    asset VatBone of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkVat/Vat.ush";
        _UshFunctionName = n"CkUsf_Look_VatBone";
        _WpoFunctionName = n"CkUsf_Look_VatBone_WPO";
        _Domain          = ECk_Usf_Domain::SurfaceLit;
        _LookName        = n"VatBone";

        // ---- uniforms ----
        FCk_Usf_ParamDesc BaseColorTex;
        BaseColorTex._Name = n"BaseColorTex";
        BaseColorTex._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(BaseColorTex);

        FCk_Usf_ParamDesc BonePosVat;
        BonePosVat._Name = n"BonePosVat";
        BonePosVat._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(BonePosVat);

        FCk_Usf_ParamDesc BoneRotVat;
        BoneRotVat._Name = n"BoneRotVat";
        BoneRotVat._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(BoneRotVat);

        // WeightTexture storage carriers (unbound for MeshChannels collections - the WeightStorage
        // uniform branch never samples them there).
        FCk_Usf_ParamDesc BoneIdxTex;
        BoneIdxTex._Name = n"BoneIdxTex";
        BoneIdxTex._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(BoneIdxTex);

        FCk_Usf_ParamDesc BoneWeightTex;
        BoneWeightTex._Name = n"BoneWeightTex";
        BoneWeightTex._Type = ECk_Usf_ParamType::Texture2D;
        _Parameters.Add(BoneWeightTex);

        FCk_Usf_ParamDesc SampleFrequency;
        SampleFrequency._Name = n"SampleFrequency";
        SampleFrequency._Type = ECk_Usf_ParamType::Scalar;
        SampleFrequency._DefaultScalar = 30.0;
        _Parameters.Add(SampleFrequency);

        FCk_Usf_ParamDesc TotalRows;
        TotalRows._Name = n"TotalRows";
        TotalRows._Type = ECk_Usf_ParamType::Scalar;
        TotalRows._DefaultScalar = 1.0;
        _Parameters.Add(TotalRows);

        FCk_Usf_ParamDesc BoneCount;
        BoneCount._Name = n"BoneCount";
        BoneCount._Type = ECk_Usf_ParamType::Scalar;
        BoneCount._DefaultScalar = 1.0;
        _Parameters.Add(BoneCount);

        FCk_Usf_ParamDesc BoundsMin;
        BoundsMin._Name = n"BoundsMin";
        BoundsMin._Type = ECk_Usf_ParamType::Vector;
        BoundsMin._DefaultVector = FLinearColor(0.0, 0.0, 0.0, 1.0);
        _Parameters.Add(BoundsMin);

        FCk_Usf_ParamDesc BoundsMax;
        BoundsMax._Name = n"BoundsMax";
        BoundsMax._Type = ECk_Usf_ParamType::Vector;
        BoundsMax._DefaultVector = FLinearColor(0.0, 0.0, 0.0, 1.0);
        _Parameters.Add(BoundsMax);

        FCk_Usf_ParamDesc DecodeNormalized;
        DecodeNormalized._Name = n"DecodeNormalized";
        DecodeNormalized._Type = ECk_Usf_ParamType::Scalar;
        DecodeNormalized._DefaultScalar = 0.0;
        _Parameters.Add(DecodeNormalized);

        // 0 = mesh channels (UV1/UV2 indices + vertex-color weights), 1 = weight textures.
        FCk_Usf_ParamDesc WeightStorage;
        WeightStorage._Name = n"WeightStorage";
        WeightStorage._Type = ECk_Usf_ParamType::Scalar;
        WeightStorage._DefaultScalar = 0.0;
        _Parameters.Add(WeightStorage);

        // ---- per-instance playback state (slots 0..11) ----
        FCk_Usf_ParamDesc RowStartA;
        RowStartA._Name = n"RowStartA";
        RowStartA._Type = ECk_Usf_ParamType::Scalar;
        RowStartA._PerInstance = true;
        _Parameters.Add(RowStartA);

        FCk_Usf_ParamDesc RowCountA;
        RowCountA._Name = n"RowCountA";
        RowCountA._Type = ECk_Usf_ParamType::Scalar;
        RowCountA._PerInstance = true;
        _Parameters.Add(RowCountA);

        FCk_Usf_ParamDesc TimeA;
        TimeA._Name = n"TimeA";
        TimeA._Type = ECk_Usf_ParamType::Scalar;
        TimeA._PerInstance = true;
        _Parameters.Add(TimeA);

        FCk_Usf_ParamDesc RateA;
        RateA._Name = n"RateA";
        RateA._Type = ECk_Usf_ParamType::Scalar;
        RateA._PerInstance = true;
        _Parameters.Add(RateA);

        FCk_Usf_ParamDesc RowStartB;
        RowStartB._Name = n"RowStartB";
        RowStartB._Type = ECk_Usf_ParamType::Scalar;
        RowStartB._PerInstance = true;
        _Parameters.Add(RowStartB);

        FCk_Usf_ParamDesc RowCountB;
        RowCountB._Name = n"RowCountB";
        RowCountB._Type = ECk_Usf_ParamType::Scalar;
        RowCountB._PerInstance = true;
        _Parameters.Add(RowCountB);

        FCk_Usf_ParamDesc TimeB;
        TimeB._Name = n"TimeB";
        TimeB._Type = ECk_Usf_ParamType::Scalar;
        TimeB._PerInstance = true;
        _Parameters.Add(TimeB);

        FCk_Usf_ParamDesc RateB;
        RateB._Name = n"RateB";
        RateB._Type = ECk_Usf_ParamType::Scalar;
        RateB._PerInstance = true;
        _Parameters.Add(RateB);

        FCk_Usf_ParamDesc TransStart;
        TransStart._Name = n"TransStart";
        TransStart._Type = ECk_Usf_ParamType::Scalar;
        TransStart._PerInstance = true;
        _Parameters.Add(TransStart);

        FCk_Usf_ParamDesc TransDuration;
        TransDuration._Name = n"TransDuration";
        TransDuration._Type = ECk_Usf_ParamType::Scalar;
        TransDuration._PerInstance = true;
        _Parameters.Add(TransDuration);

        FCk_Usf_ParamDesc LoopA;
        LoopA._Name = n"LoopA";
        LoopA._Type = ECk_Usf_ParamType::Scalar;
        LoopA._PerInstance = true;
        _Parameters.Add(LoopA);

        FCk_Usf_ParamDesc LoopB;
        LoopB._Name = n"LoopB";
        LoopB._Type = ECk_Usf_ParamType::Scalar;
        LoopB._PerInstance = true;
        _Parameters.Add(LoopB);
    }
}
