/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonNode;
import argo.jdom.JsonNodeType;
import argo.jdom.JsonRootNode;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class JsonArray
extends JsonRootNode {
    private final List elements;

    JsonArray(Iterable iterable) {
        this.elements = JsonArray.asList(iterable);
    }

    public JsonNodeType getType() {
        return JsonNodeType.ARRAY;
    }

    public List getElements() {
        return new ArrayList(this.elements);
    }

    public String getText() {
        throw new IllegalStateException("Attempt to get text on a JsonNode without text.");
    }

    public Map getFields() {
        throw new IllegalStateException("Attempt to get fields on a JsonNode without fields.");
    }

    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }
        if (object == null || this.getClass() != object.getClass()) {
            return false;
        }
        JsonArray jsonArray = (JsonArray)object;
        return ((Object)this.elements).equals(jsonArray.elements);
    }

    public int hashCode() {
        return ((Object)this.elements).hashCode();
    }

    public String toString() {
        return "JsonArray elements:[" + this.elements + "]";
    }

    private static List asList(final Iterable iterable) {
        return new ArrayList(){
            {
                for (JsonNode jsonNode : iterable) {
                    this.add(jsonNode);
                }
            }
        };
    }
}

