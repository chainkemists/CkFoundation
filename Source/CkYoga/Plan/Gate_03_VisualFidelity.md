# Gate 03: Resource Inspector visual fidelity

**Status:** final bounded wide-fidelity slice accepted; full browser parity and narrow responsive parity remain open.

## Final wide-fidelity checkpoint (2026-09-10)

The installed Resource Inspector accepts this bounded wide slice: generic bounded `letter-spacing`, authored Ck-owned table sort indicators, header/app chrome, and browser-like detail presentation. Nested-flex app-mark attempts R2-R7 were rejected by pixel review despite green geometry tests; the accepted centered inner outlined frame has distinct cyan sides at x19/x24, y24..29 in the fresh wide capture. This is not an exact browser-parity or narrow-restacking claim.

- Shared `Saved/Logs/BuildTest-UiFinalWide-R2.log`: build success; 135/135, zero failed/skipped/contaminated, 1m02, discovery 5016; SHA256 `448C2925975881CC52E88AD816E1049ECEC7D0B693C17178EA5917A38F438438`.
- Incremental consumer build `Saved/Logs/Build-ResourceInspector-FinalWide-R7.log`: success; SHA256 `923EF5223A69F2EF5476C36FD3CCE7B61FFF0D622C710C930D842E9AD519961B`.
- One-start cached real-RHI `Saved/Logs/ResourceInspector-FinalWide-R9.log`: 10/10, zero failed/skipped/contaminated, 37s; SHA256 `E8208255DDC2954AB6396295769BABE514EE1FC6CEF6A71784197C020A12ACA7`. Final relevant diagnostics scan is zero.
- Inspected capture SHA256: wide `BE438ECCC88D192DAB61C53E3EBFE585C7A730C436F9B9653A9083CD2835DAA7`; selected navigation `A867617514194F2DC993BA4AC45587B0F86B23A10C9908A7610B7E4EA77C05BF`; pinned wide `9C5392E7D14F07016631824C0E3D7F98D6F4CC76AB1DDA2D10480FF9A73352F6`; narrow `FE153D8B6EB39299C6BBDEE7A384C95AD6BDDD7FA6D4E96A4ACA32741E609F7A`.

Remaining: navigation `+` only when a product add-set contract exists; narrow breakpoint/restacking is deferred. The Gate 03 acceptance boundary still excludes whole-campaign, packaged, controller, accessibility, localization, performance, lifetime, and specialized-adapter claims.

## Decision

`[G3-VISUAL-D1]` Browser parity remains open. Functional presence is insufficient: shared authored controls must carry the browser reference's component language. Implement reusable Foundation-owned presentation primitives and skins; do not patch Resource Inspector with bespoke Slate. Defer breakpoint-driven narrow restacking until the wide component hierarchy is credible.

## Historical observed gap (resolved for the bounded wide slice)

- State cells are plain text instead of compact semantic outline pills.
- Buttons use generic Core Slate chrome with no authored normal/hover/pressed/disabled or semantic variants.
- Tabs use filled segmented buttons instead of muted labels plus a cyan active underline.
- Body/table type scale and spacing are awkward relative to the browser reference.
- Detail sections, pinned cards, table headers/rows, and navigation lack the browser's border, surface, and selection hierarchy.

## Ordered slices

1. Shared status pill -> consumer verified: Resource Inspector State cells render semantic outlined pills from typed record fields in fresh native output. The dedicated malformed-binding/live same-key test compiles but was not in the cached test inventory; run it with the next justified fresh discovery before claiming that sub-coverage.
2. Shared control skins -> consumer verified: authored buttons and tabs expose deterministic normal/hover/pressed/disabled/selected styles without changing input or retained identity.
3. Surface/table -> consumer verified: generic structural border and table header/row/hover/selected/separator contracts are applied to Resource Inspector and inspected in fresh wide/dialog/narrow captures.
4. Header/navigation/tree style -> consumer verified: header chrome, breadcrumb/connection status, toolbar hierarchy, and real selected-tree background plus cyan rail are accepted without changing native selection or retained identity.
5. Wide pane proportions and table/toolbar density -> consumer verified: inventory is dominant at approximately 61.5% normalized share with a 26px row cadence; four columns and State pills remain visible, while narrow interaction remains preserved.
6. Detail-card/table-header -> consumer verified: compact per-column header typography, a bordered `RESOURCE SUMMARY` wrapper retaining `selected-detail`, and pinned-card cadence are accepted on the installed resource fixture.
7. Menu-button/collapse -> consumer verified: shared retained menu-button visual contract and Resource Inspector hidden-arrow overflow skin are accepted; Collapse all preserves selected category/query/records/tree identity.
8. Final wide chrome -> consumer verified: centered authored app mark, Ck-owned inactive/active sort indicators, and generic bounded letter spacing are accepted. Navigation `+` still requires a real add-set product contract.
9. Responsive restacking -> deferred by CTO direction; retain narrow reachability and no-overlap gates meanwhile.

## Acceptance boundary

- Compare fresh wide native capture directly with `Workbench.reference.html` at a comparable viewport.
- Focused shared authoring and installed Resource Inspector tests pass on the final artifact with clean runtime logs.
- Hover/pressed/selected visual output is inspected, not inferred from construction.
- No whole-campaign, packaged, controller, accessibility, or narrow-reflow claim.

## Status-pill evidence

- Incremental Development Editor build: `Build-ResourceInspector-StatusPill-R3.log`, success in 12.03s.
- Final cached rendered consumer gate: `ResourceInspector-StatusPill-R2.log`, 10/10 in 42s, one editor start, zero failed/skipped/contaminated.
- Actual runtime archive: `ResourceInspector-StatusPill-R2-Editor.log`, 10 successes and zero failed-test, ensure, fatal, AngelScript, or CSS-applicability diagnostics.
- Fresh `Wide_960x640.png` and `Narrow_640x480.png` inspected. Wide shows the new colored outline pills; the fixed test layout still clips the far-right State edge, so this is status-component acceptance rather than whole-layout parity.
- Initial red consumer run `ResourceInspector-StatusPill-R1.log` proved the installed stylesheet rejection when generic `font-size` targeted a custom element. Final code uses the registry-declared `-ck-status-pill-font-size` contract.

## Button and tabs evidence

- Final incremental Development Editor build: `Build-VisualControls-R7.log`, success, SHA256 `B74A8077065F6D7CDD3EF5C44DB45638F9E45246F5332EE883C83F80D31FFA2A`.
- Exact real-RHI shared gates pass: `UiButtonVisualStyle-R4.log` and `UiTabsVisualStyle-R3.log`. The prior fresh 130-test authoring lane passed 129 siblings and exposed the button enabled-attribute publication defect; treat the final evidence as composite, not a single 130/130 run.
- Final cached rendered consumer: `ResourceInspector-VisualControls-R4.log`, 10/10 in 39s, one editor start, zero failed/skipped/contaminated; runtime archive has 10 successes and zero relevant diagnostics.
- Fresh wide/narrow/dialog captures were inspected. Controls match the browser component language materially better; whole-layout acceptance remains blocked on surface/table/navigation hierarchy and the clipped State boundary.

## Surface and table evidence

- Foundation contract: generic `border-color`, `border-width`, and `border-radius` on background-capable structural nodes; table-only `-ck-table-header-background`, `-ck-table-header-padding-x/y`, `-ck-table-row-background`, `-ck-table-row-hover-background`, `-ck-table-row-selected-background`, `-ck-table-row-separator-color`, and `-ck-table-row-separator-width`.
- Resource Inspector CSS applies the contract to the wide navy/cyan header, panes, cards, compact table header/rows, hover/selected rows, separators, and State-pill surface.
- Shared authoring gate `Ck.UiAuthoring.VisualStyle` passed 4/4; SHA256 `08BDD83925FE26F2AC1FDC04E842AE4E4CCB0195C8B52F7514289E503B831267`.
- Final cached consumer gate `ResourceInspector-SurfaceTable-R2.log` passed 10/10 in one editor start; SHA256 `BBF4A18A138C08DBD12FBC9F466C2E4F1A5EF611706EF30745556E4DFF5F45FF`. Editor archive SHA256 `E25F921778381F1410564D54278E2B80FE72C74012C6188A1E0E3CCFA55EFED4`; zero relevant diagnostic hits.
- Fresh wide capture SHA256 `1B0C99B78EAC369C2AA5A478E2F501811ABAA0E6CBE59FAC661ED2219BFAC27C` shows all four columns and State pills. Dialog SHA256 `8DF0D97B66E510A115DDFC0D799FC308B190D24B59A204D29D7881988F189B91`; narrow SHA256 `077B3EFE52F3C78228A6F3E35A6FCA09A07B08C6F7128E2543D696656C3C4E91`.
- Acceptance is limited to this surface/table slice. Gate 03 and overall browser parity remain open; narrow responsive reflow is explicitly deferred and narrow evidence is reachability-only.

## Header, navigation, and tree-style evidence

- Foundation tree visual contract accepts authored normal, hover, selected, and selected-accent paint declarations. Resource Inspector applies it while retaining real native tree selection; fresh selected-navigation capture visibly accepts the selected background and cyan rail. Header chrome, breadcrumb/connection status, toolbar hierarchy, and State pills are also accepted from fresh captures.
- Shared gate `Ck.UiAuthoring.Tree.VisualStyle` passed 1/1 in `BuildTest-TreeVisual-R3.log`; SHA256 `E0946FC1572A63949581A4F363BBB349DA9786D7E0FAFE068F771623692AC141`. Editor archive SHA256 `0E01ACAC3CB404B37564DE135E5B6F5F8E0E0FF73FFE2C13C7BEB764F4902966`.
- Final cached consumer `ResourceInspector-HeaderNav-R5.log` passed 10/10 in one Toolbox invocation with an internally isolated net-test lane; SHA256 `30ACF587D843C929986CC999ADB2EC379FB4726259682B93077BD8F62A0F158F`. Main editor archive SHA256 `EC2CAAEA9ABCD7A5EBA3E5A3A287D0065C36DAAC272F878C32D063421E0944B1`; net archive SHA256 `70B20DFC060B1E0D11649AEA0CF6E4D9FA49211089BC784A097126215EA9F90A`; diagnostics scan found zero relevant automation, ensure, fatal, AngelScript, or CSS errors.
- The automated proof covers parser application, selection, stable compatible reload, retained identity, and invalid-input rejection. Paint colors and cyan accent rail are capture-accepted visual evidence.
- Fresh captures: wide SHA256 `EA74E2C35CD7BA9AC7FCA080E1A1CD92497024E21670BCAEB76D5943810DE058`; selected navigation `E5074C02CE9D121E92DA74BB4303432C38C4249FABC471107945FD9329CBDB38`; pinned `76605611E28C589B54DB21B452ADFCCEE73B80D8F59DCB357D624B033BCD4B76`; dialog `D9136C26CFC14A516D7DA39D3E4B6BA53732E3F340572EA9B50B40BAD431E51C`.
- This accepts only the header/navigation/tree-style slice. Gate 03 and browser parity remain open. Next is wide pane proportions plus table/toolbar density; exact menu skin, letter spacing, and navigation tools remain later. Narrow responsive reflow remains explicitly deferred.

## Wide proportion and density evidence

- The accepted wide pass makes inventory dominant at approximately 61.5% normalized share, keeps a 26px table-row cadence, and preserves four visible columns plus State pills. Narrow interaction remains preserved; this does not establish responsive restacking parity.
- The initial density gate `ResourceInspector-WideDensity-R1.log` was 9/10: reduced detail flex wrapping clipped pinned Details after end-scroll. The discriminating correction restored detail weight to `.26` while retaining the inventory normalized share; R2 restored the cached consumer gate to 10/10.
- Cached `Ck.ResourceInspector` passes 10/10 in `Saved/Logs/ResourceInspector-WideDensity-R2.log`; SHA256 `D786E963232A6C50A7F1C6F070C64E531BB798B830D6578832F85EA1B766E2EE`. Editor archive `Saved/Logs/ResourceInspector-WideDensity-R2-Editor.log` SHA256 `B3FACDD8FCB74C08364E8C0CCDB66FE4AFE0E73BD603327C5057E06951D32015`; relevant diagnostics are zero.
- Fresh captures: wide SHA256 `33890A26DB1BCBDCF46A25CD3759F9B32048C43A18A291C15030E45473AD5EA0`; selected navigation `F3D2BFAEE30A59EB857229E698738557686CB114A15A0CBD1E93B7B3E5F89EFF`; pinned wide `2CDFE3319C766B75CF42899B584CFE2453D5211F935F6C39C9E4AA3E30182793`; dialog `D681FEBD44276EAED930F84F0F22F4E70B119ABD404F29CFB9FBDDA22C70B74C`; narrow `7D964EB6B65A5F619D0934A3F9FCFD4008C82B030D7E83C6977D4932B4CF4953`; pinned narrow `3CEB10DF95889ABD3F1D244A2535582675D73585D3169583246B3E8856B59677`.
- This accepts only the wide proportion/density slice. Gate 03 and browser parity remain open. Next wide work is the navigation-tool row, exact overflow/menu/icon skin, table-header typography/sort indicators, and detail eyebrow/cards; narrow responsive restacking remains explicitly deferred.

## Detail-card and table-header evidence

- Resource-only changes add compact per-column table-header typography, a bordered `RESOURCE SUMMARY` wrapper retaining the existing `selected-detail` binding, and refined pinned-card cadence.
- One cached editor spawn ran `Ck.ResourceInspector` 10/10 in 59s with zero contaminated tests. `Saved/Logs/ResourceInspector-DetailCards-R1.log` SHA256 `6DF9E524D0EF79EE0CF2F80D0AF691F407243E09A114F74CE145D44DD4F84DB8`. Relevant diagnostics are zero except for the unrelated Chromium USB device error.
- Fresh captures: wide SHA256 `6A0BDBF3FB4220CD7415E2FA60512F8E7E98E325E468402D0874B71750FF77D2`; selected `D28AED5F050B0F96E7067DACDA032E217C886D22FFB31B6498731685990161EC`; pinned wide `743076B6C7D5E80E1C8B5EFB64CD82DADD1D23781985C309B09445B13A250C4D`; dialog `E37350BFF8DF02768A3770E2517A71305B96778CCF9A76C0AD96C7266271A65D`; narrow `1203E2C76EACC9E030E822A23769D809FC2185EBE6ECD7E2860C240693ECC608`; pinned narrow `C7187C218716FC0BBBD1AAC3BBA2B7FC9687BD3A443C50E2725F3A13373151B0`.
- This accepts only the resource detail-card/table-header slice. Gate 03 and browser parity remain open. Navigation `+`/Collapse actions are absent; exact overflow/menu-button chrome, inactive-sort glyph styling, and letter spacing remain unsupported. Narrow responsive restacking remains explicitly deferred.

## Menu-button and collapse evidence

- Menu-style closeout: R3 failed only because the style-clear test retained `class="styled"` with an empty stylesheet. Adversarial review also caught exact authored 9x6 padding being ignored by value-equality presence inference. The correction adds explicit `HasContentPadding` and class-free otherwise-identical style-clear markup. Final `Saved/Logs/BuildTest-UiMenuVisual-R4.log` built successfully and passed 1/1, zero failed/contaminated, in 33s; discovery 5015; SHA256 `4976F35F42ECBAEB48AA9C636A53FD5ACDDF4882D549CC5203156A53C1F1FCFF`.
- Shared retained menu-button CSS supports normal, hover, pressed, disabled, radius, outline, padding, and visible/hidden arrow declarations with stable member-owned style and compatible reload. Resource Inspector authors the hidden-arrow overflow skin. Its real Collapse all action preserves selected category, query, records, and tree identity; `+` remains intentionally absent pending a product add-set contract.
- `BuildTest-ResourceInspector-MenuCollapse-R1.log` SHA256 `3DD4CA8DDB70974C8E89CE7FF230ABEE0EE760A5DD93C40B075CB38A0CBE786F` built successfully; `Ck.ResourceInspector` passed 10/10, zero contaminated, in 1m26s. Actual scheduler cost was three editor starts (net-pinned one, main nine, inline discovery), correcting the planned two.
- `UiMenus-Visual-R1.log` SHA256 `7FAC61A6DBABB50E75BB55718DB950412465326E8F1544222955B07890BE1471` was 9/10 because the new test omitted synthetic hover move, causing zero action calls and cascading reopen failure; production consumer was green. Test-only fix `BuildTest-UiMenuVisual-R2.log` SHA256 `7E378EC958E1812FCEAB2209E875E057E5069DE042B371129BA390551D2A4F2B` built successfully and passed VisualStyle 1/1, zero contaminated, in 36s across two editor starts (lane plus discovery). Relevant final scans are zero except unrelated Chromium USB device error.
- Fresh captures: wide SHA256 `6FBCBFDC0366A68D6C3480181102F8646B03365CE3A971328338DBA37A2283C0`; selected `8A472DF97509F6C698C5C20CDE5469FA2C23DE351631D45433ACE2AD76B5D863`; pinned wide `A39BA8D1D856290B8B0920BA32515BFCE9D0AF19720236FA31B4B3741230EAB1`; dialog `884E550D1FC17FF9945FDF80489CA336FD04D908FDF4F8D7D60B7954182B1A5A`; narrow `4F49082440B750036355A0E4DCF4310F8C0DEF197A86779A3B4C6611136D22D8`; pinned narrow `ADF74AA82B5232B8E10381D31C42A9294A9AD37F60327257DAF9DA79E9C3EC5A`.
- This accepts only the menu-button/collapse slice. Gate 03 and browser parity remain open. `+` requires a real add-set product contract; exact app mark/icon, inactive-sort glyph styling, and letter spacing remain; narrow responsive restacking remains deferred.
