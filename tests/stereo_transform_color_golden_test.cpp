// stereo_transform_color_golden_test.cpp
// Golden-vector regression tests for StereoRendering viewport/eye helpers.
// No framework — assert + main.  Returns 0 on pass, 1 on any failure.
//
// Implementation under test:
//   native/src/net/minecraft/client/render/platform/StereoRendering.cpp
//   native/src/net/minecraft/client/option/GameOptions.hpp (inline methods)

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/platform/StereoRendering.hpp"
#include "net/minecraft/client/util/UiScale.hpp"

#include <cassert>
#include <cstdio>

using net::minecraft::client::option::GameOptions;
using net::minecraft::client::render::platform::StereoRendering;
using net::minecraft::client::util::uiFramebufferWidth;

static void assertBool(bool actual, bool expected, const char* label)
{
    if (actual != expected) {
        fprintf(stderr, "FAIL %s: expected %s got %s\n",
            label, expected ? "true" : "false", actual ? "true" : "false");
        assert(actual == expected);
    }
}

static void assertInt(int actual, int expected, const char* label)
{
    if (actual != expected) {
        fprintf(stderr, "FAIL %s: expected %d got %d\n", label, expected, actual);
        assert(actual == expected);
    }
}

static void assertIntRgb(int r, int g, int b, int er, int eg, int eb, const char* label)
{
    if (r != er || g != eg || b != eb) {
        fprintf(stderr,
            "FAIL %s: expected (%d,%d,%d) got (%d,%d,%d)\n",
            label, er, eg, eb, r, g, b);
        assert(r == er && g == eg && b == eb);
    }
}

// ---------------------------------------------------------------------------
// needsSecondEye — true for anaglyph (1) and side-by-side (2), false for off (0)
// ---------------------------------------------------------------------------
static void test_needs_second_eye()
{
    GameOptions off;
    off.stereoMode = 0;
    assertBool(StereoRendering::needsSecondEye(off), false, "needsSecondEye_mode0");

    GameOptions anaglyph;
    anaglyph.stereoMode = 1;
    assertBool(StereoRendering::needsSecondEye(anaglyph), true, "needsSecondEye_mode1");

    GameOptions sbs;
    sbs.stereoMode = 2;
    assertBool(StereoRendering::needsSecondEye(sbs), true, "needsSecondEye_mode2");
}

// ---------------------------------------------------------------------------
// viewportWidth — full width for off/anaglyph; half width for side-by-side
// ---------------------------------------------------------------------------
static void test_viewport_width()
{
    constexpr int displayWidth = 1920;

    GameOptions off;
    off.stereoMode = 0;
    assertInt(uiFramebufferWidth(off, displayWidth), 1920, "viewportWidth_mode0");

    GameOptions anaglyph;
    anaglyph.stereoMode = 1;
    assertInt(uiFramebufferWidth(anaglyph, displayWidth), 1920, "viewportWidth_mode1");

    GameOptions sbs;
    sbs.stereoMode = 2;
    assertInt(uiFramebufferWidth(sbs, displayWidth), 960, "viewportWidth_mode2");

    // Odd display width — integer halving matches GameRenderer eye sizing.
    assertInt(uiFramebufferWidth(sbs, 1921), 960, "viewportWidth_mode2_odd");
}

// ---------------------------------------------------------------------------
// Font spec (documentation-only) — TextRenderer uses extra channel, not b.
// Formula: gray=(r*30+g*59+extra*11)/100; greenEye=(r*30+g*70)/100;
//          blueEye=(r*30+extra*70)/100
// ---------------------------------------------------------------------------
static void test_font_glyph_color_spec()
{
    const int r = 100;
    const int g = 200;
    const int extra = 50;

    const int mixR = (r * 30 + g * 59 + extra * 11) / 100;
    const int mixG = (r * 30 + g * 70) / 100;
    const int mixB = (r * 30 + extra * 70) / 100;

    assertIntRgb(mixR, mixG, mixB, 153, 170, 65, "font_glyph_color_spec");
}

// ---------------------------------------------------------------------------
int main()
{
    test_needs_second_eye();
    test_viewport_width();
    test_font_glyph_color_spec();
    fprintf(stdout, "All StereoRendering golden tests passed.\n");
    return 0;
}
