/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import java.util.ArrayList;
import java.util.List;
import net.minecraft.inventory.CraftingInventory;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.CraftingRecipe;

public class ShapelessRecipe
implements CraftingRecipe {
    private final ItemStack output;
    private final List input;

    public ShapelessRecipe(ItemStack output, List input) {
        this.output = output;
        this.input = input;
    }

    public ItemStack getOutput() {
        return this.output;
    }

    public boolean matches(CraftingInventory craftingInventory) {
        ArrayList arrayList = new ArrayList(this.input);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                ItemStack itemStack = craftingInventory.getStack(j, i);
                if (itemStack == null) continue;
                boolean bl = false;
                for (ItemStack itemStack2 : arrayList) {
                    if (itemStack.itemId != itemStack2.itemId || itemStack2.getDamage() != -1 && itemStack.getDamage() != itemStack2.getDamage()) continue;
                    bl = true;
                    arrayList.remove(itemStack2);
                    break;
                }
                if (bl) continue;
                return false;
            }
        }
        return arrayList.isEmpty();
    }

    public ItemStack craft(CraftingInventory craftingInventory) {
        return this.output.copy();
    }

    public int getSize() {
        return this.input.size();
    }
}

