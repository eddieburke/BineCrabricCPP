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
import java.util.regex.Pattern;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class JsonNumberNode
extends JsonNode {
    private static final Pattern PATTERN = Pattern.compile("(-?)(0|([1-9]([0-9]*)))(\\.[0-9]+)?((e|E)(\\+|-)?[0-9]+)?");
    private final String value;

    JsonNumberNode(String string) {
        if (string == null) {
            throw new NullPointerException("Attempt to construct a JsonNumber with a null value.");
        }
        if (!PATTERN.matcher(string).matches()) {
            throw new IllegalArgumentException("Attempt to construct a JsonNumber with a String [" + string + "] that does not match the JSON number specification.");
        }
        this.value = string;
    }

    public JsonNodeType getType() {
        return JsonNodeType.NUMBER;
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
        JsonNumberNode jsonNumberNode = (JsonNumberNode)object;
        return this.value.equals(jsonNumberNode.value);
    }

    public int hashCode() {
        return this.value.hashCode();
    }

    public String toString() {
        return "JsonNumberNode value:[" + this.value + "]";
    }
}

