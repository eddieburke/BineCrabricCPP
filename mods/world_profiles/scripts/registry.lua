local Profiles = require("scripts.profiles")

return Profiles.new({
  require("scripts.worldtypes.default"),
  require("scripts.worldtypes.flatlands"),
  require("scripts.worldtypes.highlands"),
  require("scripts.worldtypes.caves"),
  require("scripts.worldtypes.alpha"),
  require("scripts.worldtypes.modern"),
  require("scripts.worldtypes.infdev_20100227"),
  (require("scripts.worldtypes.infdev_20100415")),
}, minecraft.world.dimensions())
