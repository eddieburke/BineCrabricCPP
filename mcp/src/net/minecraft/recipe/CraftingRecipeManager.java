/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 */
package net.minecraft.recipe;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import net.minecraft.block.Block;
import net.minecraft.inventory.CraftingInventory;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.recipe.ArmorRecipes;
import net.minecraft.recipe.BasicBlockRecipes;
import net.minecraft.recipe.CraftingRecipe;
import net.minecraft.recipe.DyeRecipes;
import net.minecraft.recipe.FoodRecipes;
import net.minecraft.recipe.ShapedRecipe;
import net.minecraft.recipe.ShapelessRecipe;
import net.minecraft.recipe.StorageBlockRecipes;
import net.minecraft.recipe.ToolRecipes;
import net.minecraft.recipe.WeaponRecipes;

public class CraftingRecipeManager {
    private static final CraftingRecipeManager INSTANCE = new CraftingRecipeManager();
    private List recipes = new ArrayList();

    public static final CraftingRecipeManager getInstance() {
        return INSTANCE;
    }

    private CraftingRecipeManager() {
        new ToolRecipes().add(this);
        new WeaponRecipes().add(this);
        new StorageBlockRecipes().add(this);
        new FoodRecipes().add(this);
        new BasicBlockRecipes().add(this);
        new ArmorRecipes().add(this);
        new DyeRecipes().add(this);
        this.addShapedRecipe(new ItemStack(Item.PAPER, 3), "###", Character.valueOf('#'), Item.SUGAR_CANE);
        this.addShapedRecipe(new ItemStack(Item.BOOK, 1), "#", "#", "#", Character.valueOf('#'), Item.PAPER);
        this.addShapedRecipe(new ItemStack(Block.FENCE, 2), "###", "###", Character.valueOf('#'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Block.JUKEBOX, 1), "###", "#X#", "###", Character.valueOf('#'), Block.PLANKS, Character.valueOf('X'), Item.DIAMOND);
        this.addShapedRecipe(new ItemStack(Block.NOTE_BLOCK, 1), "###", "#X#", "###", Character.valueOf('#'), Block.PLANKS, Character.valueOf('X'), Item.REDSTONE);
        this.addShapedRecipe(new ItemStack(Block.BOOKSHELF, 1), "###", "XXX", "###", Character.valueOf('#'), Block.PLANKS, Character.valueOf('X'), Item.BOOK);
        this.addShapedRecipe(new ItemStack(Block.SNOW_BLOCK, 1), "##", "##", Character.valueOf('#'), Item.SNOWBALL);
        this.addShapedRecipe(new ItemStack(Block.CLAY, 1), "##", "##", Character.valueOf('#'), Item.CLAY);
        this.addShapedRecipe(new ItemStack(Block.BRICKS, 1), "##", "##", Character.valueOf('#'), Item.BRICK);
        this.addShapedRecipe(new ItemStack(Block.GLOWSTONE, 1), "##", "##", Character.valueOf('#'), Item.GLOWSTONE_DUST);
        this.addShapedRecipe(new ItemStack(Block.WOOL, 1), "##", "##", Character.valueOf('#'), Item.STRING);
        this.addShapedRecipe(new ItemStack(Block.TNT, 1), "X#X", "#X#", "X#X", Character.valueOf('X'), Item.GUNPOWDER, Character.valueOf('#'), Block.SAND);
        this.addShapedRecipe(new ItemStack(Block.SLAB, 3, 3), "###", Character.valueOf('#'), Block.COBBLESTONE);
        this.addShapedRecipe(new ItemStack(Block.SLAB, 3, 0), "###", Character.valueOf('#'), Block.STONE);
        this.addShapedRecipe(new ItemStack(Block.SLAB, 3, 1), "###", Character.valueOf('#'), Block.SANDSTONE);
        this.addShapedRecipe(new ItemStack(Block.SLAB, 3, 2), "###", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Block.LADDER, 2), "# #", "###", "# #", Character.valueOf('#'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Item.WOODEN_DOOR, 1), "##", "##", "##", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Block.TRAPDOOR, 2), "###", "###", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Item.IRON_DOOR, 1), "##", "##", "##", Character.valueOf('#'), Item.IRON_INGOT);
        this.addShapedRecipe(new ItemStack(Item.SIGN, 1), "###", "###", " X ", Character.valueOf('#'), Block.PLANKS, Character.valueOf('X'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Item.CAKE, 1), "AAA", "BEB", "CCC", Character.valueOf('A'), Item.MILK_BUCKET, Character.valueOf('B'), Item.SUGAR, Character.valueOf('C'), Item.WHEAT, Character.valueOf('E'), Item.EGG);
        this.addShapedRecipe(new ItemStack(Item.SUGAR, 1), "#", Character.valueOf('#'), Item.SUGAR_CANE);
        this.addShapedRecipe(new ItemStack(Block.PLANKS, 4), "#", Character.valueOf('#'), Block.LOG);
        this.addShapedRecipe(new ItemStack(Item.STICK, 4), "#", "#", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Block.TORCH, 4), "X", "#", Character.valueOf('X'), Item.COAL, Character.valueOf('#'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Block.TORCH, 4), "X", "#", Character.valueOf('X'), new ItemStack(Item.COAL, 1, 1), Character.valueOf('#'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Item.BOWL, 4), "# #", " # ", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Block.RAIL, 16), "X X", "X#X", "X X", Character.valueOf('X'), Item.IRON_INGOT, Character.valueOf('#'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Block.POWERED_RAIL, 6), "X X", "X#X", "XRX", Character.valueOf('X'), Item.GOLD_INGOT, Character.valueOf('R'), Item.REDSTONE, Character.valueOf('#'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Block.DETECTOR_RAIL, 6), "X X", "X#X", "XRX", Character.valueOf('X'), Item.IRON_INGOT, Character.valueOf('R'), Item.REDSTONE, Character.valueOf('#'), Block.STONE_PRESSURE_PLATE);
        this.addShapedRecipe(new ItemStack(Item.MINECART, 1), "# #", "###", Character.valueOf('#'), Item.IRON_INGOT);
        this.addShapedRecipe(new ItemStack(Block.JACK_O_LANTERN, 1), "A", "B", Character.valueOf('A'), Block.PUMPKIN, Character.valueOf('B'), Block.TORCH);
        this.addShapedRecipe(new ItemStack(Item.CHEST_MINECART, 1), "A", "B", Character.valueOf('A'), Block.CHEST, Character.valueOf('B'), Item.MINECART);
        this.addShapedRecipe(new ItemStack(Item.FURNACE_MINECART, 1), "A", "B", Character.valueOf('A'), Block.FURNACE, Character.valueOf('B'), Item.MINECART);
        this.addShapedRecipe(new ItemStack(Item.BOAT, 1), "# #", "###", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Item.BUCKET, 1), "# #", " # ", Character.valueOf('#'), Item.IRON_INGOT);
        this.addShapedRecipe(new ItemStack(Item.FLINT_AND_STEEL, 1), "A ", " B", Character.valueOf('A'), Item.IRON_INGOT, Character.valueOf('B'), Item.FLINT);
        this.addShapedRecipe(new ItemStack(Item.BREAD, 1), "###", Character.valueOf('#'), Item.WHEAT);
        this.addShapedRecipe(new ItemStack(Block.WOODEN_STAIRS, 4), "#  ", "## ", "###", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Item.FISHING_ROD, 1), "  #", " #X", "# X", Character.valueOf('#'), Item.STICK, Character.valueOf('X'), Item.STRING);
        this.addShapedRecipe(new ItemStack(Block.COBBLESTONE_STAIRS, 4), "#  ", "## ", "###", Character.valueOf('#'), Block.COBBLESTONE);
        this.addShapedRecipe(new ItemStack(Item.PAINTING, 1), "###", "#X#", "###", Character.valueOf('#'), Item.STICK, Character.valueOf('X'), Block.WOOL);
        this.addShapedRecipe(new ItemStack(Item.GOLDEN_APPLE, 1), "###", "#X#", "###", Character.valueOf('#'), Block.GOLD_BLOCK, Character.valueOf('X'), Item.APPLE);
        this.addShapedRecipe(new ItemStack(Block.LEVER, 1), "X", "#", Character.valueOf('#'), Block.COBBLESTONE, Character.valueOf('X'), Item.STICK);
        this.addShapedRecipe(new ItemStack(Block.LIT_REDSTONE_TORCH, 1), "X", "#", Character.valueOf('#'), Item.STICK, Character.valueOf('X'), Item.REDSTONE);
        this.addShapedRecipe(new ItemStack(Item.REPEATER, 1), "#X#", "III", Character.valueOf('#'), Block.LIT_REDSTONE_TORCH, Character.valueOf('X'), Item.REDSTONE, Character.valueOf('I'), Block.STONE);
        this.addShapedRecipe(new ItemStack(Item.CLOCK, 1), " # ", "#X#", " # ", Character.valueOf('#'), Item.GOLD_INGOT, Character.valueOf('X'), Item.REDSTONE);
        this.addShapedRecipe(new ItemStack(Item.COMPASS, 1), " # ", "#X#", " # ", Character.valueOf('#'), Item.IRON_INGOT, Character.valueOf('X'), Item.REDSTONE);
        this.addShapedRecipe(new ItemStack(Item.MAP, 1), "###", "#X#", "###", Character.valueOf('#'), Item.PAPER, Character.valueOf('X'), Item.COMPASS);
        this.addShapedRecipe(new ItemStack(Block.BUTTON, 1), "#", "#", Character.valueOf('#'), Block.STONE);
        this.addShapedRecipe(new ItemStack(Block.STONE_PRESSURE_PLATE, 1), "##", Character.valueOf('#'), Block.STONE);
        this.addShapedRecipe(new ItemStack(Block.WOODEN_PRESSURE_PLATE, 1), "##", Character.valueOf('#'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Block.DISPENSER, 1), "###", "#X#", "#R#", Character.valueOf('#'), Block.COBBLESTONE, Character.valueOf('X'), Item.BOW, Character.valueOf('R'), Item.REDSTONE);
        this.addShapedRecipe(new ItemStack(Block.PISTON, 1), "TTT", "#X#", "#R#", Character.valueOf('#'), Block.COBBLESTONE, Character.valueOf('X'), Item.IRON_INGOT, Character.valueOf('R'), Item.REDSTONE, Character.valueOf('T'), Block.PLANKS);
        this.addShapedRecipe(new ItemStack(Block.STICKY_PISTON, 1), "S", "P", Character.valueOf('S'), Item.SLIMEBALL, Character.valueOf('P'), Block.PISTON);
        this.addShapedRecipe(new ItemStack(Item.BED, 1), "###", "XXX", Character.valueOf('#'), Block.WOOL, Character.valueOf('X'), Block.PLANKS);
        Collections.sort(this.recipes, new Comparator(){

            public int compare(CraftingRecipe arg, CraftingRecipe arg2) {
                if (arg instanceof ShapelessRecipe && arg2 instanceof ShapedRecipe) {
                    return 1;
                }
                if (arg2 instanceof ShapelessRecipe && arg instanceof ShapedRecipe) {
                    return -1;
                }
                if (arg2.getSize() < arg.getSize()) {
                    return -1;
                }
                if (arg2.getSize() > arg.getSize()) {
                    return 1;
                }
                return 0;
            }
        });
        System.out.println(this.recipes.size() + " recipes");
    }

    void addShapedRecipe(ItemStack output, Object ... input) {
        Object object;
        Object object2;
        String string = "";
        int n = 0;
        int n2 = 0;
        int n3 = 0;
        if (input[n] instanceof String[]) {
            object2 = (String[])input[n++];
            for (int i = 0; i < ((String[])object2).length; ++i) {
                object = object2[i];
                ++n3;
                n2 = ((String)object).length();
                string = string + (String)object;
            }
        } else {
            while (input[n] instanceof String) {
                object2 = (String)input[n++];
                ++n3;
                n2 = ((String)object2).length();
                string = string + (String)object2;
            }
        }
        object2 = new HashMap();
        while (n < input.length) {
            Character c = (Character)input[n];
            object = null;
            if (input[n + 1] instanceof Item) {
                object = new ItemStack((Item)input[n + 1]);
            } else if (input[n + 1] instanceof Block) {
                object = new ItemStack((Block)input[n + 1], 1, -1);
            } else if (input[n + 1] instanceof ItemStack) {
                object = (ItemStack)input[n + 1];
            }
            object2.put(c, object);
            n += 2;
        }
        ItemStack[] itemStackArray = new ItemStack[n2 * n3];
        for (int i = 0; i < n2 * n3; ++i) {
            char c = string.charAt(i);
            itemStackArray[i] = object2.containsKey(Character.valueOf(c)) ? ((ItemStack)object2.get(Character.valueOf(c))).copy() : null;
        }
        this.recipes.add(new ShapedRecipe(n2, n3, itemStackArray, output));
    }

    void addShapelessRecipe(ItemStack output, Object ... input) {
        ArrayList<ItemStack> arrayList = new ArrayList<ItemStack>();
        for (Object object : input) {
            if (object instanceof ItemStack) {
                arrayList.add(((ItemStack)object).copy());
                continue;
            }
            if (object instanceof Item) {
                arrayList.add(new ItemStack((Item)object));
                continue;
            }
            if (object instanceof Block) {
                arrayList.add(new ItemStack((Block)object));
                continue;
            }
            throw new RuntimeException("Invalid shapeless recipy!");
        }
        this.recipes.add(new ShapelessRecipe(output, arrayList));
    }

    public ItemStack craft(CraftingInventory craftingInventory) {
        for (int i = 0; i < this.recipes.size(); ++i) {
            CraftingRecipe craftingRecipe = (CraftingRecipe)this.recipes.get(i);
            if (!craftingRecipe.matches(craftingInventory)) continue;
            return craftingRecipe.craft(craftingInventory);
        }
        return null;
    }

    public List getRecipes() {
        return this.recipes;
    }
}

