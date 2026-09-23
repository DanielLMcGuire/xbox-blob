#pragma once

#include "raylib.h"
#include "raymath.h"

#include "scene_math.h"
#include "scene_prim_types.h"
#include "scene_anim.h"

#include <vector>

struct PrimitiveInstance
{
    uint16_t idxVersion = 0;
    int idxPosAnim = -1;
    int idxRotAnim = -1;
    Quaternion baseRot = QuaternionIdentity();
    Vector3 baseTrans{};
    Vector3 scale{1, 1, 1};
    bool bHiZ = false;

    Matrix worldMatrix(const SceneAnimTables &anim) const
    {
        Matrix m = MatrixScale(scale.x, scale.y, scale.z);
        m = MatrixMultiply(m, MatrixRotateLH(baseRot));
        m = MatrixMultiply(m, MatrixTranslate(baseTrans.x, baseTrans.y, baseTrans.z));
        if (idxRotAnim >= 0)
            m = MatrixMultiply(m, MatrixRotateLH(anim.rotAnims[idxRotAnim]));
        if (idxPosAnim >= 0)
        {
            const Vector3 &p = anim.posAnims[idxPosAnim];
            m = MatrixMultiply(m, MatrixTranslate(p.x, p.y, p.z));
        }
        return m;
    }
};

struct ScenePrimitiveSet
{
    PrimitiveTypes type = pt_NoTypes;
    std::vector<PrimitiveInstance> instances;
};

std::vector<ScenePrimitiveSet> BuildSceneInstances(const SceneAnimTables &anim);
