/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.ChainedFunctor;
import argo.jdom.JsonNode;
import argo.jdom.JsonNodeFactories;
import argo.jdom.JsonNodeSelector;
import argo.jdom.JsonNodeType;
import argo.jdom.JsonStringNode;
import argo.jdom.LeafFunctor;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class JsonNodeSelectors {
    private JsonNodeSelectors() {
    }

    public static JsonNodeSelector aStringNode(Object ... objects) {
        return JsonNodeSelectors.chainOn(objects, new JsonNodeSelector(new LeafFunctor(){

            public boolean matchesNode(JsonNode jsonNode) {
                return JsonNodeType.STRING == jsonNode.getType();
            }

            public String shortForm() {
                return "A short form string";
            }

            public String typeSafeApplyTo(JsonNode jsonNode) {
                return jsonNode.getText();
            }

            public String toString() {
                return "a value that is a string";
            }
        }));
    }

    public static JsonNodeSelector anArrayNode(Object ... objects) {
        return JsonNodeSelectors.chainOn(objects, new JsonNodeSelector(new LeafFunctor(){

            public boolean matchesNode(JsonNode jsonNode) {
                return JsonNodeType.ARRAY == jsonNode.getType();
            }

            public String shortForm() {
                return "A short form array";
            }

            public List typeSafeApplyTo(JsonNode jsonNode) {
                return jsonNode.getElements();
            }

            public String toString() {
                return "an array";
            }
        }));
    }

    public static JsonNodeSelector anObjectNode(Object ... objects) {
        return JsonNodeSelectors.chainOn(objects, new JsonNodeSelector(new LeafFunctor(){

            public boolean matchesNode(JsonNode jsonNode) {
                return JsonNodeType.OBJECT == jsonNode.getType();
            }

            public String shortForm() {
                return "A short form object";
            }

            public Map typeSafeApplyTo(JsonNode jsonNode) {
                return jsonNode.getFields();
            }

            public String toString() {
                return "an object";
            }
        }));
    }

    public static JsonNodeSelector aField(String string) {
        return JsonNodeSelectors.aField(JsonNodeFactories.aJsonString(string));
    }

    public static JsonNodeSelector aField(final JsonStringNode jsonStringNode) {
        return new JsonNodeSelector(new LeafFunctor(){

            public boolean matchesNode(Map map) {
                return map.containsKey(jsonStringNode);
            }

            public String shortForm() {
                return "\"" + jsonStringNode.getText() + "\"";
            }

            public JsonNode typeSafeApplyTo(Map map) {
                return (JsonNode)map.get(jsonStringNode);
            }

            public String toString() {
                return "a field called [\"" + jsonStringNode.getText() + "\"]";
            }
        });
    }

    public static JsonNodeSelector anObjectNodeWithField(String string) {
        return JsonNodeSelectors.anObjectNode(new Object[0]).with(JsonNodeSelectors.aField(string));
    }

    public static JsonNodeSelector anElement(final int i) {
        return new JsonNodeSelector(new LeafFunctor(){

            public boolean matchesNode(List list) {
                return list.size() > i;
            }

            public String shortForm() {
                return Integer.toString(i);
            }

            public JsonNode typeSafeApplyTo(List list) {
                return (JsonNode)list.get(i);
            }

            public String toString() {
                return "an element at index [" + i + "]";
            }
        });
    }

    public static JsonNodeSelector anArrayNodeWithElement(int i) {
        return JsonNodeSelectors.anArrayNode(new Object[0]).with(JsonNodeSelectors.anElement(i));
    }

    private static JsonNodeSelector chainOn(Object[] objects, JsonNodeSelector jsonNodeSelector) {
        JsonNodeSelector jsonNodeSelector2 = jsonNodeSelector;
        for (int i = objects.length - 1; i >= 0; --i) {
            if (objects[i] instanceof Integer) {
                jsonNodeSelector2 = JsonNodeSelectors.chainedJsonNodeSelector(JsonNodeSelectors.anArrayNodeWithElement((Integer)objects[i]), jsonNodeSelector2);
                continue;
            }
            if (objects[i] instanceof String) {
                jsonNodeSelector2 = JsonNodeSelectors.chainedJsonNodeSelector(JsonNodeSelectors.anObjectNodeWithField((String)objects[i]), jsonNodeSelector2);
                continue;
            }
            throw new IllegalArgumentException("Element [" + objects[i] + "] of path elements" + " [" + Arrays.toString(objects) + "] was of illegal type [" + objects[i].getClass().getCanonicalName() + "]; only Integer and String are valid.");
        }
        return jsonNodeSelector2;
    }

    private static JsonNodeSelector chainedJsonNodeSelector(JsonNodeSelector jsonNodeSelector, JsonNodeSelector jsonNodeSelector2) {
        return new JsonNodeSelector(new ChainedFunctor(jsonNodeSelector, jsonNodeSelector2));
    }
}

