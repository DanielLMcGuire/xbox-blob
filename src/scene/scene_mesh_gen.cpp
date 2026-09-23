#include "scene_mesh_gen.h"

#include "raymath.h"

#include "scene_math.h"

#include <cmath>

static void AppendStripAsTriangles(std::vector<uint16_t> &out, const std::vector<uint16_t> &strip)
{
    for (size_t k = 0; k + 2 < strip.size(); k++)
    {
        uint16_t a = strip[k], b = strip[k + 1], c = strip[k + 2];
        if (a == b || b == c || a == c) continue;
        if ((k & 1) == 0)
            out.insert(out.end(), {a, b, c});
        else
            out.insert(out.end(), {a, c, b});
    }
}

static void AppendFanAsTriangles(std::vector<uint16_t> &out, const std::vector<uint16_t> &fan)
{
    for (size_t k = 1; k + 1 < fan.size(); k++)
        out.insert(out.end(), {fan[0], fan[k], fan[k + 1]});
}

static inline void SinCosf(float a, float &s, float &c) { s = sinf(a); c = cosf(a); }

SceneMesh CreateSphereMesh(const SphereVers &sv, bool bump, int ndet_bias)
{
    SceneMesh mesh;
    int nSegs = (int)(sv.nSegs >> ndet_bias);
    int nSlices = nSegs / 2;
    float fDeltaTheta = (2.f * PI) / (float)nSegs;
    float fDeltaPhi = PI / (float)(nSlices - 1);

    mesh.vertices.reserve((size_t)nSlices * nSegs);
    for (int i = 0; i < nSlices; i++)
    {
        float fPhi = fDeltaPhi * (float)i;
        float fSinPhi, fCosPhi;
        SinCosf(fPhi, fSinPhi, fCosPhi);
        float fV = fPhi / PI;
        for (int j = 0; j < nSegs; j++)
        {
            float fTheta = fDeltaTheta * (float)j;
            float fSinTheta, fCosTheta;
            SinCosf(fTheta, fSinTheta, fCosTheta);

            SceneVertex vert{};
            vert.pos = {fCosPhi, fCosTheta * fSinPhi, fSinTheta * fSinPhi};
            vert.tangent = {0.f, -fSinTheta, fCosTheta};
            vert.normal = vert.pos;
            vert.bitangent = Vector3CrossProduct(vert.tangent, vert.normal);
            if (bump)
            {
                vert.u = fTheta / (2.f * PI);
                vert.v = fV;
            }
            mesh.vertices.push_back(vert);
        }
    }

    std::vector<uint16_t> strip;
    uint16_t wCurVert = 0;
    for (int i = 0; i < nSlices - 1; i++)
    {
        uint16_t wStartVert = wCurVert;
        for (int j = 0; j < nSegs + 1; j++)
        {
            strip.push_back(wCurVert + nSegs);
            strip.push_back(wCurVert);
            if (j < nSegs - 1)
                ++wCurVert;
            else
                wCurVert = wStartVert;
        }
        wCurVert += (uint16_t)nSegs;
    }
    AppendStripAsTriangles(mesh.indices, strip);
    return mesh;
}

SceneMesh CreateCylinderMesh(const CylinderVers &cv, bool bump, int ndet_bias)
{
    SceneMesh mesh;
    int nsides = cv.nSides >> ndet_bias;
    float fDeltaZ = 1.f / (float)cv.nHeightSeg;
    float fDeltaTheta = (2.f * PI) / (float)nsides;

    std::vector<Vector3> ppts(nsides + 1);
    for (int i = 0; i < nsides + 1; i++)
    {
        float fTheta = fDeltaTheta * (float)i;
        float s, c;
        SinCosf(fTheta, s, c);
        ppts[i] = {c, s, fTheta / (2.f * PI)};
    }

    for (int i = nsides - 1; i >= 0; i--)
    {
        SceneVertex vert{};
        vert.pos = {ppts[i].x, ppts[i].y, 1.f};
        vert.tangent = {1.f, 0.f, 0.f};
        vert.bitangent = {0.f, 1.f, 0.f};
        vert.normal = {0.f, 0.f, 1.f};
        if (bump) { vert.u = ppts[i].x + 0.5f; vert.v = ppts[i].y + 0.5f; }
        mesh.vertices.push_back(vert);
    }
    for (int i = 0; i < nsides; i++)
    {
        SceneVertex vert{};
        vert.pos = {ppts[i].x, ppts[i].y, 0.f};
        vert.tangent = {1.f, 0.f, 0.f};
        vert.bitangent = {0.f, -1.f, 0.f};
        vert.normal = {0.f, 0.f, -1.f};
        if (bump) { vert.u = ppts[i].x + 0.5f; vert.v = ppts[i].y + 0.5f; }
        mesh.vertices.push_back(vert);
    }
    uint16_t bodyStart = (uint16_t)mesh.vertices.size();
    for (int i = 0; i < cv.nHeightSeg + 1; i++)
    {
        float fZ = fDeltaZ * (float)i;
        for (int j = 0; j < nsides + 1; j++)
        {
            SceneVertex vert{};
            vert.pos = {ppts[j].x, ppts[j].y, fZ};
            vert.tangent = {-ppts[j].y, ppts[j].x, 0.f};
            vert.bitangent = {0.f, 0.f, 1.f};
            vert.normal = {ppts[j].x, ppts[j].y, 0.f};
            if (bump) { vert.u = 4.0f * ppts[j].z; vert.v = fZ * 32.0f; }
            mesh.vertices.push_back(vert);
        }
    }

    std::vector<uint16_t> fanTop(nsides), fanBot(nsides);
    for (int i = 0; i < nsides; i++) fanTop[i] = (uint16_t)i;
    for (int i = 0; i < nsides; i++) fanBot[i] = (uint16_t)(nsides + i);
    AppendFanAsTriangles(mesh.indices, fanTop);
    AppendFanAsTriangles(mesh.indices, fanBot);

    int vstride = nsides + 1;
    for (int row = 0; row < cv.nHeightSeg; row++)
    {
        std::vector<uint16_t> rowStrip;
        rowStrip.reserve((size_t)(nsides + 1) * 2);
        for (int col = 0; col < nsides + 1; col++)
        {
            rowStrip.push_back(bodyStart + (uint16_t)(row * vstride + col));
            rowStrip.push_back(bodyStart + (uint16_t)((row + 1) * vstride + col));
        }
        AppendStripAsTriangles(mesh.indices, rowStrip);
    }
    return mesh;
}

SceneMesh CreateConeMesh(const ConeVers &cv, bool bump, int ndet_bias)
{
    SceneMesh mesh;
    int nsides = cv.nSides >> ndet_bias;
    float fDeltaTheta = (2.f * PI) / (float)nsides;

    std::vector<Vector3> pptsBot(nsides + 1), pptsTop(nsides + 1);
    for (int i = 0; i < nsides + 1; i++)
    {
        float fTheta = fDeltaTheta * (float)i;
        float s, c;
        SinCosf(fTheta, s, c);
        pptsBot[i] = {c * cv.fRad1, s * cv.fRad1, fTheta / (2.f * PI)};
        pptsTop[i] = {c * cv.fRad2, s * cv.fRad2, fTheta / (2.f * PI)};
    }
    float fDeltaZ = 1.f / (float)cv.nHeightSeg;

    for (int i = nsides - 1; i >= 0; i--)
    {
        SceneVertex vert{};
        vert.pos = {pptsTop[i].x, pptsTop[i].y, cv.fHeight};
        vert.tangent = {1.f, 0.f, 0.f};
        vert.bitangent = {0.f, 1.f, 0.f};
        vert.normal = {0.f, 0.f, 1.f};
        if (bump) { vert.u = pptsTop[i].x + 0.5f; vert.v = pptsTop[i].y + 0.5f; }
        mesh.vertices.push_back(vert);
    }
    for (int i = 0; i < nsides; i++)
    {
        SceneVertex vert{};
        vert.pos = {pptsBot[i].x, pptsBot[i].y, 0.f};
        vert.tangent = {1.f, 0.f, 0.f};
        vert.bitangent = {0.f, -1.f, 0.f};
        vert.normal = {0.f, 0.f, -1.f};
        if (bump) { vert.u = pptsBot[i].x + 0.5f; vert.v = pptsBot[i].y + 0.5f; }
        mesh.vertices.push_back(vert);
    }
    uint16_t bodyStart = (uint16_t)mesh.vertices.size();
    for (int i = 0; i < cv.nHeightSeg + 1; i++)
    {
        float fZ = fDeltaZ * (float)i;
        for (int j = 0; j < nsides + 1; j++)
        {
            SceneVertex vert{};
            vert.pos = {
                pptsBot[j].x + fZ * (pptsTop[j].x - pptsBot[j].x),
                pptsBot[j].y + fZ * (pptsTop[j].y - pptsBot[j].y),
                fZ * cv.fHeight};
            Vector3 s = Vector3Normalize(Vector3{-pptsTop[j].y, pptsTop[j].x, 0.f});
            Vector3 t = Vector3Normalize(Vector3{
                pptsTop[j].x - pptsBot[j].x,
                pptsTop[j].y - pptsBot[j].y,
                cv.fHeight});
            vert.tangent = s;
            vert.bitangent = t;
            vert.normal = Vector3CrossProduct(s, t);
            if (bump) { vert.u = pptsTop[j].z; vert.v = fZ; }
            mesh.vertices.push_back(vert);
        }
    }

    std::vector<uint16_t> fanTop(nsides), fanBot(nsides);
    for (int i = 0; i < nsides; i++) fanTop[i] = (uint16_t)i;
    for (int i = 0; i < nsides; i++) fanBot[i] = (uint16_t)(nsides + i);
    AppendFanAsTriangles(mesh.indices, fanTop);
    AppendFanAsTriangles(mesh.indices, fanBot);

    int vstride = nsides + 1;
    for (int row = 0; row < cv.nHeightSeg; row++)
    {
        std::vector<uint16_t> rowStrip;
        rowStrip.reserve((size_t)(nsides + 1) * 2);
        for (int col = 0; col < nsides + 1; col++)
        {
            rowStrip.push_back(bodyStart + (uint16_t)(row * vstride + col));
            rowStrip.push_back(bodyStart + (uint16_t)((row + 1) * vstride + col));
        }
        AppendStripAsTriangles(mesh.indices, rowStrip);
    }
    return mesh;
}

SceneMesh CreateBoxMesh(bool bump)
{
    SceneMesh mesh;
    static const float fUV[4][2] = {{0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}, {0.f, 0.f}};

    struct Face { Vector3 s, t, n; Vector3 p[4]; };
    static const Face faces[6] = {
        {{0,1,0},{0,0,1},{1,0,0}, {{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,0.5f,-0.5f},{0.5f,-0.5f,-0.5f}}},
        {{-1,0,0},{0,0,1},{0,1,0}, {{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f},{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f}}},
        {{0,-1,0},{0,0,1},{0,-1,0}, {{-0.5f,0.5f,0.5f},{-0.5f,-0.5f,0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f}}},
        {{1,0,0},{0,0,1},{0,-1,0}, {{-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f}}},
        {{1,0,0},{0,1,0},{0,0,1}, {{-0.5f,0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,-0.5f,0.5f},{-0.5f,-0.5f,0.5f}}},
        {{1,0,0},{0,-1,0},{0,0,-1}, {{-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f}}},
    };

    for (int i = 0; i < 6; i++)
    {
        uint16_t vStart = (uint16_t)mesh.vertices.size();
        for (int j = 0; j < 4; j++)
        {
            SceneVertex vert{};
            vert.pos = faces[i].p[j];
            vert.tangent = faces[i].s;
            vert.bitangent = faces[i].t;
            vert.normal = faces[i].n;
            if (bump) { vert.u = fUV[j][0]; vert.v = fUV[j][1]; }
            mesh.vertices.push_back(vert);
        }
        mesh.indices.insert(mesh.indices.end(), {vStart, (uint16_t)(vStart + 1), (uint16_t)(vStart + 2),
                                                  vStart, (uint16_t)(vStart + 2), (uint16_t)(vStart + 3)});
    }
    return mesh;
}

SceneMesh CreateTorusMesh(const TorusVers &tv, bool bump, int ndet_bias)
{
    SceneMesh mesh;
    int nsegs = tv.nSegs >> ndet_bias;
    int nsides = tv.nSides >> ndet_bias;
    float fDeltaTheta = (2.f * PI) / (float)nsegs;
    float fDeltaPhi = (2.f * PI) / (float)nsides;

    mesh.vertices.reserve((size_t)nsides * nsegs);
    for (int i = 0; i < nsides; i++)
    {
        float fPhi = fDeltaPhi * (float)i;
        float fSinPhi, fCosPhi;
        SinCosf(fPhi, fSinPhi, fCosPhi);
        float fRad = 1.f + (fCosPhi * tv.fRatio);
        float fV = fPhi / (2.f * PI);
        float fZ = fSinPhi * tv.fRatio;
        for (int j = 0; j < nsegs; j++)
        {
            float fTheta = fDeltaTheta * (float)j;
            float fSinTheta, fCosTheta;
            SinCosf(fTheta, fSinTheta, fCosTheta);

            SceneVertex vert{};
            vert.pos = {fCosTheta * fRad, fSinTheta * fRad, fZ};
            vert.tangent = {-fSinTheta, fCosTheta, 0.f};
            vert.bitangent = {fCosTheta * -fSinPhi, fSinTheta * -fSinPhi, fCosPhi};
            vert.normal = Vector3CrossProduct(vert.tangent, vert.bitangent);
            if (bump) { vert.u = fTheta / (2.f * PI); vert.v = fV; }
            mesh.vertices.push_back(vert);
        }
    }

    std::vector<uint16_t> strip;
    uint16_t wLoVert = 0;
    uint16_t wHiVert = (uint16_t)nsegs;
    for (int i = 0; i < nsides; i++)
    {
        uint16_t wStripStartLo = wLoVert;
        uint16_t wStripStartHi = wHiVert;
        for (int j = 0; j < nsegs + 1; j++)
        {
            strip.push_back(wLoVert);
            strip.push_back(wHiVert);
            if (j < nsegs - 1)
            {
                ++wLoVert;
                ++wHiVert;
            }
            else
            {
                wLoVert = wStripStartLo;
                wHiVert = wStripStartHi;
            }
        }
        wLoVert += (uint16_t)nsegs;
        if ((i + 1) < (nsides - 1))
            wHiVert += (uint16_t)nsegs;
        else
            wHiVert = 0;
    }
    AppendStripAsTriangles(mesh.indices, strip);
    return mesh;
}

SceneMesh CreateSurfOfRevMesh(const SurfOfRevVers &sv, bool bump, int ndet_bias)
{
    SceneMesh mesh;

    int dwPolyPts = sv.nPts;
    for (int i = 0; i < sv.nPts; i++)
        if (!(sv.pts[i].flags & sr_Smooth)) ++dwPolyPts;

    std::vector<Vector3> pSegNorms(dwPolyPts), pVerts(dwPolyPts);
    enum { nf_PrevSeg, nf_NextSeg, nf_BothSeg };
    std::vector<int> dwVertFlags(dwPolyPts);

    Vector3 vAxis = {sv.ax, sv.ay, sv.az};
    Vector3 ptOnAxis = {sv.px, sv.py, sv.pz};

    int ntot = 0;
    for (int i = 0; i < sv.nPts; i++)
    {
        pVerts[ntot] = {sv.pts[i].x, sv.pts[i].y, sv.pts[i].z};
        if (!(sv.pts[i].flags & sr_Smooth))
        {
            dwVertFlags[ntot] = nf_PrevSeg;
            pSegNorms[ntot] = {0, 0, 0};
            ntot++;
            pVerts[ntot] = {sv.pts[i].x, sv.pts[i].y, sv.pts[i].z};
            dwVertFlags[ntot] = nf_NextSeg;
        }
        else
        {
            dwVertFlags[ntot] = nf_BothSeg;
        }
        int dwNextVert = i + 1;
        if (dwNextVert == sv.nPts) dwNextVert = 0;
        Vector3 seg = {
            sv.pts[dwNextVert].x - sv.pts[i].x,
            sv.pts[dwNextVert].y - sv.pts[i].y,
            sv.pts[dwNextVert].z - sv.pts[i].z};
        Vector3 axisToPt = {
            sv.pts[dwNextVert].x - ptOnAxis.x,
            sv.pts[dwNextVert].y - ptOnAxis.y,
            sv.pts[dwNextVert].z - ptOnAxis.z};
        Vector3 tang = Vector3CrossProduct(vAxis, axisToPt);
        pSegNorms[ntot] = Vector3Normalize(Vector3CrossProduct(tang, seg));
        ntot++;
    }

    std::vector<Vector3> pVertNorms(dwPolyPts);
    for (int i = 0; i < dwPolyPts; i++)
    {
        switch (dwVertFlags[i])
        {
        case nf_PrevSeg:
            pVertNorms[i] = pSegNorms[(i + dwPolyPts - 1) % dwPolyPts];
            break;
        case nf_NextSeg:
            pVertNorms[i] = pSegNorms[i];
            break;
        case nf_BothSeg:
            pVertNorms[i] = Vector3Normalize(Vector3Add(
                pSegNorms[(i + dwPolyPts - 1) % dwPolyPts], pSegNorms[i]));
            break;
        }
    }

    int nsegs = sv.nSegs >> ndet_bias;
    float fDeltaTheta = (2.f * PI) / (float)nsegs;
    float fDeltaV = 1.f / (float)(sv.nPts - 1);

    mesh.vertices.reserve((size_t)(nsegs + 1) * dwPolyPts);
    for (int i = 0; i < nsegs + 1; i++)
    {
        float fTheta = fDeltaTheta * (float)i;
        Quaternion quat = QuaternionFromAxisAngle(vAxis, fTheta);
        float fU = fTheta / (2.f * PI);

        unsigned unique_vert_count = 0;
        for (int j = 0; j < dwPolyPts; j++)
        {
            float fV = fDeltaV * (float)unique_vert_count;
            if (dwVertFlags[j] != nf_PrevSeg) ++unique_vert_count;

            Vector3 pt = Vector3Subtract(pVerts[j], ptOnAxis);
            SceneVertex vert{};
            vert.pos = Vector3Add(RotateRH(pt, quat), ptOnAxis);

            Vector3 tang = Vector3CrossProduct(vAxis, pt);
            vert.tangent = Vector3Normalize(RotateRH(tang, quat));
            vert.normal = RotateRH(pVertNorms[j], quat);
            vert.bitangent = Vector3CrossProduct(vert.normal, vert.tangent);
            if (bump) { vert.u = fU * 10.f; vert.v = fV * 10.f; }
            mesh.vertices.push_back(vert);
        }
    }

    std::vector<uint16_t> strip;
    uint16_t wLeftVert = 0;
    uint16_t wRightVert = (uint16_t)dwPolyPts;
    for (int i = 0; i < nsegs; i++)
    {
        uint16_t wStartStripRight = wRightVert;
        uint16_t wStartStripLeft = wLeftVert;
        for (int j = 0; j < dwPolyPts + 1; j++)
        {
            strip.push_back(wRightVert);
            strip.push_back(wLeftVert);
            if (j < dwPolyPts - 1)
            {
                ++wRightVert;
                ++wLeftVert;
            }
            else
            {
                wRightVert = wStartStripRight;
                wLeftVert = wStartStripLeft;
            }
        }
        wLeftVert = wRightVert;
        wRightVert += (uint16_t)dwPolyPts;
    }
    AppendStripAsTriangles(mesh.indices, strip);
    return mesh;
}
