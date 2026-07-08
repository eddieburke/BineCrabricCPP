#include "net/minecraft/client/render/culling/Frustum.hpp"

#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render {
Frustum& Frustum::getInstance() {
    static Frustum instance;
    return instance;
}

void Frustum::normalize(float plane[4]) {
    const float length = MathHelper::sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    if (length == 0.0f) {
        return;
    }
    plane[0] /= length;
    plane[1] /= length;
    plane[2] /= length;
    plane[3] /= length;
}

void Frustum::compute() {
    gl::getFloatv(gl::matrix_::ProjectionMatrix, projectionMatrix.data());
    gl::getFloatv(gl::matrix_::ModelViewMatrix, modelMatrix.data());
    clipMatrix[0] = modelMatrix[0] * projectionMatrix[0] + modelMatrix[1] * projectionMatrix[4] +
                    modelMatrix[2] * projectionMatrix[8] + modelMatrix[3] * projectionMatrix[12];
    clipMatrix[1] = modelMatrix[0] * projectionMatrix[1] + modelMatrix[1] * projectionMatrix[5] +
                    modelMatrix[2] * projectionMatrix[9] + modelMatrix[3] * projectionMatrix[13];
    clipMatrix[2] = modelMatrix[0] * projectionMatrix[2] + modelMatrix[1] * projectionMatrix[6] +
                    modelMatrix[2] * projectionMatrix[10] + modelMatrix[3] * projectionMatrix[14];
    clipMatrix[3] = modelMatrix[0] * projectionMatrix[3] + modelMatrix[1] * projectionMatrix[7] +
                    modelMatrix[2] * projectionMatrix[11] + modelMatrix[3] * projectionMatrix[15];
    clipMatrix[4] = modelMatrix[4] * projectionMatrix[0] + modelMatrix[5] * projectionMatrix[4] +
                    modelMatrix[6] * projectionMatrix[8] + modelMatrix[7] * projectionMatrix[12];
    clipMatrix[5] = modelMatrix[4] * projectionMatrix[1] + modelMatrix[5] * projectionMatrix[5] +
                    modelMatrix[6] * projectionMatrix[9] + modelMatrix[7] * projectionMatrix[13];
    clipMatrix[6] = modelMatrix[4] * projectionMatrix[2] + modelMatrix[5] * projectionMatrix[6] +
                    modelMatrix[6] * projectionMatrix[10] + modelMatrix[7] * projectionMatrix[14];
    clipMatrix[7] = modelMatrix[4] * projectionMatrix[3] + modelMatrix[5] * projectionMatrix[7] +
                    modelMatrix[6] * projectionMatrix[11] + modelMatrix[7] * projectionMatrix[15];
    clipMatrix[8] = modelMatrix[8] * projectionMatrix[0] + modelMatrix[9] * projectionMatrix[4] +
                    modelMatrix[10] * projectionMatrix[8] + modelMatrix[11] * projectionMatrix[12];
    clipMatrix[9] = modelMatrix[8] * projectionMatrix[1] + modelMatrix[9] * projectionMatrix[5] +
                    modelMatrix[10] * projectionMatrix[9] + modelMatrix[11] * projectionMatrix[13];
    clipMatrix[10] = modelMatrix[8] * projectionMatrix[2] + modelMatrix[9] * projectionMatrix[6] +
                     modelMatrix[10] * projectionMatrix[10] + modelMatrix[11] * projectionMatrix[14];
    clipMatrix[11] = modelMatrix[8] * projectionMatrix[3] + modelMatrix[9] * projectionMatrix[7] +
                     modelMatrix[10] * projectionMatrix[11] + modelMatrix[11] * projectionMatrix[15];
    clipMatrix[12] = modelMatrix[12] * projectionMatrix[0] + modelMatrix[13] * projectionMatrix[4] +
                     modelMatrix[14] * projectionMatrix[8] + modelMatrix[15] * projectionMatrix[12];
    clipMatrix[13] = modelMatrix[12] * projectionMatrix[1] + modelMatrix[13] * projectionMatrix[5] +
                     modelMatrix[14] * projectionMatrix[9] + modelMatrix[15] * projectionMatrix[13];
    clipMatrix[14] = modelMatrix[12] * projectionMatrix[2] + modelMatrix[13] * projectionMatrix[6] +
                     modelMatrix[14] * projectionMatrix[10] + modelMatrix[15] * projectionMatrix[14];
    clipMatrix[15] = modelMatrix[12] * projectionMatrix[3] + modelMatrix[13] * projectionMatrix[7] +
                     modelMatrix[14] * projectionMatrix[11] + modelMatrix[15] * projectionMatrix[15];
    frustum[0][0] = clipMatrix[3] - clipMatrix[0];
    frustum[0][1] = clipMatrix[7] - clipMatrix[4];
    frustum[0][2] = clipMatrix[11] - clipMatrix[8];
    frustum[0][3] = clipMatrix[15] - clipMatrix[12];
    normalize(frustum[0].data());
    frustum[1][0] = clipMatrix[3] + clipMatrix[0];
    frustum[1][1] = clipMatrix[7] + clipMatrix[4];
    frustum[1][2] = clipMatrix[11] + clipMatrix[8];
    frustum[1][3] = clipMatrix[15] + clipMatrix[12];
    normalize(frustum[1].data());
    frustum[2][0] = clipMatrix[3] + clipMatrix[1];
    frustum[2][1] = clipMatrix[7] + clipMatrix[5];
    frustum[2][2] = clipMatrix[11] + clipMatrix[9];
    frustum[2][3] = clipMatrix[15] + clipMatrix[13];
    normalize(frustum[2].data());
    frustum[3][0] = clipMatrix[3] - clipMatrix[1];
    frustum[3][1] = clipMatrix[7] - clipMatrix[5];
    frustum[3][2] = clipMatrix[11] - clipMatrix[9];
    frustum[3][3] = clipMatrix[15] - clipMatrix[13];
    normalize(frustum[3].data());
    frustum[4][0] = clipMatrix[3] - clipMatrix[2];
    frustum[4][1] = clipMatrix[7] - clipMatrix[6];
    frustum[4][2] = clipMatrix[11] - clipMatrix[10];
    frustum[4][3] = clipMatrix[15] - clipMatrix[14];
    normalize(frustum[4].data());
    frustum[5][0] = clipMatrix[3] + clipMatrix[2];
    frustum[5][1] = clipMatrix[7] + clipMatrix[6];
    frustum[5][2] = clipMatrix[11] + clipMatrix[10];
    frustum[5][3] = clipMatrix[15] + clipMatrix[14];
    normalize(frustum[5].data());
}
}  // namespace net::minecraft::client::render
