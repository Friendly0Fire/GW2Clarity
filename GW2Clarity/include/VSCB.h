#pragma once

#include "ShaderManager.h"

namespace GW2Radial
{
struct VSCB
{
    fVector4 spriteDimensions;
    glm::mat4x4 tiltMatrix;
    f32 spriteZ;
};

ConstantBuffer<VSCB>& GetVSCB();
} // namespace GW2Radial