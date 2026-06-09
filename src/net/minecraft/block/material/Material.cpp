#include "net/minecraft/block/material/Material.hpp"

#include "net/minecraft/block/material/AirMaterial.hpp"
#include "net/minecraft/block/material/FluidMaterial.hpp"
#include "net/minecraft/block/material/PortalMaterial.hpp"
#include "net/minecraft/block/material/ReplaceableMaterial.hpp"

namespace net::minecraft::block::material {

namespace {

struct InitFluid : FluidMaterial {
    explicit InitFluid(const MapColor& color) : FluidMaterial(color) {}
    static InitFluid water()
    {
        InitFluid material(MapColor::BLUE);
        material.setDestroyPistonBehavior();
        return material;
    }
    static InitFluid lava()
    {
        InitFluid material(MapColor::RED);
        material.setDestroyPistonBehavior();
        return material;
    }
};

struct InitReplaceable : ReplaceableMaterial {
    explicit InitReplaceable(const MapColor& color) : ReplaceableMaterial(color) {}
    static InitReplaceable plant()
    {
        InitReplaceable material(MapColor::GREEN);
        material.setDestroyPistonBehavior();
        return material;
    }
    static InitReplaceable pistonBreakable()
    {
        InitReplaceable material(MapColor::CLEAR);
        material.setDestroyPistonBehavior();
        return material;
    }
    static InitReplaceable snowLayer()
    {
        InitReplaceable material(MapColor::WHITE);
        material.setReplaceable();
        material.setTransparent();
        material.setHandHarvestable();
        material.setDestroyPistonBehavior();
        return material;
    }
};

struct InitAir : AirMaterial {
    explicit InitAir(const MapColor& color) : AirMaterial(color) {}
    static InitAir fire()
    {
        InitAir material(MapColor::CLEAR);
        material.setDestroyPistonBehavior();
        return material;
    }
};

struct InitPortal : PortalMaterial {
    explicit InitPortal(const MapColor& color) : PortalMaterial(color) {}
    static InitPortal netherPortal()
    {
        InitPortal material(MapColor::CLEAR);
        material.setUnpushablePistonBehavior();
        return material;
    }
};

AirMaterial airMaterial {MapColor::CLEAR};
FluidMaterial waterMaterial = InitFluid::water();
FluidMaterial lavaMaterial = InitFluid::lava();
ReplaceableMaterial plantMaterial = InitReplaceable::plant();
AirMaterial fireMaterial = InitAir::fire();
ReplaceableMaterial pistonBreakableMaterial = InitReplaceable::pistonBreakable();
ReplaceableMaterial snowLayerMaterial = InitReplaceable::snowLayer();
PortalMaterial netherPortalMaterial = InitPortal::netherPortal();

} // namespace

// Mirrors Material.java static fields (beta 1.7.3 MCP).
Material& Material::AIR = airMaterial;
Material Material::SOLID_ORGANIC {MapColor::PALE_GREEN};
Material Material::SOIL {MapColor::ORANGE};
Material Material::WOOD = [] {
    Material material(MapColor::BROWN);
    material.setBurnable();
    return material;
}();
Material Material::STONE = [] {
    Material material(MapColor::GRAY);
    material.setHandHarvestable();
    return material;
}();
Material Material::METAL = [] {
    Material material(MapColor::LIGHT_GRAY2);
    material.setHandHarvestable();
    return material;
}();
Material& Material::WATER = waterMaterial;
Material& Material::LAVA = lavaMaterial;
Material Material::LEAVES = [] {
    Material material(MapColor::GREEN);
    material.setBurnable();
    material.setTransparent();
    material.setDestroyPistonBehavior();
    return material;
}();
Material& Material::PLANT = plantMaterial;
Material Material::SPONGE {MapColor::LIGHT_GRAY};
Material Material::WOOL = [] {
    Material material(MapColor::LIGHT_GRAY);
    material.setBurnable();
    return material;
}();
Material& Material::FIRE = fireMaterial;
Material Material::SAND {MapColor::PALE_YELLOW};
Material& Material::PISTON_BREAKABLE = pistonBreakableMaterial;
Material Material::GLASS = [] {
    Material material(MapColor::CLEAR);
    material.setTransparent();
    return material;
}();
Material Material::TNT = [] {
    Material material(MapColor::RED);
    material.setBurnable();
    material.setTransparent();
    return material;
}();
Material Material::UNUSED = [] {
    Material material(MapColor::GREEN);
    material.setDestroyPistonBehavior();
    return material;
}();
Material Material::ICE = [] {
    Material material(MapColor::LIGHT_BLUE);
    material.setTransparent();
    return material;
}();
Material& Material::SNOW_LAYER = snowLayerMaterial;
Material Material::SNOW_BLOCK = [] {
    Material material(MapColor::WHITE);
    material.setHandHarvestable();
    return material;
}();
Material Material::CACTUS = [] {
    Material material(MapColor::GREEN);
    material.setTransparent();
    material.setDestroyPistonBehavior();
    return material;
}();
Material Material::CLAY {MapColor::SILVER};
Material Material::PUMPKIN = [] {
    Material material(MapColor::GREEN);
    material.setDestroyPistonBehavior();
    return material;
}();
Material& Material::NETHER_PORTAL = netherPortalMaterial;
Material Material::CAKE = [] {
    Material material(MapColor::CLEAR);
    material.setDestroyPistonBehavior();
    return material;
}();
Material Material::COBWEB = [] {
    Material material(MapColor::LIGHT_GRAY);
    material.setHandHarvestable();
    material.setDestroyPistonBehavior();
    return material;
}();
Material Material::PISTON = [] {
    Material material(MapColor::GRAY);
    material.setUnpushablePistonBehavior();
    return material;
}();

} // namespace net::minecraft::block::material
