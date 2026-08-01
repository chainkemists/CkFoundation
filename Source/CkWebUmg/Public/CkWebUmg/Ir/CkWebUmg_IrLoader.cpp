#include "CkWebUmg_IrLoader.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg_Log.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_webumg_irloader
{
    constexpr int32 SupportedSchemaVersion = 1;

    auto
    ReadRect(
        const TSharedPtr<FJsonObject>& InObj)
        -> FCkWebUmg_IrRect
    {
        if (InObj == nullptr)
        { return {}; }

        return FCkWebUmg_IrRect{
            static_cast<float>(InObj->GetNumberField(TEXT("x"))),
            static_cast<float>(InObj->GetNumberField(TEXT("y"))),
            static_cast<float>(InObj->GetNumberField(TEXT("w"))),
            static_cast<float>(InObj->GetNumberField(TEXT("h")))};
    }

    auto
    ReadColor(
        const TSharedPtr<FJsonObject>& InParent,
        const FString& InField)
        -> TOptional<FColor>
    {
        const TArray<TSharedPtr<FJsonValue>>* Rgba = nullptr;
        if (NOT InParent->TryGetArrayField(InField, Rgba) || Rgba->Num() != 4)
        { return {}; }

        return FColor(
            static_cast<uint8>((*Rgba)[0]->AsNumber()),
            static_cast<uint8>((*Rgba)[1]->AsNumber()),
            static_cast<uint8>((*Rgba)[2]->AsNumber()),
            static_cast<uint8>((*Rgba)[3]->AsNumber()));
    }

    auto
    ReadFloat4(
        const TSharedPtr<FJsonObject>& InParent,
        const FString& InField)
        -> FVector4f
    {
        auto Result = FVector4f::Zero();

        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (NOT InParent->TryGetArrayField(InField, Values) || Values->Num() != 4)
        { return Result; }

        for (auto Index = 0; Index < 4; ++Index)
        { Result[Index] = static_cast<float>((*Values)[Index]->AsNumber()); }

        return Result;
    }

    auto
    ReadNode(
        const TSharedPtr<FJsonObject>& InObj)
        -> TSharedPtr<FCkWebUmg_IrNode>
    {
        const auto NodeIsValid = InObj != nullptr;
        CK_ENSURE_IF_NOT(NodeIsValid, TEXT("ckui node object is null — malformed document"))
        {}
        if (NOT NodeIsValid)
        { return {}; }

        auto Node = MakeShared<FCkWebUmg_IrNode>();
        Node->Id = InObj->GetStringField(TEXT("id"));
        Node->Tag = InObj->GetStringField(TEXT("tag"));
        InObj->TryGetStringField(TEXT("asset"), Node->Asset);

        const auto ReadBox = [](const TSharedPtr<FJsonObject>& InBoxObj) -> FCkWebUmg_IrBox
        {
            return FCkWebUmg_IrBox{
                ReadRect(InBoxObj->GetObjectField(TEXT("content"))),
                ReadRect(InBoxObj->GetObjectField(TEXT("padding"))),
                ReadRect(InBoxObj->GetObjectField(TEXT("border"))),
                ReadRect(InBoxObj->GetObjectField(TEXT("margin")))};
        };
        Node->Box = ReadBox(InObj->GetObjectField(TEXT("box")));

        const TSharedPtr<FJsonObject>* BoxUntransformedObj = nullptr;
        if (InObj->TryGetObjectField(TEXT("boxUntransformed"), BoxUntransformedObj))
        { Node->BoxUntransformed = ReadBox(*BoxUntransformedObj); }

        const auto& LayoutObj = InObj->GetObjectField(TEXT("layout"));
        auto& Layout = Node->Layout;
        Layout.Display = LayoutObj->GetStringField(TEXT("display"));
        Layout.Direction = LayoutObj->GetStringField(TEXT("direction"));
        Layout.Justify = LayoutObj->GetStringField(TEXT("justify"));
        Layout.Align = LayoutObj->GetStringField(TEXT("align"));
        Layout.AlignSelf = LayoutObj->GetStringField(TEXT("alignSelf"));
        Layout.AlignContent = LayoutObj->GetStringField(TEXT("alignContent"));
        Layout.Wrap = LayoutObj->GetStringField(TEXT("wrap"));
        Layout.Position = LayoutObj->GetStringField(TEXT("position"));
        Layout.Basis = LayoutObj->GetStringField(TEXT("basis"));
        Layout.BoxSizing = LayoutObj->GetStringField(TEXT("boxSizing"));
        Layout.Grow = static_cast<float>(LayoutObj->GetNumberField(TEXT("grow")));
        Layout.Shrink = static_cast<float>(LayoutObj->GetNumberField(TEXT("shrink")));
        Layout.ZIndex = static_cast<int32>(LayoutObj->GetNumberField(TEXT("zIndex")));
        Layout.Order = static_cast<int32>(LayoutObj->GetNumberField(TEXT("order")));

        const TArray<TSharedPtr<FJsonValue>>* GapValues = nullptr;
        if (LayoutObj->TryGetArrayField(TEXT("gap"), GapValues) && GapValues->Num() == 2)
        {
            Layout.Gap = FVector2f(
                static_cast<float>((*GapValues)[0]->AsNumber()),
                static_cast<float>((*GapValues)[1]->AsNumber()));
        }

        const TArray<TSharedPtr<FJsonValue>>* OverflowValues = nullptr;
        if (LayoutObj->TryGetArrayField(TEXT("overflow"), OverflowValues) && OverflowValues->Num() == 2)
        {
            Layout.OverflowX = (*OverflowValues)[0]->AsString();
            Layout.OverflowY = (*OverflowValues)[1]->AsString();
        }

        const TArray<TSharedPtr<FJsonValue>>* SizingValues = nullptr;
        if (LayoutObj->TryGetArrayField(TEXT("sizingAuthored"), SizingValues))
        {
            for (const auto& Value : *SizingValues)
            { Layout.SizingAuthored.Add(Value->AsString()); }
        }

        const auto ReadConstraintPair = [&](const TCHAR* InField, TOptional<float> (&OutPair)[2]) -> void
        {
            const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
            if (NOT LayoutObj->TryGetArrayField(InField, Values) || Values->Num() != 2)
            { return; }
            for (auto Index = 0; Index < 2; ++Index)
            {
                if ((*Values)[Index]->Type == EJson::Number)
                { OutPair[Index] = static_cast<float>((*Values)[Index]->AsNumber()); }
            }
        };
        ReadConstraintPair(TEXT("minSize"), Layout.MinSize);
        ReadConstraintPair(TEXT("maxSize"), Layout.MaxSize);

        const TSharedPtr<FJsonObject>* InsetObj = nullptr;
        if (LayoutObj->TryGetObjectField(TEXT("inset"), InsetObj))
        {
            auto Inset = FCkWebUmg_IrInset{};
            Inset.Top = (*InsetObj)->GetStringField(TEXT("top"));
            Inset.Right = (*InsetObj)->GetStringField(TEXT("right"));
            Inset.Bottom = (*InsetObj)->GetStringField(TEXT("bottom"));
            Inset.Left = (*InsetObj)->GetStringField(TEXT("left"));

            const TArray<TSharedPtr<FJsonValue>>* AuthoredValues = nullptr;
            if ((*InsetObj)->TryGetArrayField(TEXT("authored"), AuthoredValues))
            {
                for (const auto& Side : *AuthoredValues)
                { Inset.Authored.Add(Side->AsString()); }
            }
            Layout.Inset = Inset;
        }

        const auto& PaintObj = InObj->GetObjectField(TEXT("paint"));
        auto& Paint = Node->Paint;
        const TSharedPtr<FJsonObject>* BackgroundObj = nullptr;
        if (PaintObj->TryGetObjectField(TEXT("background"), BackgroundObj))
        {
            const auto BackgroundType = (*BackgroundObj)->GetStringField(TEXT("type"));
            if (BackgroundType == TEXT("color"))
            {
                Paint.BackgroundColor = ReadColor(*BackgroundObj, TEXT("rgba"));
            }
            else if (BackgroundType == TEXT("gradient"))
            {
                auto Gradient = FCkWebUmg_IrGradient{};
                Gradient.GradientType = (*BackgroundObj)->GetStringField(TEXT("gradientType"));
                auto Angle = 0.0;
                if ((*BackgroundObj)->TryGetNumberField(TEXT("angleDeg"), Angle))
                { Gradient.AngleDeg = static_cast<float>(Angle); }

                const auto ReadVec2 = [&](const TCHAR* InField) -> TOptional<FVector2f>
                {
                    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
                    if (NOT (*BackgroundObj)->TryGetArrayField(InField, Values) || Values->Num() != 2)
                    { return {}; }
                    return FVector2f(
                        static_cast<float>((*Values)[0]->AsNumber()),
                        static_cast<float>((*Values)[1]->AsNumber()));
                };
                Gradient.RadialCenter = ReadVec2(TEXT("center"));
                Gradient.RadialRadius = ReadVec2(TEXT("radius"));

                const TArray<TSharedPtr<FJsonValue>>* StopValues = nullptr;
                if ((*BackgroundObj)->TryGetArrayField(TEXT("stops"), StopValues))
                {
                    for (const auto& StopValue : *StopValues)
                    {
                        const auto& StopObj = StopValue->AsObject();
                        auto Stop = FCkWebUmg_IrGradientStop{};
                        if (const auto Color = ReadColor(StopObj, TEXT("rgba")); Color.IsSet())
                        { Stop.Color = *Color; }
                        auto Pos = 0.0;
                        if (StopObj->TryGetNumberField(TEXT("posPct"), Pos))
                        { Stop.PosPct = static_cast<float>(Pos); }
                        Gradient.Stops.Add(Stop);
                    }
                }
                Paint.Gradient = Gradient;
            }
            else if (BackgroundType == TEXT("image"))
            {
                auto AssetId = FString{};
                if ((*BackgroundObj)->TryGetStringField(TEXT("asset"), AssetId) && NOT AssetId.IsEmpty())
                { Paint.BackgroundImageAsset = AssetId; }
            }
        }
        Paint.BorderRadius = ReadFloat4(PaintObj, TEXT("borderRadius"));
        Paint.BorderWidth = ReadFloat4(PaintObj, TEXT("borderWidth"));
        Paint.BorderColor = ReadColor(PaintObj, TEXT("borderColor"));

        const TArray<TSharedPtr<FJsonValue>>* BorderColorValues = nullptr;
        if (PaintObj->TryGetArrayField(TEXT("borderColors"), BorderColorValues) && BorderColorValues->Num() == 4)
        {
            for (const auto& ColorValue : *BorderColorValues)
            {
                const auto& Rgba = ColorValue->AsArray();
                if (Rgba.Num() != 4)
                { Paint.BorderColors.Reset(); break; }
                Paint.BorderColors.Add(FColor(
                    static_cast<uint8>(Rgba[0]->AsNumber()), static_cast<uint8>(Rgba[1]->AsNumber()),
                    static_cast<uint8>(Rgba[2]->AsNumber()), static_cast<uint8>(Rgba[3]->AsNumber())));
            }
        }

        const TSharedPtr<FJsonObject>* ShadowObj = nullptr;
        if (PaintObj->TryGetObjectField(TEXT("boxShadow"), ShadowObj))
        {
            const TArray<TSharedPtr<FJsonValue>>* LayerValues = nullptr;
            if ((*ShadowObj)->TryGetArrayField(TEXT("layers"), LayerValues))
            {
                for (const auto& LayerValue : *LayerValues)
                {
                    const auto& LayerObj = LayerValue->AsObject();
                    auto Layer = FCkWebUmg_IrShadowLayer{};
                    if (const auto Color = ReadColor(LayerObj, TEXT("rgba")); Color.IsSet())
                    { Layer.Color = *Color; }
                    const TArray<TSharedPtr<FJsonValue>>* OffsetValues = nullptr;
                    if (LayerObj->TryGetArrayField(TEXT("offset"), OffsetValues) && OffsetValues->Num() == 2)
                    {
                        Layer.Offset = FVector2f(
                            static_cast<float>((*OffsetValues)[0]->AsNumber()),
                            static_cast<float>((*OffsetValues)[1]->AsNumber()));
                    }
                    Layer.Blur = static_cast<float>(LayerObj->GetNumberField(TEXT("blur")));
                    Layer.Spread = static_cast<float>(LayerObj->GetNumberField(TEXT("spread")));
                    LayerObj->TryGetBoolField(TEXT("inset"), Layer.Inset);
                    Paint.ShadowLayers.Add(Layer);
                }
            }
            else
            { Paint.HasUntypedShadow = true; }
        }

        const TSharedPtr<FJsonObject>* TransformObj = nullptr;
        if (PaintObj->TryGetObjectField(TEXT("transform"), TransformObj))
        {
            auto Transform = FCkWebUmg_IrTransform{};
            const TArray<TSharedPtr<FJsonValue>>* MatrixValues = nullptr;
            if ((*TransformObj)->TryGetArrayField(TEXT("matrix"), MatrixValues) && MatrixValues->Num() == 6)
            {
                for (const auto& Value : *MatrixValues)
                { Transform.Matrix.Add(static_cast<float>(Value->AsNumber())); }
            }
            const TArray<TSharedPtr<FJsonValue>>* OriginValues = nullptr;
            if ((*TransformObj)->TryGetArrayField(TEXT("origin"), OriginValues) && OriginValues->Num() == 2)
            {
                Transform.Origin = FVector2f(
                    static_cast<float>((*OriginValues)[0]->AsNumber()),
                    static_cast<float>((*OriginValues)[1]->AsNumber()));
            }
            Paint.Transform = Transform;
        }
        Paint.Opacity = static_cast<float>(PaintObj->GetNumberField(TEXT("opacity")));
        Paint.Visibility = PaintObj->GetStringField(TEXT("visibility"));

        const TSharedPtr<FJsonObject>* TextObj = nullptr;
        if (InObj->TryGetObjectField(TEXT("text"), TextObj))
        {
            auto Text = FCkWebUmg_IrText{};
            Text.Content = (*TextObj)->GetStringField(TEXT("content"));
            Text.Family = (*TextObj)->GetStringField(TEXT("family"));
            Text.SizePx = static_cast<float>((*TextObj)->GetNumberField(TEXT("sizePx")));
            Text.Weight = static_cast<int32>((*TextObj)->GetNumberField(TEXT("weight")));
            Text.LetterSpacingPx = static_cast<float>((*TextObj)->GetNumberField(TEXT("letterSpacingPx")));
            Text.Color = ReadColor(*TextObj, TEXT("color"));
            Text.Align = (*TextObj)->GetStringField(TEXT("align"));
            Text.WhiteSpace = (*TextObj)->GetStringField(TEXT("whiteSpace"));
            (*TextObj)->TryGetStringField(TEXT("transformCase"), Text.TransformCase);

            auto LineHeight = 0.0;
            if ((*TextObj)->TryGetNumberField(TEXT("lineHeightPx"), LineHeight))
            { Text.LineHeightPx = static_cast<float>(LineHeight); }

            const TArray<TSharedPtr<FJsonValue>>* LineBoxValues = nullptr;
            if ((*TextObj)->TryGetArrayField(TEXT("lineBoxes"), LineBoxValues))
            {
                for (const auto& LineBoxValue : *LineBoxValues)
                {
                    const auto& Rect = LineBoxValue->AsArray();
                    if (Rect.Num() != 4)
                    { continue; }
                    Text.LineBoxes.Add(FCkWebUmg_IrRect{
                        static_cast<float>(Rect[0]->AsNumber()), static_cast<float>(Rect[1]->AsNumber()),
                        static_cast<float>(Rect[2]->AsNumber()), static_cast<float>(Rect[3]->AsNumber())});
                }
            }

            Node->Text = Text;
        }

        const TArray<TSharedPtr<FJsonValue>>* ChildValues = nullptr;
        if (InObj->TryGetArrayField(TEXT("children"), ChildValues))
        {
            for (const auto& ChildValue : *ChildValues)
            {
                if (auto Child = ReadNode(ChildValue->AsObject()); Child != nullptr)
                { Node->Children.Add(Child); }
            }
        }

        return Node;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::webumg
{
    auto
    LoadIrDocument(
        const FString& InJsonText)
        -> TOptional<FCkWebUmg_IrDocument>
    {
        auto RootObj = TSharedPtr<FJsonObject>{};
        const auto Reader = TJsonReaderFactory<>::Create(InJsonText);

        const auto ParseSucceeded = FJsonSerializer::Deserialize(Reader, RootObj) && RootObj != nullptr;
        CK_ENSURE_IF_NOT(ParseSucceeded, TEXT("ckui.json failed to parse as JSON"))
        {}
        if (NOT ParseSucceeded)
        { return {}; }

        const auto Schema = static_cast<int32>(RootObj->GetNumberField(TEXT("schema")));
        const auto SchemaIsSupported = Schema == ck_webumg_irloader::SupportedSchemaVersion;
        CK_ENSURE_IF_NOT(SchemaIsSupported,
            TEXT("ckui.json schema [{}] is not the supported version [{}]"),
            Schema, ck_webumg_irloader::SupportedSchemaVersion)
        {}
        if (NOT SchemaIsSupported)
        { return {}; }

        auto Document = FCkWebUmg_IrDocument{};
        Document.Schema = Schema;

        const auto& SourceObj = RootObj->GetObjectField(TEXT("source"));
        Document.Browser = SourceObj->GetStringField(TEXT("browser"));
        Document.Dpr = static_cast<float>(SourceObj->GetNumberField(TEXT("dpr")));

        const TArray<TSharedPtr<FJsonValue>>* ViewportValues = nullptr;
        if (SourceObj->TryGetArrayField(TEXT("viewport"), ViewportValues) && ViewportValues->Num() == 2)
        {
            Document.Viewport = FIntPoint(
                static_cast<int32>((*ViewportValues)[0]->AsNumber()),
                static_cast<int32>((*ViewportValues)[1]->AsNumber()));
        }

        const TArray<TSharedPtr<FJsonValue>>* AssetValues = nullptr;
        if (RootObj->TryGetArrayField(TEXT("assets"), AssetValues))
        {
            for (const auto& AssetValue : *AssetValues)
            {
                const auto& AssetObj = AssetValue->AsObject();
                Document.AssetSourcesById.Add(
                    AssetObj->GetStringField(TEXT("id")), AssetObj->GetStringField(TEXT("src")));
            }
        }

        Document.Root = ck_webumg_irloader::ReadNode(RootObj->GetObjectField(TEXT("root")));

        const auto RootIsValid = Document.Root != nullptr;
        CK_ENSURE_IF_NOT(RootIsValid, TEXT("ckui.json has no readable root node"))
        {}
        if (NOT RootIsValid)
        { return {}; }

        return Document;
    }

    auto
    LoadIrDocumentFromFile(
        const FString& InFilePath)
        -> TOptional<FCkWebUmg_IrDocument>
    {
        auto JsonText = FString{};

        const auto FileWasRead = FFileHelper::LoadFileToString(JsonText, *InFilePath);
        CK_ENSURE_IF_NOT(FileWasRead, TEXT("Could not read ckui.json file [{}]"), InFilePath)
        {}
        if (NOT FileWasRead)
        { return {}; }

        return LoadIrDocument(JsonText);
    }
}

// --------------------------------------------------------------------------------------------------------------------
