/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonNodeType;
import argo.jdom.JsonRootNode;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class JsonObject
extends JsonRootNode {
    private final Map fields;

    JsonObject(Map map) {
        this.fields = new HashMap(map);
    }

    public Map getFields() {
        return new HashMap(this.fields);
    }

    public JsonNodeType getType() {
        return JsonNodeType.OBJECT;
    }

    public String getText() {
        throw new IllegalStateException("Attempt to get text on a JsonNode without text.");
    }

    public List getElements() {
        throw new IllegalStateException("Attempt to get elements on a JsonNode without elements.");
    }

    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }
        if (object == null || this.getClass() != object.getClass()) {
            return false;
        }
        JsonObject jsonObject = (JsonObject)object;
        return ((Object)this.fields).equals(jsonObject.fields);
    }

    public int hashCode() {
        return ((Object)this.fields).hashCode();
    }

    public String toString() {
        return "JsonObject fields:[" + this.fields + "]";
    }
}

