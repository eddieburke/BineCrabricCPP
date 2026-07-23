/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package argo.format;

import argo.format.JsonEscapedString;
import argo.format.JsonFormatter;
import argo.jdom.JsonNode;
import argo.jdom.JsonRootNode;
import argo.jdom.JsonStringNode;
import java.io.IOException;
import java.io.StringWriter;
import java.io.Writer;
import java.util.TreeSet;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(value=EnvType.CLIENT)
public final class CompactJsonFormatter
implements JsonFormatter {
    public String format(JsonRootNode jsonRootNode) {
        StringWriter stringWriter = new StringWriter();
        try {
            this.format(jsonRootNode, stringWriter);
        }
        catch (IOException iOException) {
            throw new RuntimeException("Coding failure in Argo:  StringWriter gave an IOException", iOException);
        }
        return stringWriter.toString();
    }

    public void format(JsonRootNode jsonRootNode, Writer writer) {
        this.formatJsonNode(jsonRootNode, writer);
    }

    private void formatJsonNode(JsonNode jsonNode, Writer writer) {
        boolean bl = true;
        switch (jsonNode.getType()) {
            case ARRAY: {
                writer.append('[');
                for (JsonNode jsonNode2 : jsonNode.getElements()) {
                    if (!bl) {
                        writer.append(',');
                    }
                    bl = false;
                    this.formatJsonNode(jsonNode2, writer);
                }
                writer.append(']');
                break;
            }
            case OBJECT: {
                writer.append('{');
                for (JsonStringNode jsonStringNode : new TreeSet(jsonNode.getFields().keySet())) {
                    if (!bl) {
                        writer.append(',');
                    }
                    bl = false;
                    this.formatJsonNode(jsonStringNode, writer);
                    writer.append(':');
                    this.formatJsonNode((JsonNode)jsonNode.getFields().get(jsonStringNode), writer);
                }
                writer.append('}');
                break;
            }
            case STRING: {
                writer.append('\"').append(new JsonEscapedString(jsonNode.getText()).toString()).append('\"');
                break;
            }
            case NUMBER: {
                writer.append(jsonNode.getText());
                break;
            }
            case FALSE: {
                writer.append("false");
                break;
            }
            case TRUE: {
                writer.append("true");
                break;
            }
            case NULL: {
                writer.append("null");
                break;
            }
            default: {
                throw new RuntimeException("Coding failure in Argo:  Attempt to format a JsonNode of unknown type [" + (Object)((Object)jsonNode.getType()) + "];");
            }
        }
    }
}

