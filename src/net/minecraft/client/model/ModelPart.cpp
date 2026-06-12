#include "net/minecraft/client/model/ModelPart.hpp"

#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/util/GlAllocationUtils.hpp"

#include <array>

namespace net::minecraft::client::model {

ModelPart::ModelPart(const ModelPart& other)
    : textureU(other.textureU),
      textureV(other.textureV),
      mirror(other.mirror),
      visible(other.visible),
      hidden(other.hidden),
      pitch(other.pitch),
      yaw(other.yaw),
      roll(other.roll),
      pivotX(other.pivotX),
      pivotY(other.pivotY),
      pivotZ(other.pivotZ),
      faces_(other.faces_),
      children_(other.children_)
{
}

ModelPart& ModelPart::operator=(const ModelPart& other)
{
    if (this == &other) {
        return *this;
    }
    textureU = other.textureU;
    textureV = other.textureV;
    mirror = other.mirror;
    visible = other.visible;
    hidden = other.hidden;
    pitch = other.pitch;
    yaw = other.yaw;
    roll = other.roll;
    pivotX = other.pivotX;
    pivotY = other.pivotY;
    pivotZ = other.pivotZ;
    faces_ = other.faces_;
    children_ = other.children_;
    compiled_ = false;
    list_ = 0;
    return *this;
}

void ModelPart::addCuboid(float x, float y, float z, int sizeX, int sizeY, int sizeZ, float dilation)
{
    float x1 = x + static_cast<float>(sizeX);
    float y1 = y + static_cast<float>(sizeY);
    float z1 = z + static_cast<float>(sizeZ);
    x -= dilation;
    y -= dilation;
    z -= dilation;
    x1 += dilation;
    y1 += dilation;
    z1 += dilation;
    if (mirror) {
        const float swap = x1;
        x1 = x;
        x = swap;
    }

    const Vertex vertex(x, y, z, 0.0f, 0.0f);
    const Vertex vertex2(x1, y, z, 0.0f, 8.0f);
    const Vertex vertex3(x1, y1, z, 8.0f, 8.0f);
    const Vertex vertex4(x, y1, z, 8.0f, 0.0f);
    const Vertex vertex5(x, y, z1, 0.0f, 0.0f);
    const Vertex vertex6(x1, y, z1, 0.0f, 8.0f);
    const Vertex vertex7(x1, y1, z1, 8.0f, 8.0f);
    const Vertex vertex8(x, y1, z1, 8.0f, 0.0f);

    std::array<Quad, 6> newFaces {
        Quad(std::array<Vertex, 4> {vertex6, vertex2, vertex3, vertex7}, textureU + sizeZ + sizeX, textureV + sizeZ,
            textureU + sizeZ + sizeX + sizeZ, textureV + sizeZ + sizeY),
        Quad(std::array<Vertex, 4> {vertex, vertex5, vertex8, vertex4}, textureU + 0, textureV + sizeZ, textureU + sizeZ,
            textureV + sizeZ + sizeY),
        Quad(std::array<Vertex, 4> {vertex6, vertex5, vertex, vertex2}, textureU + sizeZ, textureV + 0, textureU + sizeZ + sizeX,
            textureV + sizeZ),
        Quad(std::array<Vertex, 4> {vertex3, vertex4, vertex8, vertex7}, textureU + sizeZ + sizeX, textureV + 0,
            textureU + sizeZ + sizeX + sizeX, textureV + sizeZ),
        Quad(std::array<Vertex, 4> {vertex2, vertex, vertex4, vertex3}, textureU + sizeZ, textureV + sizeZ, textureU + sizeZ + sizeX,
            textureV + sizeZ + sizeY),
        Quad(std::array<Vertex, 4> {vertex5, vertex6, vertex7, vertex8}, textureU + sizeZ + sizeX + sizeZ, textureV + sizeZ,
            textureU + sizeZ + sizeX + sizeZ + sizeX, textureV + sizeZ + sizeY),
    };
    if (mirror) {
        for (Quad& face : newFaces) {
            face.flip();
        }
    }
    faces_.insert(faces_.end(), newFaces.begin(), newFaces.end());
    compiled_ = false;
    list_ = 0;
}

void ModelPart::setPivot(float x, float y, float z)
{
    pivotX = x;
    pivotY = y;
    pivotZ = z;
}

void ModelPart::compileList(float scale)
{
    list_ = util::GlAllocationUtils::generateDisplayLists(1);
    gl::GL11::glNewList(list_, gl::GL11::GL_COMPILE);
    render::Tessellator& tessellator = render::Tessellator::INSTANCE;
    for (const Quad& face : faces_) {
        face.render(tessellator, scale);
    }
    gl::GL11::glEndList();
    compiled_ = true;
}

void ModelPart::render(float scale)
{
    if (hidden || !visible) {
        return;
    }
    if (!compiled_) {
        compileList(scale);
    }
    const auto renderChildren = [this, scale]() {
        for (ModelPart* child : children_) {
            if (child != nullptr) {
                child->render(scale);
            }
        }
    };
    if (pitch != 0.0f || yaw != 0.0f || roll != 0.0f) {
        gl::GL11::glPushMatrix();
        gl::GL11::glTranslatef(pivotX * scale, pivotY * scale, pivotZ * scale);
        if (roll != 0.0f) {
            gl::GL11::glRotatef(roll * 57.295776f, 0.0f, 0.0f, 1.0f);
        }
        if (yaw != 0.0f) {
            gl::GL11::glRotatef(yaw * 57.295776f, 0.0f, 1.0f, 0.0f);
        }
        if (pitch != 0.0f) {
            gl::GL11::glRotatef(pitch * 57.295776f, 1.0f, 0.0f, 0.0f);
        }
        gl::GL11::glCallList(list_);
        renderChildren();
        gl::GL11::glPopMatrix();
    } else if (pivotX != 0.0f || pivotY != 0.0f || pivotZ != 0.0f) {
        gl::GL11::glTranslatef(pivotX * scale, pivotY * scale, pivotZ * scale);
        gl::GL11::glCallList(list_);
        renderChildren();
        gl::GL11::glTranslatef(-pivotX * scale, -pivotY * scale, -pivotZ * scale);
    } else {
        gl::GL11::glCallList(list_);
        renderChildren();
    }
}

void ModelPart::renderForceTransform(float scale)
{
    if (hidden || !visible) {
        return;
    }
    if (!compiled_) {
        compileList(scale);
    }
    gl::GL11::glPushMatrix();
    gl::GL11::glTranslatef(pivotX * scale, pivotY * scale, pivotZ * scale);
    if (yaw != 0.0f) {
        gl::GL11::glRotatef(yaw * 57.295776f, 0.0f, 1.0f, 0.0f);
    }
    if (pitch != 0.0f) {
        gl::GL11::glRotatef(pitch * 57.295776f, 1.0f, 0.0f, 0.0f);
    }
    if (roll != 0.0f) {
        gl::GL11::glRotatef(roll * 57.295776f, 0.0f, 0.0f, 1.0f);
    }
    gl::GL11::glCallList(list_);
    gl::GL11::glPopMatrix();
}

void ModelPart::transform(float scale)
{
    if (hidden || !visible) {
        return;
    }
    if (!compiled_) {
        compileList(scale);
    }
    if (pitch != 0.0f || yaw != 0.0f || roll != 0.0f) {
        gl::GL11::glTranslatef(pivotX * scale, pivotY * scale, pivotZ * scale);
        if (roll != 0.0f) {
            gl::GL11::glRotatef(roll * 57.295776f, 0.0f, 0.0f, 1.0f);
        }
        if (yaw != 0.0f) {
            gl::GL11::glRotatef(yaw * 57.295776f, 0.0f, 1.0f, 0.0f);
        }
        if (pitch != 0.0f) {
            gl::GL11::glRotatef(pitch * 57.295776f, 1.0f, 0.0f, 0.0f);
        }
    } else if (pivotX != 0.0f || pivotY != 0.0f || pivotZ != 0.0f) {
        gl::GL11::glTranslatef(pivotX * scale, pivotY * scale, pivotZ * scale);
    }
}

void ModelPart::addChild(ModelPart& child)
{
    children_.push_back(&child);
}

} // namespace net::minecraft::client::model
