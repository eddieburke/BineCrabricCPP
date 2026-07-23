/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonArrayNodeBuilder;
import argo.jdom.JsonNode;
import argo.jdom.JsonNodeBuilder;
import argo.jdom.JsonNodeFactories;
import argo.jdom.JsonNumberNodeBuilder;
import argo.jdom.JsonObjectNodeBuilder;
import argo.jdom.JsonStringNodeBuilder;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonNodeBuilders {
    private JsonNodeBuilders() {
    }

    public static JsonNodeBuilder aNullBuilder() {
        return new JsonNodeBuilder(){

            public JsonNode build() {
                return JsonNodeFactories.aJsonNull();
            }
        };
    }

    public static JsonNodeBuilder aTrueBuilder() {
        return new JsonNodeBuilder(){

            public JsonNode build() {
                return JsonNodeFactories.aJsonTrue();
            }
        };
    }

    public static JsonNodeBuilder aFalseBuilder() {
        return new JsonNodeBuilder(){

            public JsonNode build() {
                return JsonNodeFactories.aJsonFalse();
            }
        };
    }

    public static JsonNodeBuilder aNumberBuilder(String string) {
        return new JsonNumberNodeBuilder(string);
    }

    public static JsonStringNodeBuilder aStringBuilder(String string) {
        return new JsonStringNodeBuilder(string);
    }

    public static JsonObjectNodeBuilder anObjectBuilder() {
        return new JsonObjectNodeBuilder();
    }

    public static JsonArrayNodeBuilder anArrayBuilder() {
        return new JsonArrayNodeBuilder();
    }
}

