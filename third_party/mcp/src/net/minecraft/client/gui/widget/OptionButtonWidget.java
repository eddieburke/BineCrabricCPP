/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.gui.widget;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.gui.widget.ButtonWidget;
import net.minecraft.client.option.Option;

@Environment(value=EnvType.CLIENT)
public class OptionButtonWidget
extends ButtonWidget {
    private final Option option;

    public OptionButtonWidget(int i, int j, int k, String string) {
        this(i, j, k, null, string);
    }

    public OptionButtonWidget(int i, int j, int k, int l, int m, String string) {
        super(i, j, k, l, m, string);
        this.option = null;
    }

    public OptionButtonWidget(int id, int x, int y, Option option, String text) {
        super(id, x, y, 150, 20, text);
        this.option = option;
    }

    public Option getOption() {
        return this.option;
    }
}

