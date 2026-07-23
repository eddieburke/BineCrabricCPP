/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonFieldBuilder;
import argo.jdom.JsonNodeBuilder;
import argo.jdom.JsonNodeFactories;
import argo.jdom.JsonRootNode;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonObjectNodeBuilder
implements JsonNodeBuilder {
    private final List fieldBuilders = new LinkedList();

    JsonObjectNodeBuilder() {
    }

    public JsonObjectNodeBuilder withFieldBuilder(JsonFieldBuilder jsonFieldBuilder) {
        this.fieldBuilders.add(jsonFieldBuilder);
        return this;
    }

    public JsonRootNode build() {
        return JsonNodeFactories.aJsonObject(new HashMap(){
            {
                for (JsonFieldBuilder jsonFieldBuilder : JsonObjectNodeBuilder.this.fieldBuilders) {
                    this.put(jsonFieldBuilder.buildKey(), jsonFieldBuilder.buildValue());
                }
            }
        });
    }
}

