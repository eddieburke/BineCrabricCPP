/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonNodeDoesNotMatchChainedJsonNodeSelectorException;
import argo.jdom.JsonNodeDoesNotMatchPathElementsException;
import argo.jdom.JsonNodeFactories;
import argo.jdom.JsonNodeSelector;
import argo.jdom.JsonNodeSelectors;
import argo.jdom.JsonNodeType;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public abstract class JsonNode {
    JsonNode() {
    }

    public abstract JsonNodeType getType();

    public abstract String getText();

    public abstract Map getFields();

    public abstract List getElements();

    public final String getStringValue(Object ... objects) {
        return (String)this.wrapExceptionsFor(JsonNodeSelectors.aStringNode(objects), this, objects);
    }

    public final List getArrayNode(Object ... objects) {
        return (List)this.wrapExceptionsFor(JsonNodeSelectors.anArrayNode(objects), this, objects);
    }

    private Object wrapExceptionsFor(JsonNodeSelector jsonNodeSelector, JsonNode jsonNode, Object[] objects) {
        try {
            return jsonNodeSelector.getValue(jsonNode);
        }
        catch (JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException) {
            throw JsonNodeDoesNotMatchPathElementsException.jsonNodeDoesNotMatchPathElementsException(jsonNodeDoesNotMatchChainedJsonNodeSelectorException, objects, JsonNodeFactories.aJsonArray(jsonNode));
        }
    }
}

