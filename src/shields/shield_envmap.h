#pragma once

#include "raylib.h"

class IntroSceneRenderer;

class Blob;

class ShieldEnvMap
{
public:
    ShieldEnvMap() = default;
    ~ShieldEnvMap() { unload(); }
    ShieldEnvMap(const ShieldEnvMap &) = delete;
    ShieldEnvMap &operator=(const ShieldEnvMap &) = delete;

    bool create(IntroSceneRenderer &scene, const Blob &blob, Vector3 tint);
    void unload();

    unsigned int id() const { return cubemap; }

    static void FaceBasis(int face, Vector3 &forward, Vector3 &up);

private:
    unsigned int cubemap = 0;
};
