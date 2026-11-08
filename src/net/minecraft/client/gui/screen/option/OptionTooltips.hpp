#pragma once
#include <array>
#include <string_view>
namespace net::minecraft::client::gui::screen::option {
struct OptionTooltip {
 std::string_view key;
 std::string_view text;
};
inline constexpr std::array kOptionTooltips{
    OptionTooltip{"music", "Sets the volume of music playback."},
    OptionTooltip{"sound", "Sets the volume of game sound effects."},
    OptionTooltip{"invertYMouse", "Reverses vertical mouse movement."},
    OptionTooltip{"mouseSensitivity", "Changes how quickly the view turns with the mouse."},
    OptionTooltip{"difficulty", "Changes the world's difficulty and related gameplay rules."},
    OptionTooltip{"fov", "Changes the camera field of view. Higher values show more of the world."},
    OptionTooltip{"bobView", "Adds a walking motion to the first-person view."},
    OptionTooltip{"guiScale", "Changes the size of menus and on-screen text."},
    OptionTooltip{"fancyGraphics", "Uses more detailed graphics effects, including transparent leaves."},
    OptionTooltip{"viewDistance", "Sets how many chunks around you are rendered."},
    OptionTooltip{"renderScale", "Scales the rendered scene before it is displayed. Lower values can improve performance."},
    OptionTooltip{"mipmapLevel", "Sets the level of distant texture smoothing. Higher levels reduce shimmering."},
    OptionTooltip{"mipmapLinear", "Uses smoother linear filtering for mipmapped textures."},
    OptionTooltip{"aoLevel", "Controls ambient occlusion shading in corners and where surfaces meet."},
    OptionTooltip{"ao", "Enables or disables ambient occlusion shading."},
    OptionTooltip{"brightness", "Adjusts the brightness of the world image."},
    OptionTooltip{"clearWater", "Makes water more transparent so the underwater view is clearer."},
    OptionTooltip{"backgroundThreads", "Sets how many worker threads process background game tasks. Auto uses the available compute budget."},
    OptionTooltip{"fpsLimit", "Caps the game's frame rate to the selected performance target."},
    OptionTooltip{"smoothFps", "Uses adaptive VSync when the frame rate limit is Balanced or Power Saver."},
    OptionTooltip{"smoothInput", "Smooths mouse-look changes over time instead of applying each movement immediately."},
    OptionTooltip{"entityShadows", "Draws shadows beneath entities."},
    OptionTooltip{"chunkUpdates", "Limits how many chunk rebuilds can be processed per frame."},
    OptionTooltip{"chunkUpdatesDynamic", "Adapts the chunk rebuild limit to frame time."},
    OptionTooltip{"preloadedChunks", "Expands the nearby chunk residency margin to keep more chunks ready around the render distance."},
    OptionTooltip{"entityDistanceScale", "Scales the distance at which entities are rendered."},
    OptionTooltip{"frustumCulling", "Skips terrain sections outside the camera's visible view."},
    OptionTooltip{"distantTerrain", "Enables native terrain LOD beyond the full-detail chunk range."},
    OptionTooltip{"lodNativeRadius", "Sets the radius kept at full chunk detail before distant terrain LOD begins."},
    OptionTooltip{"lodQuality", "Chooses the detail and processing cost of distant terrain meshes."},
    OptionTooltip{"lodMemoryMiB", "Sets the memory budget used by distant terrain data."},
    OptionTooltip{"clouds", "Chooses cloud detail or turns clouds off."},
    OptionTooltip{"cloudsHeight", "Sets cloud height as a percentage of the world's vertical range."},
    OptionTooltip{"trees", "Chooses detailed or fast rendering for tree leaves."},
    OptionTooltip{"grass", "Chooses the detail mode used to render grass and foliage."},
    OptionTooltip{"water", "Chooses water rendering detail or turns water rendering off."},
    OptionTooltip{"rain", "Chooses rain and snow rendering detail or turns weather particles off."},
    OptionTooltip{"sky", "Shows or hides the sky."},
    OptionTooltip{"stars", "Shows or hides stars in the night sky."},
    OptionTooltip{"vignette", "Adds darkening around the edges of the screen."},
    OptionTooltip{"underwaterOverlay", "Shows or hides the underwater tint overlay."},
    OptionTooltip{"animatedWater", "Shows animated water textures."},
    OptionTooltip{"animatedLava", "Shows animated lava textures."},
    OptionTooltip{"animatedFire", "Shows animated fire textures."},
    OptionTooltip{"animatedPortal", "Shows animated portal textures."},
    OptionTooltip{"animatedRedstone", "Shows animated redstone textures."},
    OptionTooltip{"animatedExplosion", "Shows animated explosion textures."},
    OptionTooltip{"animatedFlame", "Shows animated flame textures."},
    OptionTooltip{"animatedSmoke", "Shows animated smoke textures."},
    OptionTooltip{"weather", "Enables or disables world weather effects."},
    OptionTooltip{"time", "Chooses the world's clock mode. A mod that owns the celestial clock can override this control."},
    OptionTooltip{"autoSaveTicks", "Sets how often the world is autosaved."},
    OptionTooltip{"fastDebugInfo", "Uses a faster, reduced debug overlay update."},
};
[[nodiscard]] inline std::string_view optionTooltip(std::string_view key) {
 for(const OptionTooltip& tooltip : kOptionTooltips) {
  if(tooltip.key == key) {
   return tooltip.text;
  }
 }
 return {};
}
}
