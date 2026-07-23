/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonArray;
import argo.jdom.JsonConstants;
import argo.jdom.JsonNode;
import argo.jdom.JsonNumberNode;
import argo.jdom.JsonObject;
import argo.jdom.JsonRootNode;
import argo.jdom.JsonStringNode;
import java.util.Arrays;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonNodeFactories {
    private JsonNodeFactories() {
    }

    public static JsonNode aJsonNull() {
        return JsonConstants.NULL;
    }

    public static JsonNode aJsonTrue() {
        return JsonConstants.TRUE;
    }

    public static JsonNode aJsonFalse() {
        return JsonConstants.FALSE;
    }

    public static JsonStringNode aJsonString(String string) {
        return new JsonStringNode(string);
    }

    public static JsonNode aJsonNumber(String string) {
        return new JsonNumberNode(string);
    }

    public static JsonRootNode aJsonArray(Iterable iterable) {
        return new JsonArray(iterable);
    }

    public static JsonRootNode aJsonArray(JsonNode ... jsonNodes) {
        return JsonNodeFactories.aJsonArray(Arrays.asList(jsonNodes));
    }

    public static JsonRootNode aJsonObject(Map map) {
        return new JsonObject(map);
    }
}

