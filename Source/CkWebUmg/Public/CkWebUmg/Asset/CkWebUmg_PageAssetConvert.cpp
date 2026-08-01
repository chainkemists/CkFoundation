#include "CkWebUmg_PageAssetConvert.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_webumg_pageassetconvert
{
    auto ToVec4(const FCkWebUmg_IrRect& InRect) -> FVector4f
    { return FVector4f{InRect.X, InRect.Y, InRect.W, InRect.H}; }

    auto ToRect(const FVector4f& InVec) -> FCkWebUmg_IrRect
    { return FCkWebUmg_IrRect{InVec.X, InVec.Y, InVec.Z, InVec.W}; }

    auto
    ToBoxData(
        const FCkWebUmg_IrBox& InBox)
        -> FCk_WebUmg_BoxData
    {
        return FCk_WebUmg_BoxData{}
            .Set_Content(ToVec4(InBox.Content)).Set_Padding(ToVec4(InBox.Padding))
            .Set_Border(ToVec4(InBox.Border)).Set_Margin(ToVec4(InBox.Margin));
    }

    auto
    ToIrBox(
        const FCk_WebUmg_BoxData& InData)
        -> FCkWebUmg_IrBox
    {
        return FCkWebUmg_IrBox{
            ToRect(InData.Get_Content()), ToRect(InData.Get_Padding()),
            ToRect(InData.Get_Border()), ToRect(InData.Get_Margin())};
    }

    auto
    FlattenNode(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        TArray<FCk_WebUmg_NodeData>& InOutNodes,
        TArray<FCk_WebUmg_ReportEntryData>& InOutReport)
        -> int32
    {
        for (const auto& Entry : InNode->Unsupported)
        {
            InOutReport.Add(FCk_WebUmg_ReportEntryData{}
                .Set_NodeId(InNode->Id).Set_Property(Entry.Property)
                .Set_Value(Entry.Value).Set_Source(Entry.Source));
        }
        const auto Index = InOutNodes.AddDefaulted();
        {
            auto& Data = InOutNodes[Index];
            Data.Set_Id(InNode->Id).Set_Tag(InNode->Tag).Set_CkName(InNode->CkName)
                .Set_CkBind(InNode->CkBind).Set_CkSlot(InNode->CkSlot).Set_AssetId(InNode->Asset);
            Data.Set_Box(ToBoxData(InNode->Box));
            if (InNode->BoxUntransformed.IsSet())
            { Data.Set_HasBoxUntransformed(true).Set_BoxUntransformed(ToBoxData(*InNode->BoxUntransformed)); }

            const auto& L = InNode->Layout;
            auto Layout = FCk_WebUmg_LayoutData{}
                .Set_Display(L.Display).Set_Direction(L.Direction).Set_Justify(L.Justify)
                .Set_Align(L.Align).Set_AlignSelf(L.AlignSelf).Set_AlignContent(L.AlignContent)
                .Set_Wrap(L.Wrap).Set_Position(L.Position).Set_Basis(L.Basis).Set_BoxSizing(L.BoxSizing)
                .Set_OverflowX(L.OverflowX).Set_OverflowY(L.OverflowY)
                .Set_Gap(L.Gap).Set_Grow(L.Grow).Set_Shrink(L.Shrink)
                .Set_ZIndex(L.ZIndex).Set_Order(L.Order)
                .Set_SizingAuthored(L.SizingAuthored)
                .Set_MinMaxSize(FVector4f{
                    L.MinSize[0].Get(-1.0f), L.MinSize[1].Get(-1.0f),
                    L.MaxSize[0].Get(-1.0f), L.MaxSize[1].Get(-1.0f)});
            if (L.Inset.IsSet())
            {
                Layout.Set_HasInset(true)
                    .Set_InsetTop(L.Inset->Top).Set_InsetRight(L.Inset->Right)
                    .Set_InsetBottom(L.Inset->Bottom).Set_InsetLeft(L.Inset->Left)
                    .Set_InsetAuthored(L.Inset->Authored);
            }
            Data.Set_Layout(Layout);

            const auto& P = InNode->Paint;
            auto Paint = FCk_WebUmg_PaintData{}
                .Set_BackgroundImageAsset(P.BackgroundImageAsset.Get(FString{}))
                .Set_BorderRadius(P.BorderRadius).Set_BorderWidth(P.BorderWidth)
                .Set_BorderColors(P.BorderColors)
                .Set_HasUntypedShadow(P.HasUntypedShadow)
                .Set_Opacity(P.Opacity).Set_Visibility(P.Visibility);
            if (P.BackgroundColor.IsSet())
            { Paint.Set_HasBackgroundColor(true).Set_BackgroundColor(*P.BackgroundColor); }
            if (P.BorderColor.IsSet())
            { Paint.Set_HasBorderColor(true).Set_BorderColor(*P.BorderColor); }
            if (P.Gradient.IsSet())
            {
                auto Gradient = FCk_WebUmg_GradientData{}.Set_GradientType(P.Gradient->GradientType);
                if (P.Gradient->AngleDeg.IsSet())
                { Gradient.Set_HasAngle(true).Set_AngleDeg(*P.Gradient->AngleDeg); }
                if (P.Gradient->RadialCenter.IsSet() && P.Gradient->RadialRadius.IsSet())
                {
                    Gradient.Set_HasRadialGeometry(true)
                        .Set_RadialCenter(*P.Gradient->RadialCenter).Set_RadialRadius(*P.Gradient->RadialRadius);
                }
                auto Stops = TArray<FCk_WebUmg_GradientStopData>{};
                for (const auto& Stop : P.Gradient->Stops)
                {
                    Stops.Add(FCk_WebUmg_GradientStopData{}
                        .Set_Color(Stop.Color).Set_PosPct(Stop.PosPct.Get(-1.0f)));
                }
                Gradient.Set_Stops(Stops);
                Paint.Set_HasGradient(true).Set_Gradient(Gradient);
            }
            auto ShadowLayers = TArray<FCk_WebUmg_ShadowLayerData>{};
            for (const auto& Layer : P.ShadowLayers)
            {
                ShadowLayers.Add(FCk_WebUmg_ShadowLayerData{}
                    .Set_Color(Layer.Color).Set_Offset(Layer.Offset)
                    .Set_Blur(Layer.Blur).Set_Spread(Layer.Spread).Set_Inset(Layer.Inset));
            }
            Paint.Set_ShadowLayers(ShadowLayers);
            if (P.Transform.IsSet())
            {
                Paint.Set_HasTransform(true)
                    .Set_TransformMatrix(P.Transform->Matrix).Set_TransformOrigin(P.Transform->Origin);
            }
            Data.Set_Paint(Paint);

            if (InNode->Text.IsSet())
            {
                const auto& T = *InNode->Text;
                auto Text = FCk_WebUmg_TextData{}
                    .Set_Content(T.Content).Set_Family(T.Family).Set_SizePx(T.SizePx).Set_Weight(T.Weight)
                    .Set_LineHeightPx(T.LineHeightPx.Get(-1.0f)).Set_LetterSpacingPx(T.LetterSpacingPx)
                    .Set_Align(T.Align).Set_WhiteSpace(T.WhiteSpace).Set_TransformCase(T.TransformCase);
                if (T.Color.IsSet())
                { Text.Set_HasColor(true).Set_Color(*T.Color); }
                auto LineBoxes = TArray<FVector4f>{};
                for (const auto& Line : T.LineBoxes)
                { LineBoxes.Add(ToVec4(Line)); }
                Text.Set_LineBoxes(LineBoxes);
                Data.Set_HasText(true).Set_Text(Text);
            }
        }

        auto ChildIndices = TArray<int32>{};
        for (const auto& Child : InNode->Children)
        { ChildIndices.Add(FlattenNode(Child, InOutNodes, InOutReport)); }
        InOutNodes[Index].Set_ChildIndices(ChildIndices);
        return Index;
    }

    auto
    UnflattenNode(
        const TArray<FCk_WebUmg_NodeData>& InNodes,
        int32 InIndex)
        -> TSharedPtr<FCkWebUmg_IrNode>
    {
        if (NOT InNodes.IsValidIndex(InIndex))
        { return {}; }

        const auto& Data = InNodes[InIndex];
        auto Node = MakeShared<FCkWebUmg_IrNode>();
        Node->Id = Data.Get_Id();
        Node->Tag = Data.Get_Tag();
        Node->CkName = Data.Get_CkName();
        Node->CkBind = Data.Get_CkBind();
        Node->CkSlot = Data.Get_CkSlot();
        Node->Asset = Data.Get_AssetId();
        Node->Box = ToIrBox(Data.Get_Box());
        if (Data.Get_HasBoxUntransformed())
        { Node->BoxUntransformed = ToIrBox(Data.Get_BoxUntransformed()); }

        const auto& Layout = Data.Get_Layout();
        auto& L = Node->Layout;
        L.Display = Layout.Get_Display(); L.Direction = Layout.Get_Direction(); L.Justify = Layout.Get_Justify();
        L.Align = Layout.Get_Align(); L.AlignSelf = Layout.Get_AlignSelf(); L.AlignContent = Layout.Get_AlignContent();
        L.Wrap = Layout.Get_Wrap(); L.Position = Layout.Get_Position(); L.Basis = Layout.Get_Basis();
        L.BoxSizing = Layout.Get_BoxSizing(); L.OverflowX = Layout.Get_OverflowX(); L.OverflowY = Layout.Get_OverflowY();
        L.Gap = Layout.Get_Gap(); L.Grow = Layout.Get_Grow(); L.Shrink = Layout.Get_Shrink();
        L.ZIndex = Layout.Get_ZIndex(); L.Order = Layout.Get_Order();
        L.SizingAuthored = Layout.Get_SizingAuthored();
        const auto& MinMax = Layout.Get_MinMaxSize();
        if (MinMax.X >= 0.0f) { L.MinSize[0] = MinMax.X; }
        if (MinMax.Y >= 0.0f) { L.MinSize[1] = MinMax.Y; }
        if (MinMax.Z >= 0.0f) { L.MaxSize[0] = MinMax.Z; }
        if (MinMax.W >= 0.0f) { L.MaxSize[1] = MinMax.W; }
        if (Layout.Get_HasInset())
        {
            auto Inset = FCkWebUmg_IrInset{};
            Inset.Top = Layout.Get_InsetTop(); Inset.Right = Layout.Get_InsetRight();
            Inset.Bottom = Layout.Get_InsetBottom(); Inset.Left = Layout.Get_InsetLeft();
            Inset.Authored = Layout.Get_InsetAuthored();
            L.Inset = Inset;
        }

        const auto& Paint = Data.Get_Paint();
        auto& P = Node->Paint;
        if (Paint.Get_HasBackgroundColor()) { P.BackgroundColor = Paint.Get_BackgroundColor(); }
        if (Paint.Get_HasBorderColor()) { P.BorderColor = Paint.Get_BorderColor(); }
        if (NOT Paint.Get_BackgroundImageAsset().IsEmpty())
        { P.BackgroundImageAsset = Paint.Get_BackgroundImageAsset(); }
        P.BorderRadius = Paint.Get_BorderRadius(); P.BorderWidth = Paint.Get_BorderWidth();
        P.BorderColors = Paint.Get_BorderColors();
        P.HasUntypedShadow = Paint.Get_HasUntypedShadow();
        P.Opacity = Paint.Get_Opacity(); P.Visibility = Paint.Get_Visibility();
        if (Paint.Get_HasGradient())
        {
            auto Gradient = FCkWebUmg_IrGradient{};
            const auto& G = Paint.Get_Gradient();
            Gradient.GradientType = G.Get_GradientType();
            if (G.Get_HasAngle()) { Gradient.AngleDeg = G.Get_AngleDeg(); }
            if (G.Get_HasRadialGeometry())
            {
                Gradient.RadialCenter = G.Get_RadialCenter();
                Gradient.RadialRadius = G.Get_RadialRadius();
            }
            for (const auto& Stop : G.Get_Stops())
            {
                auto IrStop = FCkWebUmg_IrGradientStop{};
                IrStop.Color = Stop.Get_Color();
                if (Stop.Get_PosPct() >= 0.0f) { IrStop.PosPct = Stop.Get_PosPct(); }
                Gradient.Stops.Add(IrStop);
            }
            P.Gradient = Gradient;
        }
        for (const auto& Layer : Paint.Get_ShadowLayers())
        {
            P.ShadowLayers.Add(FCkWebUmg_IrShadowLayer{
                Layer.Get_Color(), Layer.Get_Offset(), Layer.Get_Blur(), Layer.Get_Spread(), Layer.Get_Inset()});
        }
        if (Paint.Get_HasTransform())
        {
            auto Transform = FCkWebUmg_IrTransform{};
            Transform.Matrix = Paint.Get_TransformMatrix();
            Transform.Origin = Paint.Get_TransformOrigin();
            P.Transform = Transform;
        }

        if (Data.Get_HasText())
        {
            const auto& T = Data.Get_Text();
            auto Text = FCkWebUmg_IrText{};
            Text.Content = T.Get_Content(); Text.Family = T.Get_Family();
            Text.SizePx = T.Get_SizePx(); Text.Weight = T.Get_Weight();
            if (T.Get_LineHeightPx() >= 0.0f) { Text.LineHeightPx = T.Get_LineHeightPx(); }
            Text.LetterSpacingPx = T.Get_LetterSpacingPx();
            if (T.Get_HasColor()) { Text.Color = T.Get_Color(); }
            Text.Align = T.Get_Align(); Text.WhiteSpace = T.Get_WhiteSpace();
            Text.TransformCase = T.Get_TransformCase();
            for (const auto& Line : T.Get_LineBoxes())
            { Text.LineBoxes.Add(ToRect(Line)); }
            Node->Text = Text;
        }

        for (const auto ChildIndex : Data.Get_ChildIndices())
        {
            if (auto Child = UnflattenNode(InNodes, ChildIndex); Child != nullptr)
            { Node->Children.Add(Child); }
        }
        return Node;
    }

    auto
    CollectDuplicateCkName(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        TSet<FString>& InOutSeen,
        FString& OutDuplicate)
        -> bool
    {
        if (NOT InNode->CkName.IsEmpty())
        {
            if (InOutSeen.Contains(InNode->CkName))
            {
                OutDuplicate = InNode->CkName;
                return true;
            }
            InOutSeen.Add(InNode->CkName);
        }
        for (const auto& Child : InNode->Children)
        {
            if (CollectDuplicateCkName(Child, InOutSeen, OutDuplicate))
            { return true; }
        }
        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::webumg
{
    auto
    ConvertIrToAsset(
        const FCkWebUmg_IrDocument& InDocument,
        const FString& InSourceHash,
        UCk_WebUmg_PageAsset_UE& InOutAsset)
        -> bool
    {
        using namespace ck_webumg_pageassetconvert;

        const auto RootIsValid = InDocument.Root != nullptr;
        CK_ENSURE_IF_NOT(RootIsValid, TEXT("ConvertIrToAsset: document has no root"))
        {}
        if (NOT RootIsValid)
        { return false; }

        auto SeenNames = TSet<FString>{};
        auto Duplicate = FString{};
        const auto HasDuplicate = CollectDuplicateCkName(InDocument.Root, SeenNames, Duplicate);
        CK_ENSURE_IF_NOT(NOT HasDuplicate,
            TEXT("ConvertIrToAsset: duplicate data-ck-name [{}] — emission is a hard error (DECISION 3)"),
            Duplicate)
        {}
        if (HasDuplicate)
        { return false; }

        auto Nodes = TArray<FCk_WebUmg_NodeData>{};
        auto Report = TArray<FCk_WebUmg_ReportEntryData>{};
        FlattenNode(InDocument.Root, Nodes, Report);
        for (const auto& Diagnostic : InDocument.Diagnostics)
        {
            Report.Add(FCk_WebUmg_ReportEntryData{}
                .Set_NodeId(Diagnostic.Node).Set_Property(Diagnostic.Kind)
                .Set_Value(Diagnostic.Detail).Set_Source(TEXT("page-diagnostic")));
        }

        InOutAsset.Set_SchemaVersion(InDocument.Schema);
        InOutAsset.Set_Viewport(InDocument.Viewport);
        InOutAsset.Set_Browser(InDocument.Browser);
        InOutAsset.Set_SourceHash(InSourceHash);
        InOutAsset.Set_Nodes(Nodes);
        InOutAsset.Set_ConversionReport(Report);
        return true;
    }

    auto
    ConvertAssetToIr(
        const UCk_WebUmg_PageAsset_UE& InAsset)
        -> FCkWebUmg_IrDocument
    {
        using namespace ck_webumg_pageassetconvert;

        auto Document = FCkWebUmg_IrDocument{};
        Document.Schema = InAsset.Get_SchemaVersion();
        Document.Viewport = InAsset.Get_Viewport();
        Document.Browser = InAsset.Get_Browser();
        Document.Root = UnflattenNode(InAsset.Get_Nodes(), 0);
        for (const auto& [AssetId, Texture] : InAsset.Get_Textures())
        { Document.AssetSourcesById.Add(AssetId, AssetId); } // presence only; brushes come from the asset's textures
        return Document;
    }
}

// --------------------------------------------------------------------------------------------------------------------
