/*
 * Decompiled with CFR 0.1.1 (FabricMC 57d88659).
 * 
 * Could not load the following classes:
 *  net.fabricmc.api.EnvType
 *  net.fabricmc.api.Environment
 */
package net.minecraft.client.render;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.minecraft.client.util.GlAllocationUtils;
import org.lwjgl.opengl.ARBVertexBufferObject;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GLContext;

@Environment(value=EnvType.CLIENT)
public class Tessellator {
    private static boolean TRIANGLE_MODE = true;
    private static boolean USE_VBO = false;
    private ByteBuffer byteBuffer;
    private IntBuffer intBuffer;
    private FloatBuffer floatBuffer;
    private int[] buffer;
    private int vertexCount = 0;
    private double u;
    private double v;
    private int color;
    private boolean hasColor = false;
    private boolean hasTexture = false;
    private boolean hasNormals = false;
    private int bufferPosition = 0;
    private int addedVertexCount = 0;
    private boolean colorDisabled = false;
    private int mode;
    private double xOffset;
    private double yOffset;
    private double zOffset;
    private int normal;
    public static final Tessellator INSTANCE = new Tessellator(0x200000);
    private boolean drawing = false;
    private boolean useVbo = false;
    private IntBuffer vboBuffer;
    private int vbo = 0;
    private int vboCount = 10;
    private int bufferSize;

    private Tessellator(int bufferSize) {
        this.bufferSize = bufferSize;
        this.byteBuffer = GlAllocationUtils.allocateByteBuffer(bufferSize * 4);
        this.intBuffer = this.byteBuffer.asIntBuffer();
        this.floatBuffer = this.byteBuffer.asFloatBuffer();
        this.buffer = new int[bufferSize];
        boolean bl = this.useVbo = USE_VBO && GLContext.getCapabilities().GL_ARB_vertex_buffer_object;
        if (this.useVbo) {
            this.vboBuffer = GlAllocationUtils.allocateIntBuffer(this.vboCount);
            ARBVertexBufferObject.glGenBuffersARB(this.vboBuffer);
        }
    }

    public void draw() {
        if (!this.drawing) {
            throw new IllegalStateException("Not tesselating!");
        }
        this.drawing = false;
        if (this.vertexCount > 0) {
            this.intBuffer.clear();
            this.intBuffer.put(this.buffer, 0, this.bufferPosition);
            this.byteBuffer.position(0);
            this.byteBuffer.limit(this.bufferPosition * GL11.GL_TRIANGLES);
            if (this.useVbo) {
                this.vbo = (this.vbo + GL11.GL_LINES) % this.vboCount;
                ARBVertexBufferObject.glBindBufferARB(34962, this.vboBuffer.get(this.vbo));
                ARBVertexBufferObject.glBufferDataARB(34962, this.byteBuffer, 35040);
            }
            if (this.hasTexture) {
                if (this.useVbo) {
                    GL11.glTexCoordPointer(2, 5126, 32, 12L);
                } else {
                    this.floatBuffer.position(3);
                    GL11.glTexCoordPointer(2, 32, this.floatBuffer);
                }
                GL11.glEnableClientState(32888);
            }
            if (this.hasColor) {
                if (this.useVbo) {
                    GL11.glColorPointer(4, 5121, 32, 20L);
                } else {
                    this.byteBuffer.position(20);
                    GL11.glColorPointer(4, true, 32, this.byteBuffer);
                }
                GL11.glEnableClientState(32886);
            }
            if (this.hasNormals) {
                if (this.useVbo) {
                    GL11.glNormalPointer(5120, 32, 24L);
                } else {
                    this.byteBuffer.position(24);
                    GL11.glNormalPointer(32, this.byteBuffer);
                }
                GL11.glEnableClientState(32885);
            }
            if (this.useVbo) {
                GL11.glVertexPointer(3, 5126, 32, 0L);
            } else {
                this.floatBuffer.position(0);
                GL11.glVertexPointer(3, 32, this.floatBuffer);
            }
            GL11.glEnableClientState(32884);
            if (this.mode == GL11.GL_QUADS && TRIANGLE_MODE) {
                GL11.glDrawArrays(GL11.GL_TRIANGLES, 0, this.vertexCount);
            } else {
                GL11.glDrawArrays(this.mode, 0, this.vertexCount);
            }
            GL11.glDisableClientState(32884);
            if (this.hasTexture) {
                GL11.glDisableClientState(32888);
            }
            if (this.hasColor) {
                GL11.glDisableClientState(32886);
            }
            if (this.hasNormals) {
                GL11.glDisableClientState(32885);
            }
        }
        this.reset();
    }

    private void reset() {
        this.vertexCount = 0;
        this.byteBuffer.clear();
        this.bufferPosition = 0;
        this.addedVertexCount = 0;
    }

    public void startQuads() {
        this.start(GL11.GL_QUADS);
    }

    public void start(int mode) {
        if (this.drawing) {
            throw new IllegalStateException("Already tesselating!");
        }
        this.drawing = true;
        this.reset();
        this.mode = mode;
        this.hasNormals = false;
        this.hasColor = false;
        this.hasTexture = false;
        this.colorDisabled = false;
    }

    public void texture(double u, double v) {
        this.hasTexture = true;
        this.u = u;
        this.v = v;
    }

    public void color(float r, float g, float b) {
        this.color((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f));
    }

    public void color(float r, float g, float b, float a) {
        this.color((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), (int)(a * 255.0f));
    }

    public void color(int r, int g, int b) {
        this.color(r, g, b, 255);
    }

    public void color(int r, int g, int b, int a) {
        if (this.colorDisabled) {
            return;
        }
        if (r > 255) {
            r = 255;
        }
        if (g > 255) {
            g = 255;
        }
        if (b > 255) {
            b = 255;
        }
        if (a > 255) {
            a = 255;
        }
        if (r < 0) {
            r = 0;
        }
        if (g < 0) {
            g = 0;
        }
        if (b < 0) {
            b = 0;
        }
        if (a < 0) {
            a = 0;
        }
        this.hasColor = true;
        this.color = ByteOrder.nativeOrder() == ByteOrder.LITTLE_ENDIAN ? a << 24 | b << 16 | g << 8 | r : r << 24 | g << 16 | b << 8 | a;
    }

    public void vertex(double x, double y, double z, double u, double v) {
        this.texture(u, v);
        this.vertex(x, y, z);
    }

    public void vertex(double x, double y, double z) {
        ++this.addedVertexCount;
        if (this.mode == 7 && TRIANGLE_MODE && this.addedVertexCount % 4 == 0) {
            for (int i = 0; i < 2; ++i) {
                int n = 8 * (3 - i);
                if (this.hasTexture) {
                    this.buffer[this.bufferPosition + 3] = this.buffer[this.bufferPosition - n + 3];
                    this.buffer[this.bufferPosition + 4] = this.buffer[this.bufferPosition - n + 4];
                }
                if (this.hasColor) {
                    this.buffer[this.bufferPosition + 5] = this.buffer[this.bufferPosition - n + 5];
                }
                this.buffer[this.bufferPosition + 0] = this.buffer[this.bufferPosition - n + 0];
                this.buffer[this.bufferPosition + 1] = this.buffer[this.bufferPosition - n + 1];
                this.buffer[this.bufferPosition + 2] = this.buffer[this.bufferPosition - n + 2];
                ++this.vertexCount;
                this.bufferPosition += 8;
            }
        }
        if (this.hasTexture) {
            this.buffer[this.bufferPosition + 3] = Float.floatToRawIntBits((float)this.u);
            this.buffer[this.bufferPosition + 4] = Float.floatToRawIntBits((float)this.v);
        }
        if (this.hasColor) {
            this.buffer[this.bufferPosition + 5] = this.color;
        }
        if (this.hasNormals) {
            this.buffer[this.bufferPosition + 6] = this.normal;
        }
        this.buffer[this.bufferPosition + 0] = Float.floatToRawIntBits((float)(x + this.xOffset));
        this.buffer[this.bufferPosition + 1] = Float.floatToRawIntBits((float)(y + this.yOffset));
        this.buffer[this.bufferPosition + 2] = Float.floatToRawIntBits((float)(z + this.zOffset));
        this.bufferPosition += 8;
        ++this.vertexCount;
        if (this.vertexCount % 4 == 0 && this.bufferPosition >= this.bufferSize - 32) {
            this.draw();
            this.drawing = true;
        }
    }

    public void color(int rgb) {
        int n = rgb >> 16 & 0xFF;
        int n2 = rgb >> 8 & 0xFF;
        int n3 = rgb & 0xFF;
        this.color(n, n2, n3);
    }

    public void color(int rgb, int a) {
        int n = rgb >> 16 & 0xFF;
        int n2 = rgb >> 8 & 0xFF;
        int n3 = rgb & 0xFF;
        this.color(n, n2, n3, a);
    }

    public void disableColor() {
        this.colorDisabled = true;
    }

    public void normal(float x, float y, float z) {
        if (!this.drawing) {
            System.out.println("But..");
        }
        this.hasNormals = true;
        byte by = (byte)(x * 128.0f);
        byte by2 = (byte)(y * 127.0f);
        byte by3 = (byte)(z * 127.0f);
        this.normal = by | by2 << 8 | by3 << 16;
    }

    public void translate(double x, double y, double z) {
        this.xOffset = x;
        this.yOffset = y;
        this.zOffset = z;
    }

    public void translate(float x, float y, float z) {
        this.xOffset += (double)x;
        this.yOffset += (double)y;
        this.zOffset += (double)z;
    }
}

