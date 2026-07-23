/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package com.jcraft.jogg;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public class Packet {
    public byte[] packet_base;
    public int packet;
    public int bytes;
    public int b_o_s;
    public int e_o_s;
    public long granulepos;
    public long packetno;
}

