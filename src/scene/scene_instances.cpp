#include "scene_instances.h"
#include "scene_data.h"
#include "scene_math.h"

namespace
{
constexpr float LO_Z_VAL = 0.f;

Vector3 DecodeTrans(short tx, short ty, short tz)
{
    return {
        (float)tx * OO_PRIM_TRANS_SCALE_X + PRIM_TRANS_DELTA_X,
        (float)ty * OO_PRIM_TRANS_SCALE_Y + PRIM_TRANS_DELTA_Y,
        (float)tz * OO_PRIM_TRANS_SCALE_Z + PRIM_TRANS_DELTA_Z,
    };
}

bool ComputeBHiZ(const SceneAnimTables &anim, Vector3 baseTrans, int idxRotAnim, int idxPosAnim)
{
    Vector3 v = baseTrans;
    if (idxRotAnim >= 0)
    {
        const Quaternion &q0 = anim.quats[anim.quatIdSeq[idxRotAnim][0]];
        v = RotateLH(v, q0);
    }
    if (idxPosAnim >= 0)
    {
        const Vector3 &p0 = anim.pos[anim.posIdSeq[idxPosAnim][0]];
        v = Vector3Add(v, p0);
    }
    return v.z > LO_Z_VAL;
}
} // namespace

std::vector<ScenePrimitiveSet> BuildSceneInstances(const SceneAnimTables &anim)
{
    std::vector<ScenePrimitiveSet> sets(pt_NoTypes);

    {
        ScenePrimitiveSet &set = sets[pt_Sphere];
        set.type = pt_Sphere;
        for (auto &s : theSphereInsts)
        {
            PrimitiveInstance inst;
            inst.idxVersion = (uint16_t)s.idVersion;
            inst.idxPosAnim = s.idPosAnim;
            inst.idxRotAnim = s.idRotAnim;
            inst.baseRot = QuaternionIdentity();
            inst.baseTrans = DecodeTrans(s.tx, s.ty, s.tz);
            inst.scale = {s.fRad, s.fRad, s.fRad};
            inst.bHiZ = ComputeBHiZ(anim, inst.baseTrans, inst.idxRotAnim, inst.idxPosAnim);
            set.instances.push_back(inst);
        }
    }
    {
        ScenePrimitiveSet &set = sets[pt_Cylinder];
        set.type = pt_Cylinder;
        for (auto &c : theCylinderInsts)
        {
            PrimitiveInstance inst;
            inst.idxVersion = (uint16_t)c.idVersion;
            inst.idxPosAnim = c.idPosAnim;
            inst.idxRotAnim = c.idRotAnim;
            inst.baseRot = anim.quats[(uint8_t)c.idQuat];
            inst.baseTrans = DecodeTrans(c.tx, c.ty, c.tz);
            inst.scale = {c.fRad, c.fRad, c.fHalfHeight * 2.f};
            inst.bHiZ = ComputeBHiZ(anim, inst.baseTrans, inst.idxRotAnim, inst.idxPosAnim);
            set.instances.push_back(inst);
        }
    }
    {
        ScenePrimitiveSet &set = sets[pt_Cone];
        set.type = pt_Cone;
        for (auto &c : theConeInsts)
        {
            PrimitiveInstance inst;
            inst.idxVersion = (uint16_t)c.idVersion;
            inst.idxPosAnim = c.idPosAnim;
            inst.idxRotAnim = c.idRotAnim;
            inst.baseRot = anim.quats[(uint8_t)c.idQuat];
            inst.baseTrans = DecodeTrans(c.tx, c.ty, c.tz);
            inst.scale = {1, 1, 1};
            inst.bHiZ = ComputeBHiZ(anim, inst.baseTrans, inst.idxRotAnim, inst.idxPosAnim);
            set.instances.push_back(inst);
        }
    }
    {
        ScenePrimitiveSet &set = sets[pt_Box];
        set.type = pt_Box;
        for (auto &b : theBoxInsts)
        {
            PrimitiveInstance inst;
            inst.idxVersion = 0;
            inst.idxPosAnim = b.idPosAnim;
            inst.idxRotAnim = b.idRotAnim;
            inst.baseRot = anim.quats[(uint8_t)b.idQuat];
            inst.baseTrans = DecodeTrans(b.tx, b.ty, b.tz);
            inst.scale = {b.fWidth, b.fLen, b.fHeight};
            inst.bHiZ = ComputeBHiZ(anim, inst.baseTrans, inst.idxRotAnim, inst.idxPosAnim);
            set.instances.push_back(inst);
        }
    }
    {
        ScenePrimitiveSet &set = sets[pt_Torus];
        set.type = pt_Torus;
        for (auto &t : theTorusInsts)
        {
            PrimitiveInstance inst;
            inst.idxVersion = (uint16_t)t.idVersion;
            inst.idxPosAnim = t.idPosAnim;
            inst.idxRotAnim = t.idRotAnim;
            inst.baseRot = anim.quats[(uint8_t)t.idQuat];
            inst.baseTrans = DecodeTrans(t.tx, t.ty, t.tz);
            inst.scale = {t.fRad1, t.fRad1, t.fRad1};
            inst.bHiZ = ComputeBHiZ(anim, inst.baseTrans, inst.idxRotAnim, inst.idxPosAnim);
            set.instances.push_back(inst);
        }
    }
    {
        ScenePrimitiveSet &set = sets[pt_SurfOfRev];
        set.type = pt_SurfOfRev;
        for (auto &s : theSurfOfRevInsts)
        {
            PrimitiveInstance inst;
            inst.idxVersion = (uint16_t)s.idVersion;
            inst.idxPosAnim = s.idPosAnim;
            inst.idxRotAnim = s.idRotAnim;
            inst.baseRot = anim.quats[(uint8_t)s.idQuat];
            inst.baseTrans = DecodeTrans(s.tx, s.ty, s.tz);
            inst.scale = {1, 1, 1};
            inst.bHiZ = ComputeBHiZ(anim, inst.baseTrans, inst.idxRotAnim, inst.idxPosAnim);
            set.instances.push_back(inst);
        }
    }
    return sets;
}
