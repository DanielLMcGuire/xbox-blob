#include "scene_anim.h"

#include "anim_data.h"
#include "scene_math.h"

#include <cmath>

std::vector<int16_t> SceneAnimTables::decompressIndices(const char *p_indices, int nindices)
{
    std::vector<int16_t> out(nindices);

    if (p_indices[0] == 127)
    {
        int8_t hi = p_indices[1];
        int8_t lo = p_indices[2];
        out[0] = (int16_t)(((((int16_t)hi) & 0xff) << 8) | (((int16_t)lo) & 0xff));
        p_indices += 2;
    }
    else
    {
        out[0] = p_indices[0];
    }

    for (int i = 1; i < nindices; i++)
    {
        if (p_indices[i] == 127)
        {
            int8_t hi = p_indices[i + 1];
            int8_t lo = p_indices[i + 2];
            int16_t delta = (int16_t)(((((int16_t)hi) & 0xff) << 8) | (((int16_t)lo) & 0xff));
            out[i] = (int16_t)(out[i - 1] + delta);
            p_indices += 2;
        }
        else
        {
            out[i] = (int16_t)(out[i - 1] + p_indices[i]);
        }
    }
    return out;
}

static std::vector<Quaternion> DecompressQuats(const short *p_quat_data, const uint32_t *p_sign_data, int nquats)
{
    std::vector<Quaternion> out(nquats);
    const float f_oo_scale = 1.f / 32750.f;
    for (int i = 0; i < nquats; i++, p_quat_data += 3)
    {
        float x = ((float)p_quat_data[0]) * f_oo_scale;
        float y = ((float)p_quat_data[1]) * f_oo_scale;
        float z = ((float)p_quat_data[2]) * f_oo_scale;
        float w = sqrtf(1.f - x * x - y * y - z * z);
        int idw = i >> 5;
        int bpos = i & 31;
        bool b_pos_w = (p_sign_data[idw] & (1u << bpos)) != 0;
        if (!b_pos_w) w *= -1.f;
        out[i] = Quaternion{x, y, z, w};
    }
    return out;
}

static std::vector<Vector3> DecompressVecs(const short *p_vec_data, int nvecs)
{
    std::vector<Vector3> out(nvecs);
    for (int i = 0; i < nvecs; i++, p_vec_data += 3)
    {
        out[i].x = ((float)p_vec_data[0]) * OO_POS_ANIM_SCALE_X + POS_ANIM_DELTA_X;
        out[i].y = ((float)p_vec_data[1]) * OO_POS_ANIM_SCALE_Y + POS_ANIM_DELTA_Y;
        out[i].z = ((float)p_vec_data[2]) * OO_POS_ANIM_SCALE_Z + POS_ANIM_DELTA_Z;
    }
    return out;
}

void SceneAnimTables::create()
{
    const int numQuats = (int)(sizeof(theQuats) / sizeof(short) / 3);
    const int numPos   = (int)(sizeof(thePos)   / sizeof(short) / 3);

    quats = DecompressQuats(theQuats, theQuatSigns, numQuats);
    pos   = DecompressVecs(thePos, numPos);

    const int nRotSeq = (int)(sizeof(theRotAnimSeq) / sizeof(RotAnimSeq));
    const int nPosSeq = (int)(sizeof(thePosAnimSeq) / sizeof(PosAnimSeq));

    quatIdSeq.resize(nRotSeq);
    for (int i = 0; i < nRotSeq; i++)
        quatIdSeq[i] = decompressIndices(theRotAnimSeq[i].quatIds, MAX_ROT_SAMPLES);

    posIdSeq.resize(nPosSeq);
    for (int i = 0; i < nPosSeq; i++)
        posIdSeq[i] = decompressIndices(thePosAnimSeq[i].posIds, MAX_POS_SAMPLES);

    rotAnims.resize(nRotSeq);
    posAnims.resize(nPosSeq);
}

void SceneAnimTables::advanceTime(float fpos)
{
    const int nPosAnims = (int)posAnims.size();
    {
        float ffrac_pos = fpos * ((float)MAX_POS_SAMPLES - 2);
        int pos_id = (int)ffrac_pos;
        float ffrac = ffrac_pos - (float)pos_id;
        for (int i = 0; i < nPosAnims; i++)
        {
            const std::vector<int16_t> &seq = posIdSeq[i];
            if (fpos <= 0.f)
            {
                posAnims[i] = pos[seq[0]];
            }
            else if (fpos >= 1.f)
            {
                posAnims[i] = pos[seq[MAX_POS_SAMPLES - 1]];
            }
            else
            {
                const Vector3 &a = pos[seq[pos_id]];
                const Vector3 &b = pos[seq[pos_id + 1]];
                posAnims[i].x = a.x * (1.f - ffrac) + b.x * ffrac;
                posAnims[i].y = a.y * (1.f - ffrac) + b.y * ffrac;
                posAnims[i].z = a.z * (1.f - ffrac) + b.z * ffrac;
            }
        }
    }

    const int nRotAnims = (int)rotAnims.size();
    {
        float ffrac_pos = fpos * ((float)MAX_ROT_SAMPLES - 2);
        int pos_id = (int)ffrac_pos;
        float ffrac = ffrac_pos - (float)pos_id;
        for (int i = 0; i < nRotAnims; i++)
        {
            const std::vector<int16_t> &seq = quatIdSeq[i];
            if (fpos <= 0.f)
            {
                rotAnims[i] = quats[seq[0]];
            }
            else if (fpos >= 1.f)
            {
                rotAnims[i] = quats[seq[MAX_ROT_SAMPLES - 1]];
            }
            else
            {
                const Quaternion &a = quats[seq[pos_id]];
                const Quaternion &b = quats[seq[pos_id + 1]];
                rotAnims[i] = QuaternionSlerp(a, b, ffrac);
            }
        }
    }
}
