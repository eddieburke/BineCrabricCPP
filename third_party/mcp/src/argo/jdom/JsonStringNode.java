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
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonStringNode
extends JsonNode
implements Comparable {
    private final String value;

    JsonStringNode(String string) {
        if (string == null) {
            throw new NullPointerException("Attempt to construct a JsonString with a null value.");
        }
        this.value = string;
    }

    public JsonNodeType getType() {
        return JsonNodeType.STRING;
    }

    public String getText() {
        return this.value;
    }

    public Map getFields() {
        throw new IllegalStateException("Attempt to get fields on a JsonNode without fields.");
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
        JsonStringNode jsonStringNode = (JsonStringNode)object;
        return this.value.equals(jsonStringNode.value);
    }

    public int hashCode() {
        return this.value.hashCode();
    }

    public String toString() {
        return "JsonStringNode value:[" + this.value + "]";
    }

    public int compareTo(JsonStringNode jsonStringNode) {
        return this.value.compareTo(jsonStringNode.value);
    }
}

