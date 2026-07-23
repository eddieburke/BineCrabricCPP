/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.screen;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.entity.player.PlayerInventory;
import net.minecraft.inventory.Inventory;
import net.minecraft.item.ItemStack;
import net.minecraft.screen.ScreenHandlerListener;
import net.minecraft.screen.slot.Slot;

public abstract class ScreenHandler {
    public List trackedStacks = new ArrayList();
    public List slots = new ArrayList();
    public int syncId = 0;
    private short revision = 0;
    protected List listeners = new ArrayList();
    private Set players = new HashSet();

    protected void addSlot(Slot slot) {
        slot.id = this.slots.size();
        this.slots.add(slot);
        this.trackedStacks.add(null);
    }

    @Environment(value=EnvType.SERVER)
    public void addListener(ScreenHandlerListener listener) {
        if (this.listeners.contains(listener)) {
            throw new IllegalArgumentException("Listener already listening");
        }
        this.listeners.add(listener);
        listener.onContentsUpdate(this, this.getStacks());
        this.sendContentUpdates();
    }

    @Environment(value=EnvType.SERVER)
    public List getStacks() {
        ArrayList<ItemStack> arrayList = new ArrayList<ItemStack>();
        for (int i = 0; i < this.slots.size(); ++i) {
            arrayList.add(((Slot)this.slots.get(i)).getStack());
        }
        return arrayList;
    }

    public void sendContentUpdates() {
        for (int i = 0; i < this.slots.size(); ++i) {
            ItemStack itemStack = ((Slot)this.slots.get(i)).getStack();
            ItemStack itemStack2 = (ItemStack)this.trackedStacks.get(i);
            if (ItemStack.areEqual(itemStack2, itemStack)) continue;
            itemStack2 = itemStack == null ? null : itemStack.copy();
            this.trackedStacks.set(i, itemStack2);
            for (int j = 0; j < this.listeners.size(); ++j) {
                ((ScreenHandlerListener)this.listeners.get(j)).onSlotUpdate(this, i, itemStack2);
            }
        }
    }

    @Environment(value=EnvType.SERVER)
    public Slot getSlot(Inventory inventory, int index) {
        for (int i = 0; i < this.slots.size(); ++i) {
            Slot slot = (Slot)this.slots.get(i);
            if (!slot.equals(inventory, index)) continue;
            return slot;
        }
        return null;
    }

    public Slot getSlot(int index) {
        return (Slot)this.slots.get(index);
    }

    public ItemStack quickMove(int slot) {
        Slot slot2 = (Slot)this.slots.get(slot);
        if (slot2 != null) {
            return slot2.getStack();
        }
        return null;
    }

    public ItemStack onSlotClick(int index, int button, boolean shift, PlayerEntity player) {
        ItemStack itemStack = null;
        if (button == 0 || button == 1) {
            PlayerInventory playerInventory = player.inventory;
            if (index == -999) {
                if (playerInventory.getCursorStack() != null && index == -999) {
                    if (button == 0) {
                        player.dropItem(playerInventory.getCursorStack());
                        playerInventory.setCursorStack(null);
                    }
                    if (button == 1) {
                        player.dropItem(playerInventory.getCursorStack().split(1));
                        if (playerInventory.getCursorStack().count == 0) {
                            playerInventory.setCursorStack(null);
                        }
                    }
                }
            } else if (shift) {
                ItemStack itemStack2 = this.quickMove(index);
                if (itemStack2 != null) {
                    int n;
                    int n2 = itemStack2.count;
                    itemStack = itemStack2.copy();
                    Slot slot = (Slot)this.slots.get(index);
                    if (slot != null && slot.getStack() != null && (n = slot.getStack().count) < n2) {
                        this.onSlotClick(index, button, shift, player);
                    }
                }
            } else {
                Slot slot = (Slot)this.slots.get(index);
                if (slot != null) {
                    int n;
                    slot.markDirty();
                    ItemStack itemStack3 = slot.getStack();
                    ItemStack itemStack4 = playerInventory.getCursorStack();
                    if (itemStack3 != null) {
                        itemStack = itemStack3.copy();
                    }
                    if (itemStack3 == null) {
                        if (itemStack4 != null && slot.canInsert(itemStack4)) {
                            int n3;
                            int n4 = n3 = button == 0 ? itemStack4.count : 1;
                            if (n3 > slot.getMaxItemCount()) {
                                n3 = slot.getMaxItemCount();
                            }
                            slot.setStack(itemStack4.split(n3));
                            if (itemStack4.count == 0) {
                                playerInventory.setCursorStack(null);
                            }
                        }
                    } else if (itemStack4 == null) {
                        int n5 = button == 0 ? itemStack3.count : (itemStack3.count + 1) / 2;
                        ItemStack itemStack5 = slot.takeStack(n5);
                        playerInventory.setCursorStack(itemStack5);
                        if (itemStack3.count == 0) {
                            slot.setStack(null);
                        }
                        slot.onTakeItem(playerInventory.getCursorStack());
                    } else if (slot.canInsert(itemStack4)) {
                        if (itemStack3.itemId != itemStack4.itemId || itemStack3.hasSubtypes() && itemStack3.getDamage() != itemStack4.getDamage()) {
                            if (itemStack4.count <= slot.getMaxItemCount()) {
                                ItemStack itemStack6 = itemStack3;
                                slot.setStack(itemStack4);
                                playerInventory.setCursorStack(itemStack6);
                            }
                        } else {
                            int n6;
                            int n7 = n6 = button == 0 ? itemStack4.count : 1;
                            if (n6 > slot.getMaxItemCount() - itemStack3.count) {
                                n6 = slot.getMaxItemCount() - itemStack3.count;
                            }
                            if (n6 > itemStack4.getMaxCount() - itemStack3.count) {
                                n6 = itemStack4.getMaxCount() - itemStack3.count;
                            }
                            itemStack4.split(n6);
                            if (itemStack4.count == 0) {
                                playerInventory.setCursorStack(null);
                            }
                            itemStack3.count += n6;
                        }
                    } else if (!(itemStack3.itemId != itemStack4.itemId || itemStack4.getMaxCount() <= 1 || itemStack3.hasSubtypes() && itemStack3.getDamage() != itemStack4.getDamage() || (n = itemStack3.count) <= 0 || n + itemStack4.count > itemStack4.getMaxCount())) {
                        itemStack4.count += n;
                        itemStack3.split(n);
                        if (itemStack3.count == 0) {
                            slot.setStack(null);
                        }
                        slot.onTakeItem(playerInventory.getCursorStack());
                    }
                }
            }
        }
        return itemStack;
    }

    public void onClosed(PlayerEntity player) {
        PlayerInventory playerInventory = player.inventory;
        if (playerInventory.getCursorStack() != null) {
            player.dropItem(playerInventory.getCursorStack());
            playerInventory.setCursorStack(null);
        }
    }

    public void onSlotUpdate(Inventory inventory) {
        this.sendContentUpdates();
    }

    @Environment(value=EnvType.CLIENT)
    public void setStackInSlot(int index, ItemStack stack) {
        this.getSlot(index).setStack(stack);
    }

    @Environment(value=EnvType.CLIENT)
    public void updateSlotStacks(ItemStack[] stacks) {
        for (int i = 0; i < stacks.length; ++i) {
            this.getSlot(i).setStack(stacks[i]);
        }
    }

    @Environment(value=EnvType.CLIENT)
    public void setProperty(int id, int value) {
    }

    @Environment(value=EnvType.CLIENT)
    public short nextRevision(PlayerInventory inventory) {
        this.revision = (short)(this.revision + 1);
        return this.revision;
    }

    @Environment(value=EnvType.CLIENT)
    public void onAcknowledgementAccepted(short actionType) {
    }

    @Environment(value=EnvType.CLIENT)
    public void onAcknowledgementDenied(short actionType) {
    }

    @Environment(value=EnvType.SERVER)
    public boolean canOpen(PlayerEntity player) {
        return !this.players.contains(player);
    }

    @Environment(value=EnvType.SERVER)
    public void updatePlayerList(PlayerEntity player, boolean remove) {
        if (remove) {
            this.players.remove(player);
        } else {
            this.players.add(player);
        }
    }

    public abstract boolean canUse(PlayerEntity var1);

    protected void insertItem(ItemStack stack, int startIndex, int endIndex, boolean fromLast) {
        ItemStack itemStack;
        Slot slot;
        int n = startIndex;
        if (fromLast) {
            n = endIndex - 1;
        }
        if (stack.isStackable()) {
            while (stack.count > 0 && (!fromLast && n < endIndex || fromLast && n >= startIndex)) {
                slot = (Slot)this.slots.get(n);
                itemStack = slot.getStack();
                if (!(itemStack == null || itemStack.itemId != stack.itemId || stack.hasSubtypes() && stack.getDamage() != itemStack.getDamage())) {
                    int n2 = itemStack.count + stack.count;
                    if (n2 <= stack.getMaxCount()) {
                        stack.count = 0;
                        itemStack.count = n2;
                        slot.markDirty();
                    } else if (itemStack.count < stack.getMaxCount()) {
                        stack.count -= stack.getMaxCount() - itemStack.count;
                        itemStack.count = stack.getMaxCount();
                        slot.markDirty();
                    }
                }
                if (fromLast) {
                    --n;
                    continue;
                }
                ++n;
            }
        }
        if (stack.count > 0) {
            n = fromLast ? endIndex - 1 : startIndex;
            while (!fromLast && n < endIndex || fromLast && n >= startIndex) {
                slot = (Slot)this.slots.get(n);
                itemStack = slot.getStack();
                if (itemStack == null) {
                    slot.setStack(stack.copy());
                    slot.markDirty();
                    stack.count = 0;
                    break;
                }
                if (fromLast) {
                    --n;
                    continue;
                }
                ++n;
            }
        }
    }
}

