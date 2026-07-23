/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.jdom;

import argo.jdom.JsonArrayNodeBuilder;
import argo.jdom.JsonFieldBuilder;
import argo.jdom.JsonNodeBuilder;
import argo.jdom.JsonNodeBuilders;
import argo.jdom.JsonObjectNodeBuilder;
import argo.jdom.JsonRootNode;
import argo.saj.JsonListener;
import java.util.Stack;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
final class JsonListenerToJdomAdapter
implements JsonListener {
    private final Stack stack = new Stack();
    private JsonNodeBuilder root;

    JsonListenerToJdomAdapter() {
    }

    JsonRootNode getDocument() {
        return (JsonRootNode)this.root.build();
    }

    public void startDocument() {
    }

    public void endDocument() {
    }

    public void startArray() {
        final JsonArrayNodeBuilder jsonArrayNodeBuilder = JsonNodeBuilders.anArrayBuilder();
        this.addRootNode(jsonArrayNodeBuilder);
        this.stack.push(new NodeContainer(){

            public void addNode(JsonNodeBuilder jsonNodeBuilder) {
                jsonArrayNodeBuilder.withElement(jsonNodeBuilder);
            }

            public void addField(JsonFieldBuilder jsonFieldBuilder) {
                throw new RuntimeException("Coding failure in Argo:  Attempt to add a field to an array.");
            }
        });
    }

    public void endArray() {
        this.stack.pop();
    }

    public void startObject() {
        final JsonObjectNodeBuilder jsonObjectNodeBuilder = JsonNodeBuilders.anObjectBuilder();
        this.addRootNode(jsonObjectNodeBuilder);
        this.stack.push(new NodeContainer(){

            public void addNode(JsonNodeBuilder jsonNodeBuilder) {
                throw new RuntimeException("Coding failure in Argo:  Attempt to add a node to an object.");
            }

            public void addField(JsonFieldBuilder jsonFieldBuilder) {
                jsonObjectNodeBuilder.withFieldBuilder(jsonFieldBuilder);
            }
        });
    }

    public void endObject() {
        this.stack.pop();
    }

    public void startField(String string) {
        final JsonFieldBuilder jsonFieldBuilder = JsonFieldBuilder.aJsonFieldBuilder().withKey(JsonNodeBuilders.aStringBuilder(string));
        ((NodeContainer)this.stack.peek()).addField(jsonFieldBuilder);
        this.stack.push(new NodeContainer(){

            public void addNode(JsonNodeBuilder jsonNodeBuilder) {
                jsonFieldBuilder.withValue(jsonNodeBuilder);
            }

            public void addField(JsonFieldBuilder jsonFieldBuilder2) {
                throw new RuntimeException("Coding failure in Argo:  Attempt to add a field to a field.");
            }
        });
    }

    public void endField() {
        this.stack.pop();
    }

    public void numberValue(String string) {
        this.addValue(JsonNodeBuilders.aNumberBuilder(string));
    }

    public void trueValue() {
        this.addValue(JsonNodeBuilders.aTrueBuilder());
    }

    public void stringValue(String string) {
        this.addValue(JsonNodeBuilders.aStringBuilder(string));
    }

    public void falseValue() {
        this.addValue(JsonNodeBuilders.aFalseBuilder());
    }

    public void nullValue() {
        this.addValue(JsonNodeBuilders.aNullBuilder());
    }

    private void addRootNode(JsonNodeBuilder jsonNodeBuilder) {
        if (this.root == null) {
            this.root = jsonNodeBuilder;
        } else {
            this.addValue(jsonNodeBuilder);
        }
    }

    private void addValue(JsonNodeBuilder jsonNodeBuilder) {
        ((NodeContainer)this.stack.peek()).addNode(jsonNodeBuilder);
    }

    @Environment(value=EnvType.CLIENT)
    interface NodeContainer {
        public void addNode(JsonNodeBuilder var1);

        public void addField(JsonFieldBuilder var1);
    }
}

