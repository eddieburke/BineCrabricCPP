/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.entity.player;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.achievement.Achievement;
import net.minecraft.achievement.Achievements;
import net.minecraft.block.entity.DispenserBlockEntity;
import net.minecraft.block.entity.FurnaceBlockEntity;
import net.minecraft.block.entity.SignBlockEntity;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screen.ingame.CraftingScreen;
import net.minecraft.client.gui.screen.ingame.DispenserScreen;
import net.minecraft.client.gui.screen.ingame.DoubleChestScreen;
import net.minecraft.client.gui.screen.ingame.FurnaceScreen;
import net.minecraft.client.gui.screen.ingame.SignEditScreen;
import net.minecraft.client.input.Input;
import net.minecraft.client.particle.PickupParticle;
import net.minecraft.client.util.Session;
import net.minecraft.client.util.SmoothUtil;
import net.minecraft.entity.Entity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.inventory.Inventory;
import net.minecraft.nbt.NbtCompound;
import net.minecraft.stat.Stat;
import net.minecraft.util.math.MathHelper;
import net.minecraft.world.World;

@Environment(value=EnvType.CLIENT)
public class ClientPlayerEntity
extends PlayerEntity {
    public Input input;
    protected Minecraft minecraft;
    private SmoothUtil sidewaysSmoother = new SmoothUtil();
    private SmoothUtil forwardSmoother = new SmoothUtil();
    private SmoothUtil verticalSmoother = new SmoothUtil();

    public ClientPlayerEntity(Minecraft minecraft, World world, Session session, int dimensionId) {
        super(world);
        this.minecraft = minecraft;
        this.dimensionId = dimensionId;
        if (session != null && session.username != null && session.username.length() > 0) {
            this.skinUrl = "http://s3.amazonaws.com/MinecraftSkins/" + session.username + ".png";
        }
        this.name = session.username;
    }

    public void move(double dx, double dy, double dz) {
        super.move(dx, dy, dz);
    }

    public void tickLiving() {
        super.tickLiving();
        this.sidewaysSpeed = this.input.movementSideways;
        this.forwardSpeed = this.input.movementForward;
        this.jumping = this.input.jumping;
    }

    public void tickMovement() {
        if (!this.minecraft.stats.hasAchievement(Achievements.OPEN_INVENTORY)) {
            this.minecraft.toast.setTutorial(Achievements.OPEN_INVENTORY);
        }
        this.lastScreenDistortion = this.screenDistortion;
        if (this.inTeleportationState) {
            if (!this.world.isRemote && this.vehicle != null) {
                this.setVehicle(null);
            }
            if (this.minecraft.currentScreen != null) {
                this.minecraft.setScreen(null);
            }
            if (this.screenDistortion == 0.0f) {
                this.minecraft.soundManager.playSound("portal.trigger", 1.0f, this.random.nextFloat() * 0.4f + 0.8f);
            }
            this.screenDistortion += 0.0125f;
            if (this.screenDistortion >= 1.0f) {
                this.screenDistortion = 1.0f;
                if (!this.world.isRemote) {
                    this.portalCooldown = 10;
                    this.minecraft.soundManager.playSound("portal.travel", 1.0f, this.random.nextFloat() * 0.4f + 0.8f);
                    this.minecraft.changeDimension();
                }
            }
            this.inTeleportationState = false;
        } else {
            if (this.screenDistortion > 0.0f) {
                this.screenDistortion -= 0.05f;
            }
            if (this.screenDistortion < 0.0f) {
                this.screenDistortion = 0.0f;
            }
        }
        if (this.portalCooldown > 0) {
            --this.portalCooldown;
        }
        this.input.update(this);
        if (this.input.sneaking && this.cameraOffset < 0.2f) {
            this.cameraOffset = 0.2f;
        }
        this.pushOutOfBlock(this.x - (double)this.width * 0.35, this.boundingBox.minY + 0.5, this.z + (double)this.width * 0.35);
        this.pushOutOfBlock(this.x - (double)this.width * 0.35, this.boundingBox.minY + 0.5, this.z - (double)this.width * 0.35);
        this.pushOutOfBlock(this.x + (double)this.width * 0.35, this.boundingBox.minY + 0.5, this.z - (double)this.width * 0.35);
        this.pushOutOfBlock(this.x + (double)this.width * 0.35, this.boundingBox.minY + 0.5, this.z + (double)this.width * 0.35);
        super.tickMovement();
    }

    public void releaseAllKeys() {
        this.input.reset();
    }

    public void updateKey(int key, boolean state) {
        this.input.updateKey(key, state);
    }

    public void writeNbt(NbtCompound nbt) {
        super.writeNbt(nbt);
        nbt.putInt("Score", this.score);
    }

    public void readNbt(NbtCompound nbt) {
        super.readNbt(nbt);
        this.score = nbt.getInt("Score");
    }

    public void closeHandledScreen() {
        super.closeHandledScreen();
        this.minecraft.setScreen(null);
    }

    public void openEditSignScreen(SignBlockEntity sign) {
        this.minecraft.setScreen(new SignEditScreen(sign));
    }

    public void openChestScreen(Inventory inventory) {
        this.minecraft.setScreen(new DoubleChestScreen(this.inventory, inventory));
    }

    public void openCraftingScreen(int x, int y, int z) {
        this.minecraft.setScreen(new CraftingScreen(this.inventory, this.world, x, y, z));
    }

    public void openFurnaceScreen(FurnaceBlockEntity furnace) {
        this.minecraft.setScreen(new FurnaceScreen(this.inventory, furnace));
    }

    public void openDispenserScreen(DispenserBlockEntity dispenser) {
        this.minecraft.setScreen(new DispenserScreen(this.inventory, dispenser));
    }

    public void sendPickup(Entity item, int count) {
        this.minecraft.particleManager.addParticle(new PickupParticle(this.minecraft.world, item, this, -0.5f));
    }

    public int getTotalArmorDurability() {
        return this.inventory.getTotalArmorDurability();
    }

    public void sendChatMessage(String message) {
    }

    public boolean isSneaking() {
        return this.input.sneaking && !this.sleeping;
    }

    public void damageTo(int health) {
        int n = this.health - health;
        if (n <= 0) {
            this.health = health;
            if (n < 0) {
                this.hearts = this.maxHealth / 2;
            }
        } else {
            this.prevHealth = n;
            this.lastHealth = this.health;
            this.hearts = this.maxHealth;
            this.applyDamage(n);
            this.damagedTime = 10;
            this.hurtTime = 10;
        }
    }

    public void respawn() {
        this.minecraft.respawnPlayer(false, 0);
    }

    public void spawn() {
    }

    public void sendMessage(String message) {
        this.minecraft.inGameHud.addTranslatedChatMessage(message);
    }

    public void increaseStat(Stat stat, int amount) {
        if (stat == null) {
            return;
        }
        if (stat.isAchievement()) {
            Achievement achievement = (Achievement)stat;
            if (achievement.parent == null || this.minecraft.stats.hasAchievement(achievement.parent)) {
                if (!this.minecraft.stats.hasAchievement(achievement)) {
                    this.minecraft.toast.set(achievement);
                }
                this.minecraft.stats.increment(stat, amount);
            }
        } else {
            this.minecraft.stats.increment(stat, amount);
        }
    }

    private boolean shouldSuffocate(int x, int y, int z) {
        return this.world.shouldSuffocate(x, y, z);
    }

    protected boolean pushOutOfBlock(double x, double y, double z) {
        int n = MathHelper.floor(x);
        int n2 = MathHelper.floor(y);
        int n3 = MathHelper.floor(z);
        double d = x - (double)n;
        double d2 = z - (double)n3;
        if (this.shouldSuffocate(n, n2, n3) || this.shouldSuffocate(n, n2 + 1, n3)) {
            boolean bl = !this.shouldSuffocate(n - 1, n2, n3) && !this.shouldSuffocate(n - 1, n2 + 1, n3);
            boolean bl2 = !this.shouldSuffocate(n + 1, n2, n3) && !this.shouldSuffocate(n + 1, n2 + 1, n3);
            boolean bl3 = !this.shouldSuffocate(n, n2, n3 - 1) && !this.shouldSuffocate(n, n2 + 1, n3 - 1);
            boolean bl4 = !this.shouldSuffocate(n, n2, n3 + 1) && !this.shouldSuffocate(n, n2 + 1, n3 + 1);
            int n4 = -1;
            double d3 = 9999.0;
            if (bl && d < d3) {
                d3 = d;
                n4 = 0;
            }
            if (bl2 && 1.0 - d < d3) {
                d3 = 1.0 - d;
                n4 = 1;
            }
            if (bl3 && d2 < d3) {
                d3 = d2;
                n4 = 4;
            }
            if (bl4 && 1.0 - d2 < d3) {
                d3 = 1.0 - d2;
                n4 = 5;
            }
            float f = 0.1f;
            if (n4 == 0) {
                this.velocityX = -f;
            }
            if (n4 == 1) {
                this.velocityX = f;
            }
            if (n4 == 4) {
                this.velocityZ = -f;
            }
            if (n4 == 5) {
                this.velocityZ = f;
            }
        }
        return false;
    }
}

