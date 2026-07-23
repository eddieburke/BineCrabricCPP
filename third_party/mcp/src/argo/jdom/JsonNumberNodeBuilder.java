/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonNode;
import argo.jdom.JsonNodeBuilder;
import argo.jdom.JsonNodeFactories;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class JsonNumberNodeBuilder
implements JsonNodeBuilder {
    private final JsonNode value;

    JsonNumberNodeBuilder(String string) {
        this.value = JsonNodeFactories.aJsonNumber(string);
    }

    public JsonNode build() {
        return this.value;
    }
}

