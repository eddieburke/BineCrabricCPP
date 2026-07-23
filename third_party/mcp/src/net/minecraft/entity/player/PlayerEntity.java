/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.player;

import java.util.Iterator;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievements;
import net.minecraft.block.BedBlock;
import net.minecraft.block.Block;
import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.block.material.Material;
import net.minecraft.entity.Entity;
import net.minecraft.entity.ItemEntity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.mob.CreeperEntity;
import net.minecraft.entity.mob.GhastEntity;
import net.minecraft.entity.mob.MonsterEntity;
import net.minecraft.entity.passive.PigEntity;
import net.minecraft.entity.passive.WolfEntity;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.entity.player.SleepAttemptResult;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.FishingBobberEntity;
import net.minecraft.entity.vehicle.BoatEntity;
import net.minecraft.entity.vehicle.MinecartEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.nbt.NbtList;
import net.minecraft.screen.PlayerScreenHandler;
import net.minecraft.screen.ScreenHandler;
import net.minecraft.stat.Stat;
import net.minecraft.stat.Stats;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.MathHelper;
import net.minecraft.util.math.Vec3i;
import net.minecraft.world.World;
import net.minecraft.world.chunk.ChunkSource;

public abstract class PlayerEntity
extends LivingEntity {
    public PlayerInventory inventory = new PlayerInventory(this);
    public ScreenHandler playerScreenHandler;
    public ScreenHandler currentScreenHandler;
    public byte unused = 0;
    public int score = 0;
    public float prevStepBobbingAmount;
    public float stepBobbingAmount;
    public boolean handSwinging = false;
    public int handSwingTicks = 0;
    public String name;
    public int dimensionId;
    @Environment(value=EnvType.CLIENT)
    public String playerCapeUrl;
    public double prevCapeX;
    public double prevCapeY;
    public double prevCapeZ;
    public double capeX;
    public double capeY;
    public double capeZ;
    protected boolean sleeping;
    public Vec3i sleepingPos;
    private int sleepTimer;
    public float sleepOffsetX;
    @Environment(value=EnvType.CLIENT)
    public float sleepOffsetY;
    public float sleepOffsetZ;
    private Vec3i spawnPos;
    private Vec3i ridingStartPos;
    public int portalCooldown = 20;
    protected boolean inTeleportationState = false;
    @Environment(value=EnvType.CLIENT)
    public float screenDistortion;
    @Environment(value=EnvType.CLIENT)
    public float lastScreenDistortion;
    @Environment(value=EnvType.SERVER)
    public float changeDimensionCooldown;
    private int damageSpill = 0;
    public FishingBobberEntity fishHook = null;

    public PlayerEntity(World arg) {
        super(arg);
        this.currentScreenHandler = this.playerScreenHandler = new PlayerScreenHandler(this.inventory, !arg.isRemote);
        this.standingEyeHeight = 1.62f;
        Vec3i vec3i = arg.getSpawnPos();
        this.setPositionAndAnglesKeepPrevAngles((double)vec3i.x + 0.5, vec3i.y + 1, (double)vec3i.z + 0.5, 0.0f, 0.0f);
        this.health = 20;
        this.modelName = "humanoid";
        this.rotationOffset = 180.0f;
        this.fireImmunityTicks = 20;
        this.texture = "/mob/char.png";
    }

    protected void initDataTracker() {
        super.initDataTracker();
        this.dataTracker.startTracking(16, (byte)0);
    }

    public void tick() {
        if (this.isSleeping()) {
            ++this.sleepTimer;
            if (this.sleepTimer > 100) {
                this.sleepTimer = 100;
            }
            if (!this.world.isRemote) {
                if (!this.isSleepingInBed()) {
                    this.wakeUp(true, true, false);
                } else if (this.world.canMonsterSpawn()) {
                    this.wakeUp(false, true, true);
                }
            }
        } else if (this.sleepTimer > 0) {
            ++this.sleepTimer;
            if (this.sleepTimer >= 110) {
                this.sleepTimer = 0;
            }
        }
        super.tick();
        if (!this.world.isRemote && this.currentScreenHandler != null && !this.currentScreenHandler.canUse(this)) {
            this.closeHandledScreen();
            this.currentScreenHandler = this.playerScreenHandler;
        }
        this.prevCapeX = this.capeX;
        this.prevCapeY = this.capeY;
        this.prevCapeZ = this.capeZ;
        double d = this.x - this.capeX;
        double d2 = this.y - this.capeY;
        double d3 = this.z - this.capeZ;
        double d4 = 10.0;
        if (d > d4) {
            this.prevCapeX = this.capeX = this.x;
        }
        if (d3 > d4) {
            this.prevCapeZ = this.capeZ = this.z;
        }
        if (d2 > d4) {
            this.prevCapeY = this.capeY = this.y;
        }
        if (d < -d4) {
            this.prevCapeX = this.capeX = this.x;
        }
        if (d3 < -d4) {
            this.prevCapeZ = this.capeZ = this.z;
        }
        if (d2 < -d4) {
            this.prevCapeY = this.capeY = this.y;
        }
        this.capeX += d * 0.25;
        this.capeZ += d3 * 0.25;
        this.capeY += d2 * 0.25;
        this.increaseStat(Stats.PLAY_ONE_MINUTE, 1);
        if (this.vehicle == null) {
            this.ridingStartPos = null;
        }
    }

    protected boolean isImmobile() {
        return this.health <= 0 || this.isSleeping();
    }

    protected void closeHandledScreen() {
        this.currentScreenHandler = this.playerScreenHandler;
    }

    @Environment(value=EnvType.CLIENT)
    public void updateCapeUrl() {
        this.capeUrl = this.playerCapeUrl = "http://s3.amazonaws.com/MinecraftCloaks/" + this.name + ".png";
    }

    public void tickRiding() {
        double d = this.x;
        double d2 = this.y;
        double d3 = this.z;
        super.tickRiding();
        this.prevStepBobbingAmount = this.stepBobbingAmount;
        this.stepBobbingAmount = 0.0f;
        this.increaseRidingMotionStats(this.x - d, this.y - d2, this.z - d3);
    }

    @Environment(value=EnvType.CLIENT)
    public void teleportTop() {
        this.standingEyeHeight = 1.62f;
        this.setBoundingBoxSpacing(0.6f, 1.8f);
        super.teleportTop();
        this.health = 20;
        this.deathTime = 0;
    }

    protected void tickLiving() {
        if (this.handSwinging) {
            ++this.handSwingTicks;
            if (this.handSwingTicks >= 8) {
                this.handSwingTicks = 0;
                this.handSwinging = false;
            }
        } else {
            this.handSwingTicks = 0;
        }
        this.swingAnimationProgress = (float)this.handSwingTicks / 8.0f;
    }

    public void tickMovement() {
        List list;
        if (this.world.difficulty == 0 && this.health < 20 && this.age % 20 * 12 == 0) {
            this.heal(1);
        }
        this.inventory.inventoryTick();
        this.prevStepBobbingAmount = this.stepBobbingAmount;
        super.tickMovement();
        float f = MathHelper.sqrt(this.velocityX * this.velocityX + this.velocityZ * this.velocityZ);
        float f2 = (float)Math.atan(-this.velocityY * (double)0.2f) * 15.0f;
        if (f > 0.1f) {
            f = 0.1f;
        }
        if (!this.onGround || this.health <= 0) {
            f = 0.0f;
        }
        if (this.onGround || this.health <= 0) {
            f2 = 0.0f;
        }
        this.stepBobbingAmount += (f - this.stepBobbingAmount) * 0.4f;
        this.tilt += (f2 - this.tilt) * 0.8f;
        if (this.health > 0 && (list = this.world.getEntities(this, this.boundingBox.expand(1.0, 0.0, 1.0))) != null) {
            for (int i = 0; i < list.size(); ++i) {
                Entity entity = (Entity)list.get(i);
                if (entity.dead) continue;
                this.collideWithEntity(entity);
            }
        }
    }

    private void collideWithEntity(Entity entity) {
        entity.onPlayerInteraction(this);
    }

    @Environment(value=EnvType.CLIENT)
    public int getScore() {
        return this.score;
    }

    public void onKilledBy(Entity adversary) {
        super.onKilledBy(adversary);
        this.setBoundingBoxSpacing(0.2f, 0.2f);
        this.setPosition(this.x, this.y, this.z);
        this.velocityY = 0.1f;
        if (this.name.equals("Notch")) {
            this.dropItem(new ItemStack(Item.APPLE, 1), true);
        }
        this.inventory.dropInventory();
        if (adversary != null) {
            this.velocityX = -MathHelper.cos((this.damagedSwingDir + this.yaw) * (float)Math.PI / 180.0f) * 0.1f;
            this.velocityZ = -MathHelper.sin((this.damagedSwingDir + this.yaw) * (float)Math.PI / 180.0f) * 0.1f;
        } else {
            this.velocityZ = 0.0;
            this.velocityX = 0.0;
        }
        this.standingEyeHeight = 0.1f;
        this.increaseStat(Stats.DEATHS, 1);
    }

    public void updateKilledAchievement(Entity entityKilled, int score) {
        this.score += score;
        if (entityKilled instanceof PlayerEntity) {
            this.increaseStat(Stats.PLAYER_KILLS, 1);
        } else {
            this.increaseStat(Stats.MOB_KILLS, 1);
        }
    }

    public void dropSelectedItem() {
        this.dropItem(this.inventory.removeStack(this.inventory.selectedSlot, 1), false);
    }

    public void dropItem(ItemStack stack) {
        this.dropItem(stack, false);
    }

    public void dropItem(ItemStack stack, boolean throwRandomly) {
        if (stack == null) {
            return;
        }
        ItemEntity itemEntity = new ItemEntity(this.world, this.x, this.y - (double)0.3f + (double)this.getEyeHeight(), this.z, stack);
        itemEntity.pickupDelay = 40;
        float f = 0.1f;
        if (throwRandomly) {
            float f2 = this.random.nextFloat() * 0.5f;
            float f3 = this.random.nextFloat() * (float)Math.PI * 2.0f;
            itemEntity.velocityX = -MathHelper.sin(f3) * f2;
            itemEntity.velocityZ = MathHelper.cos(f3) * f2;
            itemEntity.velocityY = 0.2f;
        } else {
            f = 0.3f;
            itemEntity.velocityX = -MathHelper.sin(this.yaw / 180.0f * (float)Math.PI) * MathHelper.cos(this.pitch / 180.0f * (float)Math.PI) * f;
            itemEntity.velocityZ = MathHelper.cos(this.yaw / 180.0f * (float)Math.PI) * MathHelper.cos(this.pitch / 180.0f * (float)Math.PI) * f;
            itemEntity.velocityY = -MathHelper.sin(this.pitch / 180.0f * (float)Math.PI) * f + 0.1f;
            f = 0.02f;
            float f4 = this.random.nextFloat() * (float)Math.PI * 2.0f;
            itemEntity.velocityX += Math.cos(f4) * (double)(f *= this.random.nextFloat());
            itemEntity.velocityY += (double)((this.random.nextFloat() - this.random.nextFloat()) * 0.1f);
            itemEntity.velocityZ += Math.sin(f4) * (double)f;
        }
        this.spawnItem(itemEntity);
        this.increaseStat(Stats.DROP, 1);
    }

    protected void spawnItem(ItemEntity itemEntity) {
        this.world.spawnEntity(itemEntity);
    }

    public float getBlockBreakingSpeed(Block block) {
        float f = this.inventory.getStrengthOnBlock(block);
        if (this.isInFluid(Material.WATER)) {
            f /= 5.0f;
        }
        if (!this.onGround) {
            f /= 5.0f;
        }
        return f;
    }

    public boolean canHarvest(Block block) {
        return this.inventory.isUsingEffectiveTool(block);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        NbtList nbtList = nbt.getList("Inventory");
        this.inventory.readNbt(nbtList);
        this.dimensionId = nbt.getInt("Dimension");
        this.sleeping = nbt.getBoolean("Sleeping");
        this.sleepTimer = nbt.getShort("SleepTimer");
        if (this.sleeping) {
            this.sleepingPos = new Vec3i(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z));
            this.wakeUp(true, true, false);
        }
        if (nbt.contains("SpawnX") && nbt.contains("SpawnY") && nbt.contains("SpawnZ")) {
            this.spawnPos = new Vec3i(nbt.getInt("SpawnX"), nbt.getInt("SpawnY"), nbt.getInt("SpawnZ"));
        }
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.put("Inventory", this.inventory.writeNbt(new NbtList()));
        nbt.putInt("Dimension", this.dimensionId);
        nbt.putBoolean("Sleeping", this.sleeping);
        nbt.putShort("SleepTimer", (short)this.sleepTimer);
        if (this.spawnPos != null) {
            nbt.putInt("SpawnX", this.spawnPos.x);
            nbt.putInt("SpawnY", this.spawnPos.y);
            nbt.putInt("SpawnZ", this.spawnPos.z);
        }
    }

    public void openChestScreen(Inventory inventory) {
    }

    public void openCraftingScreen(int x, int y, int z) {
    }

    public void sendPickup(Entity item, int count) {
    }

    public float getEyeHeight() {
        return 0.12f;
    }

    protected void resetEyeHeight() {
        this.standingEyeHeight = 1.62f;
    }

    public boolean damage(Entity damageSource, int amount) {
        this.despawnCounter = 0;
        if (this.health <= 0) {
            return false;
        }
        if (this.isSleeping() && !this.world.isRemote) {
            this.wakeUp(true, true, false);
        }
        if (damageSource instanceof MonsterEntity || damageSource instanceof ArrowEntity) {
            if (this.world.difficulty == 0) {
                amount = 0;
            }
            if (this.world.difficulty == 1) {
                amount = amount / 3 + 1;
            }
            if (this.world.difficulty == 3) {
                amount = amount * 3 / 2;
            }
        }
        if (amount == 0) {
            return false;
        }
        Entity entity = damageSource;
        if (entity instanceof ArrowEntity && ((ArrowEntity)entity).owner != null) {
            entity = ((ArrowEntity)entity).owner;
        }
        if (entity instanceof LivingEntity) {
            this.commandWolvesToAttack((LivingEntity)entity, false);
        }
        this.increaseStat(Stats.DAMAGE_TAKEN, amount);
        return super.damage(damageSource, amount);
    }

    protected boolean isPvpEnabled() {
        return false;
    }

    protected void commandWolvesToAttack(LivingEntity entity, boolean sitting) {
        Object object;
        if (entity instanceof CreeperEntity || entity instanceof GhastEntity) {
            return;
        }
        if (entity instanceof WolfEntity && ((WolfEntity)(object = (WolfEntity)entity)).isTamed() && this.name.equals(((WolfEntity)object).getOwnerName())) {
            return;
        }
        if (entity instanceof PlayerEntity && !this.isPvpEnabled()) {
            return;
        }
        object = this.world.collectEntitiesByClass(WolfEntity.class, Box.createCached(this.x, this.y, this.z, this.x + 1.0, this.y + 1.0, this.z + 1.0).expand(16.0, 4.0, 16.0));
        Iterator iterator = object.iterator();
        while (iterator.hasNext()) {
            Entity entity2 = (Entity)iterator.next();
            WolfEntity wolfEntity = (WolfEntity)entity2;
            if (!wolfEntity.isTamed() || wolfEntity.getTarget() != null || !this.name.equals(wolfEntity.getOwnerName()) || sitting && wolfEntity.isInSittingPose()) continue;
            wolfEntity.setSitting(false);
            wolfEntity.setTarget(entity);
        }
    }

    protected void applyDamage(int amount) {
        int n = 25 - this.inventory.getTotalArmorDurability();
        int n2 = amount * n + this.damageSpill;
        this.inventory.damageArmor(amount);
        amount = n2 / 25;
        this.damageSpill = n2 % 25;
        super.applyDamage(amount);
    }

    public void openFurnaceScreen(FurnaceBlockEntity furnace) {
    }

    public void openDispenserScreen(DispenserBlockEntity dispenser) {
    }

    public void openEditSignScreen(SignBlockEntity sign) {
    }

    public void interact(Entity entity) {
        if (entity.interact(this)) {
            return;
        }
        ItemStack itemStack = this.getHand();
        if (itemStack != null && entity instanceof LivingEntity) {
            itemStack.useOnEntity((LivingEntity)entity);
            if (itemStack.count <= 0) {
                itemStack.onRemoved(this);
                this.clearStackInHand();
            }
        }
    }

    public ItemStack getHand() {
        return this.inventory.getSelectedItem();
    }

    public void clearStackInHand() {
        this.inventory.setStack(this.inventory.selectedSlot, null);
    }

    public double getStandingEyeHeight() {
        return this.standingEyeHeight - 0.5f;
    }

    public void swingHand() {
        this.handSwingTicks = -1;
        this.handSwinging = true;
    }

    public void attack(Entity target) {
        int n = this.inventory.getAttackDamage(target);
        if (n > 0) {
            if (this.velocityY < 0.0) {
                ++n;
            }
            target.damage(this, n);
            ItemStack itemStack = this.getHand();
            if (itemStack != null && target instanceof LivingEntity) {
                itemStack.postHit((LivingEntity)target, this);
                if (itemStack.count <= 0) {
                    itemStack.onRemoved(this);
                    this.clearStackInHand();
                }
            }
            if (target instanceof LivingEntity) {
                if (target.isAlive()) {
                    this.commandWolvesToAttack((LivingEntity)target, true);
                }
                this.increaseStat(Stats.DAMAGE_DEALT, n);
            }
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void respawn() {
    }

    @Environment(value=EnvType.CLIENT)
    public abstract void spawn();

    public void onCursorStackChanged(ItemStack stack) {
    }

    public void markDead() {
        super.markDead();
        this.playerScreenHandler.onClosed(this);
        if (this.currentScreenHandler != null) {
            this.currentScreenHandler.onClosed(this);
        }
    }

    public boolean isInsideWall() {
        return !this.sleeping && super.isInsideWall();
    }

    public SleepAttemptResult trySleep(int x, int y, int z) {
        if (!this.world.isRemote) {
            if (this.isSleeping() || !this.isAlive()) {
                return SleepAttemptResult.OTHER_PROBLEM;
            }
            if (this.world.dimension.isNether) {
                return SleepAttemptResult.NOT_POSSIBLE_HERE;
            }
            if (this.world.canMonsterSpawn()) {
                return SleepAttemptResult.NOT_POSSIBLE_NOW;
            }
            if (Math.abs(this.x - (double)x) > 3.0 || Math.abs(this.y - (double)y) > 2.0 || Math.abs(this.z - (double)z) > 3.0) {
                return SleepAttemptResult.TOO_FAR_AWAY;
            }
        }
        this.setBoundingBoxSpacing(0.2f, 0.2f);
        this.standingEyeHeight = 0.2f;
        if (this.world.isPosLoaded(x, y, z)) {
            int n = this.world.getBlockMeta(x, y, z);
            int n2 = BedBlock.getDirection(n);
            float f = 0.5f;
            float f2 = 0.5f;
            switch (n2) {
                case 0: {
                    f2 = 0.9f;
                    break;
                }
                case 2: {
                    f2 = 0.1f;
                    break;
                }
                case 1: {
                    f = 0.1f;
                    break;
                }
                case 3: {
                    f = 0.9f;
                }
            }
            this.calculateSleepOffset(n2);
            this.setPosition((float)x + f, (float)y + 0.9375f, (float)z + f2);
        } else {
            this.setPosition((float)x + 0.5f, (float)y + 0.9375f, (float)z + 0.5f);
        }
        this.sleeping = true;
        this.sleepTimer = 0;
        this.sleepingPos = new Vec3i(x, y, z);
        this.velocityY = 0.0;
        this.velocityZ = 0.0;
        this.velocityX = 0.0;
        if (!this.world.isRemote) {
            this.world.updateSleepingPlayers();
        }
        return SleepAttemptResult.OK;
    }

    private void calculateSleepOffset(int bedDirection) {
        this.sleepOffsetX = 0.0f;
        this.sleepOffsetZ = 0.0f;
        switch (bedDirection) {
            case 0: {
                this.sleepOffsetZ = -1.8f;
                break;
            }
            case 2: {
                this.sleepOffsetZ = 1.8f;
                break;
            }
            case 1: {
                this.sleepOffsetX = 1.8f;
                break;
            }
            case 3: {
                this.sleepOffsetX = -1.8f;
            }
        }
    }

    public void wakeUp(boolean resetSleepTimer, boolean updateSleepingPlayers, boolean setSpawnPos) {
        this.setBoundingBoxSpacing(0.6f, 1.8f);
        this.resetEyeHeight();
        Vec3i vec3i = this.sleepingPos;
        Vec3i vec3i2 = this.sleepingPos;
        if (vec3i != null && this.world.getBlockId(vec3i.x, vec3i.y, vec3i.z) == Block.BED.id) {
            BedBlock.updateState(this.world, vec3i.x, vec3i.y, vec3i.z, false);
            vec3i2 = BedBlock.findWakeUpPosition(this.world, vec3i.x, vec3i.y, vec3i.z, 0);
            if (vec3i2 == null) {
                vec3i2 = new Vec3i(vec3i.x, vec3i.y + 1, vec3i.z);
            }
            this.setPosition((float)vec3i2.x + 0.5f, (float)vec3i2.y + this.standingEyeHeight + 0.1f, (float)vec3i2.z + 0.5f);
        }
        this.sleeping = false;
        if (!this.world.isRemote && updateSleepingPlayers) {
            this.world.updateSleepingPlayers();
        }
        this.sleepTimer = resetSleepTimer ? 0 : 100;
        if (setSpawnPos) {
            this.setSpawnPos(this.sleepingPos);
        }
    }

    private boolean isSleepingInBed() {
        return this.world.getBlockId(this.sleepingPos.x, this.sleepingPos.y, this.sleepingPos.z) == Block.BED.id;
    }

    public static Vec3i findRespawnPosition(World world, Vec3i spawnPos) {
        ChunkSource chunkSource = world.getChunkSource();
        chunkSource.loadChunk(spawnPos.x - 3 >> 4, spawnPos.z - 3 >> 4);
        chunkSource.loadChunk(spawnPos.x + 3 >> 4, spawnPos.z - 3 >> 4);
        chunkSource.loadChunk(spawnPos.x - 3 >> 4, spawnPos.z + 3 >> 4);
        chunkSource.loadChunk(spawnPos.x + 3 >> 4, spawnPos.z + 3 >> 4);
        if (world.getBlockId(spawnPos.x, spawnPos.y, spawnPos.z) != Block.BED.id) {
            return null;
        }
        Vec3i vec3i = BedBlock.findWakeUpPosition(world, spawnPos.x, spawnPos.y, spawnPos.z, 0);
        return vec3i;
    }

    @Environment(value=EnvType.CLIENT)
    public float getSleepingRotation() {
        if (this.sleepingPos != null) {
            int n = this.world.getBlockMeta(this.sleepingPos.x, this.sleepingPos.y, this.sleepingPos.z);
            int n2 = BedBlock.getDirection(n);
            switch (n2) {
                case 0: {
                    return 90.0f;
                }
                case 1: {
                    return 0.0f;
                }
                case 2: {
                    return 270.0f;
                }
                case 3: {
                    return 180.0f;
                }
            }
        }
        return 0.0f;
    }

    public boolean isSleeping() {
        return this.sleeping;
    }

    public boolean isFullyAsleep() {
        return this.sleeping && this.sleepTimer >= 100;
    }

    @Environment(value=EnvType.CLIENT)
    public int getSleepTimer() {
        return this.sleepTimer;
    }

    public void sendMessage(String message) {
    }

    public Vec3i getSpawnPos() {
        return this.spawnPos;
    }

    public void setSpawnPos(Vec3i spawnPos) {
        this.spawnPos = spawnPos != null ? new Vec3i(spawnPos) : null;
    }

    public void incrementStat(Stat stat) {
        this.increaseStat(stat, 1);
    }

    public void increaseStat(Stat stat, int amount) {
    }

    protected void jump() {
        super.jump();
        this.increaseStat(Stats.JUMP, 1);
    }

    public void travel(float x, float z) {
        double d = this.x;
        double d2 = this.y;
        double d3 = this.z;
        super.travel(x, z);
        this.updateMovementStats(this.x - d, this.y - d2, this.z - d3);
    }

    private void updateMovementStats(double x, double y, double z) {
        if (this.vehicle != null) {
            return;
        }
        if (this.isInFluid(Material.WATER)) {
            int n = Math.round(MathHelper.sqrt(x * x + y * y + z * z) * 100.0f);
            if (n > 0) {
                this.increaseStat(Stats.DIVE_ONE_CM, n);
            }
        } else if (this.isSubmergedInWater()) {
            int n = Math.round(MathHelper.sqrt(x * x + z * z) * 100.0f);
            if (n > 0) {
                this.increaseStat(Stats.SWIM_ONE_CM, n);
            }
        } else if (this.isOnLadder()) {
            if (y > 0.0) {
                this.increaseStat(Stats.CLIMB_ONE_CM, (int)Math.round(y * 100.0));
            }
        } else if (this.onGround) {
            int n = Math.round(MathHelper.sqrt(x * x + z * z) * 100.0f);
            if (n > 0) {
                this.increaseStat(Stats.WALK_ONE_CM, n);
            }
        } else {
            int n = Math.round(MathHelper.sqrt(x * x + z * z) * 100.0f);
            if (n > 25) {
                this.increaseStat(Stats.FLY_ONE_CM, n);
            }
        }
    }

    private void increaseRidingMotionStats(double deltaX, double deltaY, double deltaZ) {
        int n;
        if (this.vehicle != null && (n = Math.round(MathHelper.sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ) * 100.0f)) > 0) {
            if (this.vehicle instanceof MinecartEntity) {
                this.increaseStat(Stats.MINECART_ONE_CM, n);
                if (this.ridingStartPos == null) {
                    this.ridingStartPos = new Vec3i(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z));
                } else if (this.ridingStartPos.distanceTo(MathHelper.floor(this.x), MathHelper.floor(this.y), MathHelper.floor(this.z)) >= 1000.0) {
                    this.increaseStat(Achievements.CRAFT_RAIL, 1);
                }
            } else if (this.vehicle instanceof BoatEntity) {
                this.increaseStat(Stats.BOAT_ONE_CM, n);
            } else if (this.vehicle instanceof PigEntity) {
                this.increaseStat(Stats.PIG_ONE_CM, n);
            }
        }
    }

    protected void onLanding(float fallDistance) {
        if (fallDistance >= 2.0f) {
            this.increaseStat(Stats.FALL_ONE_CM, (int)Math.round((double)fallDistance * 100.0));
        }
        super.onLanding(fallDistance);
    }

    public void onKilledOther(LivingEntity other) {
        if (other instanceof MonsterEntity) {
            this.incrementStat(Achievements.KILL_ENEMY);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public int getItemStackTextureId(ItemStack stack) {
        int n = super.getItemStackTextureId(stack);
        if (stack.itemId == Item.FISHING_ROD.id && this.fishHook != null) {
            n = stack.getTextureId() + 16;
        }
        return n;
    }

    public void tickPortalCooldown() {
        if (this.portalCooldown > 0) {
            this.portalCooldown = 10;
            return;
        }
        this.inTeleportationState = true;
    }
}

