# gui-simplify-progress

綱:`gui-simplify-plan.md`。每段一行,agent畢段即記。

格式:`段N | done|blocked|partial | YYYY-MM-DD | 註(skipped sites、blocked因、grep異常)`

| 段 | 態 | 日 | 註 |
|---|---|---|---|
| 一 Draw2D helpers | done | 2026-06-12 | API擴:quad/coloredQuad/verticalGradientQuad/texturedQuad/coloredTexturedQuad/verticalGradientTexturedQuad。13 site替(DrawContext fill+drawTexture、InGameHud×2、EntryListWidget×8、PackScreen、Screen bg)。唯fillGradient skipped(winding異+float-color)。/tile UV安全(÷32 exact)。build exit 0 |
| 二 Keys常數 | done | 2026-06-12 | Keys.hpp(input::keys::k*);移出InputSystem.hpp。7檔15 site替(Screen×2、Chat×3、SleepingChat×2、SignEdit×4、Handled×2、TextField×2)。grep gui/ magic keycode零。build exit 0 |
| 三 GL常數+RAII | done | 2026-06-12 | GL11:+RESCALE_NORMAL/COLOR_MATERIAL/ONE_MINUS_SRC_COLOR/ONE_MINUS_DST_COLOR; GuiGlState遷移+刪local const+beginVignetteBlend; ScopedNoTexture2D/SmoothShade/RescaleNormal/ProfilerDraw; RAII DrawContext×2/EntryListWidget selection/HandledScreen/StatsScreen; gui裸GL零; EntryListWidget render尾skipped; ClientProfilerOverlay→ScopedProfilerDraw。build exit 0 |
| 四 option refresh去重 | done | 2026-06-12 | refreshOptionLabels→OptionGui.hpp;三屏同型loop除(Options/Settings/WorldSettings)。build exit 0 |
| 五 id-dispatch→callback | done | 2026-06-12 | 5a Keybinds→ActionButton+selectKeybind;5b-d Pack/SelectWorld/Stats刪轉發;5e EntryListWidget scrollBy+刪registerButtons/buttonClicked;5f handleOptionButtonClick→OptionGui+Screen dispatch;5g刪Screen::buttonClicked。ButtonWidget::id留(ctor參數)。build exit 0 |
| 六 StatsScreen reach-back | — | — | — |
| 七 ItemRenderer合一 | — | — | — |
| 八 DrawContext繼承除 | — | — | — |
| 九 小垢 | — | — | — |
