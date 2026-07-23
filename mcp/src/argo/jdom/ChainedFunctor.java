/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.Functor;
import argo.jdom.JsonNodeDoesNotMatchChainedJsonNodeSelectorException;
import argo.jdom.JsonNodeSelector;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class ChainedFunctor
implements Functor {
    private final JsonNodeSelector parentJsonNodeSelector;
    private final JsonNodeSelector childJsonNodeSelector;

    ChainedFunctor(JsonNodeSelector jsonNodeSelector, JsonNodeSelector jsonNodeSelector2) {
        this.parentJsonNodeSelector = jsonNodeSelector;
        this.childJsonNodeSelector = jsonNodeSelector2;
    }

    public boolean matchesNode(Object object) {
        return this.parentJsonNodeSelector.matches(object) && this.childJsonNodeSelector.matches(this.parentJsonNodeSelector.getValue(object));
    }

    public Object applyTo(Object object) {
        Object object2;
        Object object3;
        try {
            object3 = this.parentJsonNodeSelector.getValue(object);
        }
        catch (JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException) {
            throw JsonNodeDoesNotMatchChainedJsonNodeSelectorException.createUnchainedJsonNodeDoesNotMatchJsonNodeSelectorException(jsonNodeDoesNotMatchChainedJsonNodeSelectorException, this.parentJsonNodeSelector);
        }
        try {
            object2 = this.childJsonNodeSelector.getValue(object3);
        }
        catch (JsonNodeDoesNotMatchChainedJsonNodeSelectorException jsonNodeDoesNotMatchChainedJsonNodeSelectorException) {
            throw JsonNodeDoesNotMatchChainedJsonNodeSelectorException.createChainedJsonNodeDoesNotMatchJsonNodeSelectorException(jsonNodeDoesNotMatchChainedJsonNodeSelectorException, this.parentJsonNodeSelector);
        }
        return object2;
    }

    public String shortForm() {
        return this.childJsonNodeSelector.shortForm();
    }

    public String toString() {
        return this.parentJsonNodeSelector.toString() + ", with " + this.childJsonNodeSelector.toString();
    }
}

