/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render.block.entity;

import java.util.HashMap;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.block.entity.MobSpawnerBlockEntity;
import net.minecraft.client.render.block.entity.BlockEntityRenderer;
import net.minecraft.client.render.entity.EntityRenderDispatcher;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityRegistry;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class MobSpawnerBlockEntityRenderer
extends BlockEntityRenderer {
    private Map models = new HashMap();

    public void render(MobSpawnerBlockEntity arg, double d, double e, double f, float g) {
        GL11.glPushMatrix();
        GL11.glTranslatef((float)d + 0.5f, (float)e, (float)f + 0.5f);
        Entity entity = (Entity)this.models.get(arg.getSpawnedEntityId());
        if (entity == null) {
            entity = EntityRegistry.create(arg.getSpawnedEntityId(), null);
            this.models.put(arg.getSpawnedEntityId(), entity);
        }
        if (entity != null) {
            entity.setWorld(arg.world);
            float f2 = 0.4375f;
            GL11.glTranslatef(0.0f, 0.4f, 0.0f);
            GL11.glRotatef((float)(arg.lastRotation + (arg.rotation - arg.lastRotation) * (double)g) * 10.0f, 0.0f, 1.0f, 0.0f);
            GL11.glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
            GL11.glTranslatef(0.0f, -0.4f, 0.0f);
            GL11.glScalef(f2, f2, f2);
            entity.setPositionAndAnglesKeepPrevAngles(d, e, f, 0.0f, 0.0f);
            EntityRenderDispatcher.INSTANCE.render(entity, 0.0, 0.0, 0.0, 0.0f, g);
        }
        GL11.glPopMatrix();
    }
}

