/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.screen;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Random;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.screen.MultiplayerScreen;
import net.minecraft.client.gui.screen.Screen;
import net.minecraft.client.gui.screen.option.OptionsScreen;
import net.minecraft.client.gui.screen.pack.PackScreen;
import net.minecraft.client.gui.screen.world.SelectWorldScreen;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.resource.language.TranslationStorage;
import net.minecraft.util.math.MathHelper;
import org.lwjgl.opengl.GL11;

@Environment(value=EnvType.CLIENT)
public class TitleScreen
extends Screen {
    private static final Random RANDOM = new Random();
    private float ticks = 0.0f;
    private String splashText = "missingno";
    private ButtonWidget multiplayerButton;

    public TitleScreen() {
        try {
            ArrayList<String> arrayList = new ArrayList<String>();
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(TitleScreen.class.getResourceAsStream("/title/splashes.txt"), Charset.forName("UTF-8")));
            String string = "";
            while ((string = bufferedReader.readLine()) != null) {
                if ((string = string.trim()).length() <= 0) continue;
                arrayList.add(string);
            }
            this.splashText = (String)arrayList.get(RANDOM.nextInt(arrayList.size()));
        }
        catch (Exception exception) {
            // empty catch block
        }
    }

    public void tick() {
        this.ticks += 1.0f;
    }

    protected void keyPressed(char character, int keyCode) {
    }

    public void init() {
        Calendar calendar = Calendar.getInstance();
        calendar.setTime(new Date());
        if (calendar.get(2) + 1 == 11 && calendar.get(5) == 9) {
            this.splashText = "Happy birthday, ez!";
        } else if (calendar.get(2) + 1 == 6 && calendar.get(5) == 1) {
            this.splashText = "Happy birthday, Notch!";
        } else if (calendar.get(2) + 1 == 12 && calendar.get(5) == 24) {
            this.splashText = "Merry X-mas!";
        } else if (calendar.get(2) + 1 == 1 && calendar.get(5) == 1) {
            this.splashText = "Happy new year!";
        }
        TranslationStorage translationStorage = TranslationStorage.getInstance();
        int n = this.height / 4 + 48;
        this.buttons.add(new ButtonWidget(1, this.width / 2 - 100, n, translationStorage.get("menu.singleplayer")));
        this.multiplayerButton = new ButtonWidget(2, this.width / 2 - 100, n + 24, translationStorage.get("menu.multiplayer"));
        this.buttons.add(this.multiplayerButton);
        this.buttons.add(new ButtonWidget(3, this.width / 2 - 100, n + 48, translationStorage.get("menu.mods")));
        if (this.minecraft.isApplet) {
            this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, n + 72, translationStorage.get("menu.options")));
        } else {
            this.buttons.add(new ButtonWidget(0, this.width / 2 - 100, n + 72 + 12, 98, 20, translationStorage.get("menu.options")));
            this.buttons.add(new ButtonWidget(4, this.width / 2 + 2, n + 72 + 12, 98, 20, translationStorage.get("menu.quit")));
        }
        if (this.minecraft.session == null) {
            this.multiplayerButton.active = false;
        }
    }

    protected void buttonClicked(ButtonWidget button) {
        if (button.id == 0) {
            this.minecraft.setScreen(new OptionsScreen(this, this.minecraft.options));
        }
        if (button.id == 1) {
            this.minecraft.setScreen(new SelectWorldScreen(this));
        }
        if (button.id == 2) {
            this.minecraft.setScreen(new MultiplayerScreen(this));
        }
        if (button.id == 3) {
            this.minecraft.setScreen(new PackScreen(this));
        }
        if (button.id == 4) {
            this.minecraft.scheduleStop();
        }
    }

    public void render(int mouseX, int mouseY, float delta) {
        this.renderBackground();
        int n = 274;
        int n2 = this.width / 2 - n / 2;
        int n3 = 30;
        GL11.glBindTexture(3553, this.minecraft.textureManager.getTextureId("/title/mclogo.png"));
        GL11.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        this.drawTexture(n2 + 0, n3 + 0, 0, 0, 155, 44);
        this.drawTexture(n2 + 155, n3 + 0, 0, 45, 155, 44);
        GL11.glPushMatrix();
        GL11.glTranslatef(this.width / 2 + 90, 70.0f, 0.0f);
        GL11.glRotatef(-20.0f, 0.0f, 0.0f, 1.0f);
        float f = 1.8f - MathHelper.abs(MathHelper.sin((float)(System.currentTimeMillis() % 1000L) / 1000.0f * (float)Math.PI * 2.0f) * 0.1f);
        f = f * 100.0f / (float)(this.textRenderer.getWidth(this.splashText) + 32);
        GL11.glScalef(f, f, f);
        this.drawCenteredTextWithShadow(this.textRenderer, this.splashText, 0, -8, 0xFFFF00);
        GL11.glPopMatrix();
        this.drawTextWithShadow(this.textRenderer, "Minecraft Beta 1.7.3", 2, 2, 0x505050);
        String string = "Copyright Mojang AB. Do not distribute.";
        this.drawTextWithShadow(this.textRenderer, string, this.width - this.textRenderer.getWidth(string) - 2, this.height - 10, 0xFFFFFF);
        super.render(mouseX, mouseY, delta);
    }
}

