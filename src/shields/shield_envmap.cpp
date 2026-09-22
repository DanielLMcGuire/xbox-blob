#include "shield_envmap.h"

#include "shield_config.h"
#include "../scene/scene_renderer.h"
#include "../blob/blob.h"

#include <rlgl.h>
#include "raymath.h"

void ShieldEnvMap::FaceBasis(int face, Vector3 &forward, Vector3 &up)
{
    switch (face)
    {
    case 0: forward = {+1, 0, 0}; up = {0, -1, 0}; break; // +X
    case 1: forward = {-1, 0, 0}; up = {0, -1, 0}; break; // -X
    case 2: forward = {0, +1, 0}; up = {0, 0, +1}; break; // +Y
    case 3: forward = {0, -1, 0}; up = {0, 0, -1}; break; // -Y
    case 4: forward = {0, 0, +1}; up = {0, -1, 0}; break; // +Z
    default: forward = {0, 0, -1}; up = {0, -1, 0}; break; // -Z
    }
}

bool ShieldEnvMap::create(IntroSceneRenderer &scene, const Blob &blob, Vector3 tint)
{
    Vector3 center = blob.GetCenter();
    using namespace ShieldConfig;

    if (cubemap) return true;

    rlDrawRenderBatchActive();

    cubemap = rlLoadTextureCubemap(nullptr, ENV_MAP_SIZE, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    const unsigned int depth = rlLoadTextureDepth(ENV_MAP_SIZE, ENV_MAP_SIZE, true);
    const unsigned int fbo = rlLoadFramebuffer();
    rlFramebufferAttach(fbo, depth, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

    scene.advanceTime(ENV_SCENE_ANIM_POS);
    const Matrix proj = MatrixPerspective(PI * 0.5f, 1.0f, ENV_NEAR, ENV_FAR);

    bool ok = cubemap != 0 && fbo != 0;
    for (int face = 0; ok && face < 6; face++)
    {
        rlFramebufferAttach(fbo, cubemap, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + face, 0);
        if (!rlFramebufferComplete(fbo))
        {
            ok = false;
            break;
        }


        Vector3 forward, up;
        FaceBasis(face, forward, up);
        const Matrix view = MatrixLookAt(center, Vector3Add(center, forward), up);

        rlEnableFramebuffer(fbo);
        rlViewport(0, 0, ENV_MAP_SIZE, ENV_MAP_SIZE);
        rlClearColor(0, 0, 0, 255);
        rlClearScreenBuffers();

        scene.renderFixedLight(view, proj, center, center, tint);

        rlDrawRenderBatchActive();
        rlDisableFramebuffer();
    }

    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
    if (fbo) rlUnloadFramebuffer(fbo);

    if (!ok)
    {
        TraceLog(LOG_ERROR, "SHIELDS: couldn't build the reflection cube map");
        unload();
    }
    return ok;
}

void ShieldEnvMap::unload()
{
    if (cubemap) rlUnloadTexture(cubemap);
    cubemap = 0;
}
