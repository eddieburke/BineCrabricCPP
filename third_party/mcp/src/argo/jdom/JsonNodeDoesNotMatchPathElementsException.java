/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.format.CompactJsonFormatter;
import argo.format.JsonFormatter;
import argo.jdom.JsonNodeDoesNotMatchChainedJsonNodeSelectorException;
import argo.jdom.JsonNodeDoesNotMatchJsonNodeSelectorException;
import argo.jdom.JsonRootNode;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonNodeDoesNotMatchPathElementsException
extends JsonNodeDoesNotMatchJsonNodeSelectorException {
    private static final JsonFormatter JSON_FORMATTER = new CompactJsonFormatter();

    static JsonNodeDoesNotMatchPathElementsException jsonNodeDoesNotMatchPathElementsException(JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException, Object[] objects, JsonRootNode jsonRootNode) {
        return new JsonNodeDoesNotMatchPathElementsException(jsonNodeDoesNotMatchChainedJsonNodeSelectorException, objects, jsonRootNode);
    }

    private JsonNodeDoesNotMatchPathElementsException(JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException, Object[] objects, JsonRootNode jsonRootNode) {
        super(JsonNodeDoesNotMatchPathElementsException.formatMessage(jsonNodeDoesNotMatchChainedJsonNodeSelectorException, objects, jsonRootNode));
    }

    private static String formatMessage(JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException, Object[] objects, JsonRootNode jsonRootNode) {
        return "Failed to find " + jsonNodeDoesNotMatchChainedJsonNodeSelectorException.failedNode.toString() + " at [" + JsonNodeDoesNotMatchChainedJsonNodeSelectorException.getShortFormFailPath(jsonNodeDoesNotMatchChainedJsonNodeSelectorException.failPath) + "] while resolving [" + JsonNodeDoesNotMatchPathElementsException.commaSeparate(objects) + "] in " + JSON_FORMATTER.format(jsonRootNode) + ".";
    }

    private static String commaSeparate(Object[] objects) {
        StringBuilder stringBuilder = new StringBuilder();
        boolean bl = true;
        for (Object object : objects) {
            if (!bl) {
                stringBuilder.append(".");
            }
            bl = false;
            if (object instanceof String) {
                stringBuilder.append("\"").append(object).append("\"");
                continue;
            }
            stringBuilder.append(object);
        }
        return stringBuilder.toString();
    }
}

