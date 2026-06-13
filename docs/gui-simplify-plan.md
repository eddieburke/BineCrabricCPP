# GUI簡化總綱 — gui-simplify-plan

> 旨:GUI渲染碼乃Java直譯,遺風重。除Java垢,取C++利器(RAII、lambda、free fn、constexpr)。
> 行為不變 — pixel-parity絕對律。
> 此綱為小agent所設:一agent一段,段內依序,勿越段,勿自創。

---

## 律(每agent必守)

1. **行為不變**。渲染輸出、輸入處理、音效、順序 — 全同前。疑 → 勿改,記於progress檔。
2. **勿stub**。勿placeholder。勿TODO代碼。
3. 每段畢 → 執 `powershell -File "C:\Users\Eddie\Documents\New project 2\native\build-omega.ps1"` → 零error方算成。
4. 段畢 → 記 `native/docs/gui-simplify-progress.md` 一行:`段N | done|blocked | YYYY-MM-DD | 註`。blocked必書因。
5. 純C++。勿codegen、勿JSON/Python間接、勿新依賴、勿第三方UI庫。
6. 替換前必先`Read`目標檔全文。grep數與本綱不符 → 碼已變 → 重查,勿盲改。
7. 一段一commit。commit message英文正常體。

## 地圖

根:`native/src/net/minecraft/client/gui/`

| 區 | 要檔 |
|---|---|
| 核 | `DrawContext.hpp/.cpp`、`screen/Screen.hpp/.cpp` |
| widget | `widget/ButtonWidget.*`、`widget/EntryListWidget.*`(GuiSlot直譯,最垢)、`widget/SliderWidget.*`、`widget/OptionButtonWidget.*`、`widget/TextFieldWidget.*` |
| hud | `hud/InGameHud.*`(17KB)、`hud/toast/AchievementToast.*` |
| screen | `screen/ingame/HandledScreen.*`、`screen/StatsScreen.cpp`(17KB)、`screen/AchievementsScreen.cpp`(18KB)、`screen/option/*` |
| 共用 | `screen/option/OptionGui.hpp`(builder,已良)、`layout/*` |

## Java七罪

1. `Screen`/`ButtonWidget` 繼承 `DrawContext`(Java `Gui` God-base直譯)→ 應free fn。
2. raw tessellator四頂點litany ×15處(`startQuads()` + 4×`vertex(static_cast<double>...)`)。
3. magic keycode散布(1=Esc、28=Enter、14=Backspace、200=Up、208=Down、87=F11、47=V)。
4. magic GL enum(`glEnable(32826)`、blend `0`/`769`)。
5. id-dispatch + `dynamic_cast`(Java `actionPerformed` 直譯):`buttonClicked(ButtonWidget&)` override ×8、`button.id`比對。
6. `ItemRenderer itemRenderer` 散造 ×5處。
7. sentinel float(`mostYStart_` = -1/-2)、外類reach-back(`dynamic_cast<StatsScreen*>(minecraft_.currentScreen())` ×4)。

---

## 段一:Draw2D quad helpers(機械,低險,先行)

**造** `gui/Draw2D.hpp` + `Draw2D.cpp`,namespace `net::minecraft::client::gui::draw`:

```cpp
// 頂點序固定:(x1,y2)(x2,y2)(x2,y1)(x1,y1) — BL,BR,TR,TL
void quad(render::Tessellator& t, int x1, int y1, int x2, int y2, float z = 0.0f);
void texturedQuad(render::Tessellator& t, int x1, int y1, int x2, int y2,
    float u1, float v1, float u2, float v2, float z = 0.0f);
```

**替** raw litany。現存 `startQuads()` 15處:

| 檔 | 數 |
|---|---|
| `DrawContext.cpp` | 3 |
| `widget/EntryListWidget.cpp` | 8 |
| `hud/InGameHud.cpp` | 2 |
| `screen/Screen.cpp` | 1 |
| `screen/pack/PackScreen.cpp` | 1 |

**鐵則**(此處離caveman,序攸關):

- 替換前,逐site核對其四個 `vertex()` 呼叫的頂點順序。順序恰為 (x1,y2)(x2,y2)(x2,y1)(x1,y1) 者方可替換。
- 順序不同者(例:`DrawContext::fillGradient` 乃 (x2,y1)(x1,y1)(x1,y2)(x2,y2)),以及 quad 中途插 `tessellator.color(...)` 呼叫者:**不替**,原樣留,progress檔記「site X 序異,skipped」。
- UV算式、`texel 0.00390625f`、`tile 32.0f`:逐字搬,勿「簡化」數學。

驗:build零error。

## 段二:Keys常數

**造** `input/Keys.hpp`,namespace `net::minecraft::client::input::keys`:

```cpp
// DInput/LWJGL scancodes — 值勿改
inline constexpr int kEscape = 1;
inline constexpr int kBackspace = 14;
inline constexpr int kV = 47;
inline constexpr int kEnter = 28;
inline constexpr int kF11 = 87;
inline constexpr int kUp = 200;
inline constexpr int kDown = 208;
```

**替**已知site(先grep `keyCode == \d+|key == \d+|eventKey\(\) == \d+` 於 gui/ 重核):

- `screen/Screen.cpp:95`(87→kF11)、`:110`(1→kEscape)
- `screen/ChatScreen.cpp:55,61,80`(1,28,14)
- `screen/SleepingChatScreen.cpp:27,31`(1,28)
- `screen/ingame/SignEditScreen.cpp:32,36,40`(200,208,28,14)
- `screen/ingame/HandledScreen.cpp:232,236`(1)
- `widget/TextFieldWidget.cpp:50,64`(47,14)

純文字替換,零行為變。驗:build。

## 段三:GL常數名 + RAII guard

1. `HandledScreen.cpp:53,81`:`glEnable(32826)`/`glDisable(32826)` → 先查 `gl/GL11.hpp` 有無 `GL_RESCALE_NORMAL`(=32826=0x803A);無則加constexpr,後替。
2. `hud/InGameHud.cpp:27-28`:`kVignetteBlendSrc=0` → 命名 `GL_ZERO`;`kVignetteBlendDst=769`(=0x0301)→ `GL_ONE_MINUS_SRC_COLOR`。GL11.hpp無此二常數則加。值必核對。
3. RAII guard:`render/platform/GuiGlState` 已有 `ScopedUnlitText` 前例。加:
   - `ScopedNoTexture2D`(ctor `glDisable(GL_TEXTURE_2D)`,dtor `glEnable`)
   - `ScopedSmoothShade`(ctor `glShadeModel(GL_SMOOTH)`,dtor `GL_FLAT`)
   - **只替嚴格成對且嚴格嵌套**之 enable/disable。中途有早return、或disable/enable序交錯者:不替。EntryListWidget::render 尾部 enable/shade/endAlphaText 交錯 — 該檔此段勿動。

驗:build + 運行開inventory視覺同前。

## 段四:option label refresh去重

三處同型loop(dynamic_cast `OptionButtonWidget`/`SliderWidget` → 刷新label text):

- `screen/option/OptionsScreen.cpp:204`
- `screen/option/SettingsScreen.cpp:77`
- `screen/option/WorldSettingsScreen.cpp:266`

三處先全讀。全同 → 提升至 `OptionGui.hpp`:

```cpp
inline void refreshOptionLabels(screen::Screen& screen, const client_option::GameOptions& options);
```

異者 → 參數化;不能參數化者留原。驗:build + options畫面toggle後label即刷。

## 段五:id-dispatch死,callback生(大段,分步,一步一commit)

今態:`Screen::dispatchButtonPress` 先試 `dynamic_cast<ActionButtonWidget>` 取 `onClick`,敗則落 `buttonClicked(button)` virtual + `button.id` 比對。`buttonClicked` override 現存8處:`EntryListWidget`、`StatsScreen`、`KeybindsScreen`、`OptionsScreen`、`PackScreen`、`SelectWorldScreen`、`SettingsScreen`、`WorldSettingsScreen`。

逐步(每步:讀screen全文 → 知每id何義 → 換 `addActionButton`/onClick lambda → 刪該override → build):

- **5a** `KeybindsScreen` — 注意 `button.id` 兼作keybind index(`getKeybindKey(button.id)`),lambda須capture index。
- **5b** `PackScreen` — 轉發 `packList_->buttonClicked(button)`,與5e連動,讀兩檔再動。
- **5c** `SelectWorldScreen` — 同上轉發型。
- **5d** `StatsScreen` — 轉發 `selectedStatsList_->buttonClicked(button)`。
- **5e** `EntryListWidget` — `scrollUpButtonId_`/`scrollDownButtonId_` 比對 → 公開 `scrollBy(int)` method,呼side造ActionButton呼之;`registerButtons(std::vector<ButtonWidget>&,...)` 死參數一併除。
- **5f** `OptionsScreen`/`SettingsScreen`/`WorldSettingsScreen` — OptionButtonWidget/SliderWidget click經id→registry index。widget已有 `getRegistryIndex()`。dispatch改:helper `handleOptionButtonClick(Screen&, ButtonWidget&)` 於OptionGui.hpp,用 `getRegistryIndex()` 非 `id`。三screen行為全同方可共用。
- **5g 終結**:grep `buttonClicked` → 唯 `Screen.hpp/.cpp` 殘 → 刪 `Screen::buttonClicked` + `dispatchButtonPress` 之fallback。grep `\.id\b` 於 gui/ → 無餘用者方刪 `ButtonWidget::id`;有餘 → 留,progress記。

每步驗:build + 該screen手測(按鈕全按一遍)。

## 段六:StatsScreen reach-back

`StatsScreen.cpp:148,171,262,285`:`dynamic_cast<StatsScreen*>(minecraft_.currentScreen())` — Java inner-class直譯。列表entry改持 `StatsScreen&` member,構造時注入。dynamic_cast全除。驗:build + stats畫面排序click。

## 段七:ItemRenderer合一

散造5處:

- `hud/InGameHud.cpp:130`(function-local static)
- `hud/toast/AchievementToast.cpp:123`(每呼造!)
- `screen/AchievementsScreen.cpp:247`
- `screen/StatsScreen.cpp:427`(static)
- `screen/ingame/HandledScreen.cpp:20`(file-scope)

先讀 `render/item/ItemRenderer.hpp` 證其stateless(或state僅渲染中暫態)。是 → 造單一accessor:

```cpp
// gui/Draw2D.hpp
render::item::ItemRenderer& guiItemRenderer();  // Meyers singleton於Draw2D.cpp
```

5處全替。非stateless → 止,progress記何state。驗:build + hotbar/inventory/toast/stats/achievements item圖標同前。

## 段八:DrawContext繼承→free fn(最大,最後)

今:`class Screen : public gui::DrawContext`、`class ButtonWidget : public gui::DrawContext` — 為繼承 `fill`/`fillGradient`/`drawTexture`/`drawCenteredTextWithShadow`/`drawTextWithShadow` 五法 + `zOffset`。

1. 先grep `zOffset` 全gui/ — 知誰設非零(toast疑)。
2. 五法遷 `gui::draw` namespace(Draw2D),`zOffset` 改顯式 `float z = 0.0f` 參數。
3. 機械替:子類內裸呼 `fill(...)` → `draw::fill(...)`;`drawTexture(...)` → `draw::texture(..., z)`,z=原zOffset值(多為0,toast傳己值)。
4. 一目錄一sub-step一build:`widget/` → `hud/` → `screen/` → `screen/option/` → `screen/ingame/` → `screen/world/` → 餘。
5. 末:`Screen`/`ButtonWidget` 除 `: public gui::DrawContext`;`DrawContext` 空殼則刪檔+include清理。

驗:每sub-step build;終了全畫面smoke。

## 段九:小垢(各自獨立,擇行)

- `EntryListWidget` sentinel:`mostYStart_` 之 -1/-2 魔值 → `enum class DragState { Idle, Dragging, Suppressed }` + `float dragY_`。狀態轉移逐一對照原碼,行為全同。
- `InGameHud::addChatMessage` O(n²) substr寬度掃 → 線性walk。輸出分行必bit-同。不確定 → 勿動。
- `DrawContext::unpackColor` 之 `a<=0→1.0` hack:**留**(parity!),僅加註:`// alpha==0 treated as opaque — beta 1.7.3 parity, callers pass 24-bit colors`。

## 禁區

- `EntryListWidget::render` 內嵌input邏輯(drag/scrollbar於render中)— **勿拆**。parity險高。後人另案。
- 勿改:頂點winding、UV數學、`0.00390625f`、`32.0f` tile、blend mode、色值。
- 勿改 `screen/option/OptionGui.hpp` builder結構(已良,僅5f加helper)。
- 勿動 gui/ 外渲染碼(render/、font/)— 越界。

## 驗收(全段畢)

1. build零warning新增。
2. smoke:title→options全子頁→video→keybinds;入世界→inventory→crafting→furnace→chest→chat→sign→stats→achievements→死亡畫面→pause→multiplayer畫面。各一過,視覺同前。
3. grep終查:`dynamic_cast` 於gui/ 僅餘player/world類型者;`startQuads` 於gui/ 僅餘序異skipped者;`buttonClicked` 零;magic keycode零。
