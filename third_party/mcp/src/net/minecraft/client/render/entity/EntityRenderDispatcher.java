/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.entity;

import java.util.HashMap;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.Block;
import net.minecraft.client.font.TextRenderer;
import net.minecraft.client.option.GameOptions;
import net.minecraft.client.render.entity.ArrowEntityRenderer;
import net.minecraft.client.render.entity.BoatEntityRenderer;
import net.minecraft.client.render.entity.BoxEntityRenderer;
import net.minecraft.client.render.entity.ChickenEntityRenderer;
import net.minecraft.client.render.entity.CowEntityRenderer;
import net.minecraft.client.render.entity.CreeperEntityRenderer;
import net.minecraft.client.render.entity.EntityRenderer;
import net.minecraft.client.render.entity.FallingBlockEntityRenderer;
import net.minecraft.client.render.entity.FireballEntityRenderer;
import net.minecraft.client.render.entity.FishingBobberEntityRenderer;
import net.minecraft.client.render.entity.GhastEntityRenderer;
import net.minecraft.client.render.entity.GiantEntityRenderer;
import net.minecraft.client.render.entity.LightningEntityRenderer;
import net.minecraft.client.render.entity.LivingEntityRenderer;
import net.minecraft.client.render.entity.MinecartEntityRenderer;
import net.minecraft.client.render.entity.PaintingEntityRenderer;
import net.minecraft.client.render.entity.PigEntityRenderer;
import net.minecraft.client.render.entity.PlayerEntityRenderer;
import net.minecraft.client.render.entity.ProjectileEntityRenderer;
import net.minecraft.client.render.entity.SheepEntityRenderer;
import net.minecraft.client.render.entity.SlimeEntityRenderer;
import net.minecraft.client.render.entity.SpiderEntityRenderer;
import net.minecraft.client.render.entity.SquidEntityRenderer;
import net.minecraft.client.render.entity.TntEntityRenderer;
import net.minecraft.client.render.entity.UndeadEntityRenderer;
import net.minecraft.client.render.entity.WolfEntityRenderer;
import net.minecraft.client.render.entity.model.BipedEntityModel;
import net.minecraft.client.render.entity.model.ChickenEntityModel;
import net.minecraft.client.render.entity.model.CowEntityModel;
import net.minecraft.client.render.entity.model.PigEntityModel;
import net.minecraft.client.render.entity.model.SheepEntityModel;
import net.minecraft.client.render.entity.model.SheepWoolEntityModel;
import net.minecraft.client.render.entity.model.SkeletonEntityModel;
import net.minecraft.client.render.entity.model.SlimeEntityModel;
import net.minecraft.client.render.entity.model.SquidEntityModel;
import net.minecraft.client.render.entity.model.WolfEntityModel;
import net.minecraft.client.render.entity.model.ZombieEntityModel;
import net.minecraft.client.render.item.HeldItemRenderer;
import net.minecraft.client.render.item.ItemRenderer;
import net.minecraft.client.texture.TextureManager;
import net.minecraft.entity.Entity;
import net.minecraft.entity.FallingBlockEntity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LightningEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.TntEntity;
import net.minecraft.entity.decoration.painting.PaintingEntity;
import net.minecraft.entity.mob.CreeperEntity;
import net.minecraft.entity.mob.GhastEntity;
import net.minecraft.entity.mob.GiantEntity;
import net.minecraft.entity.mob.SkeletonEntity;
import net.minecraft.entity.mob.SlimeEntity;
import net.minecraft.entity.mob.SpiderEntity;
import net.minecraft.entity.mob.ZombieEntity;
import net.minecraft.entity.passive.ChickenEntity;
import net.minecraft.entity.passive.CowEntity;
import net.minecraft.entity.passive.PigEntity;
import net.minecraft.entity.passive.SheepEntity;
import net.minecraft.entity.passive.SquidEntity;
import net.minecraft.entity.passive.WolfEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.FireballEntity;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.entity.projectile.thrown.EggEntity;
import net.minecraft.entity.projectile.thrown.SnowballEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.item.Item;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class EntityRenderDispatcher {
    private Map renderers = new HashMap();
    public static EntityRenderDispatcher INSTANCE = new EntityRenderDispatcher();
    private TextRenderer textRenderer;
    public static double offsetX;
    public static double offsetY;
    public static double offsetZ;
    public TextureManager textureManager;
    public HeldItemRenderer heldItemRenderer;
    public World world;
    public LivingEntity cameraEntity;
    public float yaw;
    public float pitch;
    public GameOptions options;
    public double x;
    public double y;
    public double z;

    private EntityRenderDispatcher() {
        this.renderers.put(SpiderEntity.class, new SpiderEntityRenderer());
        this.renderers.put(PigEntity.class, new PigEntityRenderer(new PigEntityModel(), new PigEntityModel(0.5f), 0.7f));
        this.renderers.put(SheepEntity.class, new SheepEntityRenderer(new SheepEntityModel(), new SheepWoolEntityModel(), 0.7f));
        this.renderers.put(CowEntity.class, new CowEntityRenderer(new CowEntityModel(), 0.7f));
        this.renderers.put(WolfEntity.class, new WolfEntityRenderer(new WolfEntityModel(), 0.5f));
        this.renderers.put(ChickenEntity.class, new ChickenEntityRenderer(new ChickenEntityModel(), 0.3f));
        this.renderers.put(CreeperEntity.class, new CreeperEntityRenderer());
        this.renderers.put(SkeletonEntity.class, new UndeadEntityRenderer(new SkeletonEntityModel(), 0.5f));
        this.renderers.put(ZombieEntity.class, new UndeadEntityRenderer(new ZombieEntityModel(), 0.5f));
        this.renderers.put(SlimeEntity.class, new SlimeEntityRenderer(new SlimeEntityModel(16), new SlimeEntityModel(0), 0.25f));
        this.renderers.put(PlayerEntity.class, new PlayerEntityRenderer());
        this.renderers.put(GiantEntity.class, new GiantEntityRenderer(new ZombieEntityModel(), 0.5f, 6.0f));
        this.renderers.put(GhastEntity.class, new GhastEntityRenderer());
        this.renderers.put(SquidEntity.class, new SquidEntityRenderer(new SquidEntityModel(), 0.7f));
        this.renderers.put(LivingEntity.class, new LivingEntityRenderer(new BipedEntityModel(), 0.5f));
        this.renderers.put(Entity.class, new BoxEntityRenderer());
        this.renderers.put(PaintingEntity.class, new PaintingEntityRenderer());
        this.renderers.put(ArrowEntity.class, new ArrowEntityRenderer());
        this.renderers.put(SnowballEntity.class, new ProjectileEntityRenderer(Item.SNOWBALL.getTextureId(0)));
        this.renderers.put(EggEntity.class, new ProjectileEntityRenderer(Item.EGG.getTextureId(0)));
        this.renderers.put(FireballEntity.class, new FireballEntityRenderer());
        this.renderers.put(ItemEntity.class, new ItemRenderer());
        this.renderers.put(TntEntity.class, new TntEntityRenderer());
        this.renderers.put(FallingBlockEntity.class, new FallingBlockEntityRenderer());
        this.renderers.put(MinecartEntity.class, new MinecartEntityRenderer());
        this.renderers.put(BoatEntity.class, new BoatEntityRenderer());
        this.renderers.put(FishingBobberEntity.class, new FishingBobberEntityRenderer());
        this.renderers.put(LightningEntity.class, new LightningEntityRenderer());
        for (EntityRenderer entityRenderer : this.renderers.values()) {
            entityRenderer.setDispatcher(this);
        }
    }

    public EntityRenderer get(Class entityClass) {
        EntityRenderer entityRenderer = (EntityRenderer)this.renderers.get(entityClass);
        if (entityRenderer == null && entityClass != Entity.class) {
            entityRenderer = this.get(entityClass.getSuperclass());
            this.renderers.put(entityClass, entityRenderer);
        }
        return entityRenderer;
    }

    public EntityRenderer get(Entity entity) {
        return this.get(entity.getClass());
    }

    public void init(World world, TextureManager textureManager, TextRenderer textRenderer, LivingEntity livingEntity, GameOptions options, float scale) {
        this.world = world;
        this.textureManager = textureManager;
        this.options = options;
        this.cameraEntity = livingEntity;
        this.textRenderer = textRenderer;
        if (livingEntity.isSleeping()) {
            int n = world.getBlockId(MathHelper.floor(livingEntity.x), MathHelper.floor(livingEntity.y), MathHelper.floor(livingEntity.z));
            if (n == Block.BED.id) {
                int n2 = world.getBlockMeta(MathHelper.floor(livingEntity.x), MathHelper.floor(livingEntity.y), MathHelper.floor(livingEntity.z));
                int n3 = n2 & 3;
                this.yaw = n3 * 90 + 180;
                this.pitch = 0.0f;
            }
        } else {
            this.yaw = livingEntity.prevYaw + (livingEntity.yaw - livingEntity.prevYaw) * scale;
            this.pitch = livingEntity.prevPitch + (livingEntity.pitch - livingEntity.prevPitch) * scale;
        }
        this.x = livingEntity.lastTickX + (livingEntity.x - livingEntity.lastTickX) * (double)scale;
        this.y = livingEntity.lastTickY + (livingEntity.y - livingEntity.lastTickY) * (double)scale;
        this.z = livingEntity.lastTickZ + (livingEntity.z - livingEntity.lastTickZ) * (double)scale;
    }

    public void render(Entity entity, float scale) {
        double d = entity.lastTickX + (entity.x - entity.lastTickX) * (double)scale;
        double d2 = entity.lastTickY + (entity.y - entity.lastTickY) * (double)scale;
        double d3 = entity.lastTickZ + (entity.z - entity.lastTickZ) * (double)scale;
        float f = entity.prevYaw + (entity.yaw - entity.prevYaw) * scale;
        float f2 = entity.getBrightnessAtEyes(scale);
        GL11.glColor3f(f2, f2, f2);
        this.render(entity, d - offsetX, d2 - offsetY, d3 - offsetZ, f, scale);
    }

    public void render(Entity entity, double x, double y, double z, float yaw, float pitch) {
        EntityRenderer entityRenderer = this.get(entity);
        if (entityRenderer != null) {
            entityRenderer.render(entity, x, y, z, yaw, pitch);
            entityRenderer.postRender(entity, x, y, z, yaw, pitch);
        }
    }

    public void setWorld(World world) {
        this.world = world;
    }

    public double squaredDistanceTo(double x, double y, double z) {
        double d = x - this.x;
        double d2 = y - this.y;
        double d3 = z - this.z;
        return d * d + d2 * d2 + d3 * d3;
    }

    public TextRenderer getTextRenderer() {
        return this.textRenderer;
    }
}

