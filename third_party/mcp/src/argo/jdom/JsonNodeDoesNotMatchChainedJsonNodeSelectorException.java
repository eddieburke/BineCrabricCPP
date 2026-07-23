/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.Functor;
import argo.jdom.JsonNodeDoesNotMatchJsonNodeSelectorException;
import argo.jdom.JsonNodeSelector;
import java.util.LinkedList;
import java.util.List;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonNodeDoesNotMatchChainedJsonNodeSelectorException
extends JsonNodeDoesNotMatchJsonNodeSelectorException {
    final Functor failedNode;
    final List failPath;

    static JsonNodeDoesNotMatchJsonNodeSelectorException createJsonNodeDoesNotMatchJsonNodeSelectorException(Functor functor) {
        return new JsonNodeDoesNotMatchChainedJsonNodeSelectorException(functor, new LinkedList());
    }

    static JsonNodeDoesNotMatchJsonNodeSelectorException createChainedJsonNodeDoesNotMatchJsonNodeSelectorException(JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException, JsonNodeSelector jsonNodeSelector) {
        LinkedList<JsonNodeSelector> linkedList = new LinkedList<JsonNodeSelector>(jsonNodeDoesNotMatchChainedJsonNodeSelectorException.failPath);
        linkedList.add(jsonNodeSelector);
        return new JsonNodeDoesNotMatchChainedJsonNodeSelectorException(jsonNodeDoesNotMatchChainedJsonNodeSelectorException.failedNode, linkedList);
    }

    static JsonNodeDoesNotMatchJsonNodeSelectorException createUnchainedJsonNodeDoesNotMatchJsonNodeSelectorException(JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException, JsonNodeSelector jsonNodeSelector) {
        LinkedList<JsonNodeSelector> linkedList = new LinkedList<JsonNodeSelector>();
        linkedList.add(jsonNodeSelector);
        return new JsonNodeDoesNotMatchChainedJsonNodeSelectorException(jsonNodeDoesNotMatchChainedJsonNodeSelectorException.failedNode, linkedList);
    }

    private JsonNodeDoesNotMatchChainedJsonNodeSelectorException(Functor functor, List list) {
        super("Failed to match any JSON node at [" + JsonNodeDoesNotMatchChainedJsonNodeSelectorException.getShortFormFailPath(list) + "]");
        this.failedNode = functor;
        this.failPath = list;
    }

    static String getShortFormFailPath(List list) {
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = list.size() - 1; i >= 0; --i) {
            stringBuilder.append(((JsonNodeSelector)list.get(i)).shortForm());
            if (i == 0) continue;
            stringBuilder.append(".");
        }
        return stringBuilder.toString();
    }

    public String toString() {
        return "JsonNodeDoesNotMatchJsonNodeSelectorException{failedNode=" + this.failedNode + ", failPath=" + this.failPath + '}';
    }
}

