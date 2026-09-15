#pragma once
#include "raylib.h"
#include "scene_prim_types.h"
#include <cstdint>
#include <vector>

struct SceneVertex
{
    Vector3 pos;
    Vector3 tangent;
    Vector3 bitangent;
    Vector3 normal;
    float u = 0.f, v = 0.f;
};

struct SceneMesh
{
    std::vector<SceneVertex> vertices;
    std::vector<uint16_t> indices;
};

SceneMesh CreateSphereMesh(const SphereVers &v, bool bump, int ndet_bias);
SceneMesh CreateCylinderMesh(const CylinderVers &v, bool bump, int ndet_bias);
SceneMesh CreateConeMesh(const ConeVers &v, bool bump, int ndet_bias);
SceneMesh CreateBoxMesh(bool bump);
SceneMesh CreateTorusMesh(const TorusVers &v, bool bump, int ndet_bias);
SceneMesh CreateSurfOfRevMesh(const SurfOfRevVers &v, bool bump, int ndet_bias);
