/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.saj;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public interface JsonListener {
    public void startDocument();

    public void endDocument();

    public void startArray();

    public void endArray();

    public void startObject();

    public void endObject();

    public void startField(String var1);

    public void endField();

    public void stringValue(String var1);

    public void numberValue(String var1);

    public void trueValue();

    public void falseValue();

    public void nullValue();
}

