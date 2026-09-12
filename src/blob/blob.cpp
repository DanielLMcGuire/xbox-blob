#include "blob.h"
#include "../defines.h"
#include <rlgl.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "../util/x86-emu.h"

Blob* g_Blob = nullptr;

static QRand g_BlobRand;

constexpr int32_t LLI_RAND_MAX = 0x00010000;
constexpr int32_t LLI_RAND_MASK = 0x0000FFFF;

float BlobRandom01()
{
    static const float mul = 1.0f / (float)LLI_RAND_MAX;
    return (float)(g_BlobRand.Rand() & LLI_RAND_MASK) * mul;
}

float BlobRandom11()
{
    static const float mul = 2.0f / (float)LLI_RAND_MAX;
    return (float)(g_BlobRand.Rand() & LLI_RAND_MASK) * mul - 1.0f;
}

static float QuickLength(const Vector3& v)
{
    float h = fabsf(v.x), m = fabsf(v.y), l = fabsf(v.z), t;
    if (m > h) { t = m; m = h; h = t; }
    if (l > m) { t = l; l = m; m = t; }
    if (m > h) { t = m; m = h; h = t; }
    return 1.043388475f * (h + 0.34375f * m + 0.25f * l);
}

static void QuickNormalize(Vector3* v)
{
    float qlen = QuickLength(*v);
    if (qlen < 0.000001f) return;
    float ooQlen = 1.0f / qlen;
    v->x *= ooQlen; v->y *= ooQlen; v->z *= ooQlen;
}

static Vector3 AddScaled(Vector3 target, const Vector3& src, float scale)
{
    target.x += src.x * scale;
    target.y += src.y * scale;
    target.z += src.z * scale;
    return target;
}

constexpr float WOBBLE_ACCEL = 1000.0f;

bool Bloblet::Update(float elapsedTime, float dt)
{
    wobble = std::min(2.0f, std::max(0.5f, wobble + wobbleDirection * dt));
    if (wobbleDirection > 0.0f)
    {
        if ((wobble < 0.95f) || (wobble > 1.0f))
            wobbleDirection -= (wobble - 1.0f) * dt * WOBBLE_ACCEL;
    }
    else
    {
        if ((wobble < 1.0f) || (wobble > 1.05f))
            wobbleDirection -= (wobble - 1.0f) * dt * WOBBLE_ACCEL;
    }

    float timeProg = std::max(0.0f, (elapsedTime - BLOB_STATIC_END_TIME) * OO_MAX_INTENSITY_DELTA);

    float t = timeMultiple * (elapsedTime - startTime);
    t *= 1.4f * (1.0f + elapsedTime / 10.0f);

    float s = sinf(t);
    float sm = fabsf(s);
    sm = 1.0f - (1.0f - sm) * sqrtf(1.0f - sm);
    s = (s > 0.0f) ? sm : -sm;

    curDist = maxDist * s * timeProg;
    farSide = (curDist < 0.0f);

    position = AddScaled(g_Blob->GetCenter(), direction, curDist);

    return (fabsf(curDist) + radius < g_Blob->GetRadius() * 0.5f);
}

void BlobBump::RecalculateFacesOfInterest()
{
    facesOfInterest =
        ((direction.x - radius < -0.57735f) ? 0x0001 : 0) +
        ((direction.y - radius < -0.57735f) ? 0x0002 : 0) +
        ((direction.z - radius < -0.57735f) ? 0x0004 : 0) +
        ((direction.x + radius > +0.57735f) ? 0x0008 : 0) +
        ((direction.y + radius > +0.57735f) ? 0x0010 : 0) +
        ((direction.z + radius > +0.57735f) ? 0x0020 : 0);
}

bool BlobBump::Create(float curTime, Bloblet* bloblet)
{
    if (curTime < 0.0f) myBloblet = nullptr;

    direction = { BlobRandom11(), BlobRandom11(), BlobRandom11() };
    if (Vector3LengthSqr(direction) < 0.001f)
        direction = { BlobRandom11(), BlobRandom11(), 1.0f };

    position = { 0, 0, 0 };
    QuickNormalize(&direction);

    float timeProg = std::max(0.0f, (curTime - BLOB_STATIC_END_TIME) * OO_MAX_INTENSITY_DELTA);
    float radMagRand = BlobRandom01();

    radius = radMagRand * 0.4f + 0.4f;
    radius2 = radius * radius;
    ooRadius2 = 1.0f / radius2;
    magnitude = 0.0f;

    RecalculateFacesOfInterest();

    startTime = curTime + 0.4f * BlobRandom01();

    maxMagnitude = (1.0f - radMagRand) * 0.5f + 0.2f;
    maxMagnitude *= 0.5f + 0.5f * timeProg;

    if (!myBloblet) myBloblet = bloblet;
    if (myBloblet)
    {
        float mainRad = g_Blob->GetRadius();
        myBloblet->radius = (BlobRandom01() + 1.0f) * 0.25f * mainRad * radius;
        myBloblet->direction = direction;

        myBloblet->maxDist = mainRad * (5.0f + BlobRandom11() * 2.0f);
        myBloblet->maxDist *= 0.6f;

        myBloblet->startTime = (curTime < -1.0f) ? -BlobRandom01() * 0.3f : curTime;

        float period = 0.8f + 0.3f * BlobRandom01();
        period *= 1.0f / 0.6f;
        myBloblet->timeMultiple = 2.0f * PI / period;

        myBloblet->wobble = 1.2f;
        myBloblet->wobbleDirection = 0.0f;

        stillAttachedToBloblet = (curTime - startTime < 0.4f * period);

        myBloblet->Update(curTime, 0.0f);
        Update(curTime, 0.0f, nullptr);
    }
    else
    {
        float sequenceLen = maxMagnitude * 0.3f + BlobRandom01() * 0.3f;
        timeMul = PI / sequenceLen;
        timeMul *= timeProg * 0.2f + 0.8f;

        if (curTime < -1.0f) startTime = -BlobRandom01() * PI / timeMul;
    }

    return (myBloblet != nullptr);
}

bool BlobBump::Update(float elapsedTime, float dt, Bloblet* bloblet)
{
    if (myBloblet)
    {
        float bMag = (fabsf(myBloblet->curDist) + myBloblet->radius) / g_Blob->GetRadius();
        magnitude = std::min(2.0f, std::max(0.0f, bMag - 1.0f));

        if (stillAttachedToBloblet)
        {
            if (magnitude > 0.8f)
            {
                stillAttachedToBloblet = false;
                maxMagnitude = magnitude;
                float sequenceLen = 0.3f * magnitude;
                timeMul = 2.0f * PI / sequenceLen;
                startTime = elapsedTime - 0.25f * sequenceLen;
                myBloblet->wobble = std::max(0.6f, std::min(0.8f, magnitude - 0.5f));
                myBloblet->wobbleDirection = 0.0f;
            }
            else
            {
                if ((Vector3DotProduct(direction, myBloblet->direction) < 0.0f) != myBloblet->farSide)
                {
                    direction = Vector3Scale(direction, -1.0f);
                    position = direction;
                    RecalculateFacesOfInterest();
                }
                return false;
            }
        }

        if (!stillAttachedToBloblet)
        {
            if (bMag < 0.9f)
                stillAttachedToBloblet = true;
        }
    }

    float t = (elapsedTime - startTime) * timeMul;
    if (t > PI)
    {
        if (myBloblet == nullptr)
            return Create(elapsedTime, bloblet);
        magnitude = 0.0f;
        return false;
    }
    if (t < 0.0f) return false;

    float sinVal = sinf(t);
    magnitude = maxMagnitude * sinVal;
    position = direction;
    RecalculateFacesOfInterest();

    return false;
}

void Blob::Init()
{
    for (auto& b : blobBumps) b.Init();
    for (auto& b : bloblets) b.Init();

    numBlobBumps = 0;
    numBloblets = 0;

    color = { 0.25f, 1.0f, 0.15f, 1.0f };
    position = { 0, 0, 0 };
    scale = { 1, 1, 1 };
    radius = 2.3f;

    g_Blob = this;
}

void Blob::BuildCubeSphere(int resolution, std::vector<Vector3>& outPositions,
                           std::vector<uint16_t>& outIndices)
{
    int subdiv = std::max(1, resolution / 2);
    float step = 2.0f / static_cast<float>(subdiv);
    int stride = subdiv + 1;

    outPositions.clear();
    outPositions.resize(static_cast<size_t>(6) * stride * stride);

    for (int k = 0; k < 6; k++)
    {
        for (int j = 0; j <= subdiv; j++)
        {
            for (int i = 0; i <= subdiv; i++)
            {
                float fu = (i == subdiv) ? 1.0f : (-1.0f + step * (float)i);
                float fv = (j == subdiv) ? 1.0f : (-1.0f + step * (float)j);

                Vector3 pos{};
                switch (k)
                {
                    case 0: pos = { -1.0f, -fu, +fv }; break;
                    case 1: pos = { +fv, -1.0f, -fu }; break;
                    case 2: pos = { -fu, +fv, -1.0f }; break;
                    case 3: pos = { +1.0f, +fu, +fv }; break;
                    case 4: pos = { +fv, +1.0f, +fu }; break;
                    case 5: pos = { +fu, +fv, +1.0f }; break;
                }
                pos = Vector3Normalize(pos);

                int idx = k * stride * stride + j * stride + i;
                outPositions[idx] = pos;
            }
        }
    }

    outIndices.clear();
    outIndices.reserve((size_t)6 * subdiv * subdiv * 6);
    for (int k = 0; k < 6; k++)
    {
        uint16_t faceBase = (uint16_t)(k * stride * stride);
        for (int j = 0; j < subdiv; j++)
        {
            for (int i = 0; i < subdiv; i++)
            {
                uint16_t a = faceBase + (uint16_t)(j * stride + i);
                uint16_t b = faceBase + (uint16_t)(j * stride + i + 1);
                uint16_t c = faceBase + (uint16_t)((j + 1) * stride + i + 1);
                uint16_t d = faceBase + (uint16_t)((j + 1) * stride + i);

                outIndices.push_back(a);
                outIndices.push_back(b);
                outIndices.push_back(d);

                outIndices.push_back(b);
                outIndices.push_back(c);
                outIndices.push_back(d);
            }
        }
    }
}

void Blob::UploadStaticMesh(const std::vector<Vector3>& positions, const std::vector<uint16_t>& indices,
                             unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO)
{
    outVAO = rlLoadVertexArray();
    rlEnableVertexArray(outVAO);

    outVBO = rlLoadVertexBuffer(positions.data(), (int)(positions.size() * sizeof(Vector3)), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, sizeof(Vector3), 0);
    rlEnableVertexAttribute(0);

    outEBO = rlLoadVertexBufferElement(indices.data(), (int)(indices.size() * sizeof(uint16_t)), false);

    rlDisableVertexArray();
}

template <int width>
constexpr int ComputeGlowScale()
{
    static_assert(
        width > 0 &&
        (4096 % width) == 0 &&
        (((4096 / width) & ((4096 / width) - 1)) == 0),
        "4096/WIDTH must be a power of two"
    );

    int tmp = 4096 / width;
    int result = 1;

    while (tmp != 1)
    {
        ++result;
        tmp >>= 1;
    }

    return result;
}

void Blob::BuildGlowTexture()
{
    constexpr int WIDTH = 256;
    constexpr int HEIGHT = 256;

    constexpr uint32_t NOISE = 0;
    constexpr uint32_t INITIAL_SEED = 12345;

    Image img = GenImageColor(WIDTH, HEIGHT, BLANK);
    uint32_t* pPixels = static_cast<uint32_t*>(img.data);

    int scale = ComputeGlowScale<WIDTH>();
    int cntrx = (WIDTH - 1) / 2;
    int cntry = (HEIGHT - 1) / 2;
    uint32_t seed = INITIAL_SEED;

    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            uint32_t* pCurrentPixel = &pPixels[y * WIDTH + x];

#if defined(USE_ASM_MSVC_X86)
            _asm {
                mov     ecx, scale
                mov     eax, x
                mov     ebx, y
                sub     eax, cntrx
                sub     ebx, cntry
                sal     eax, cl
                imul    eax
                sal     ebx, cl
                xchg    eax, ebx
                mov     edi, pCurrentPixel
                imul    eax
                mov     edx, 16777216
                add     ebx, eax
                sub     edx, ebx
                jnc     noOverflow1
                xor     edx, edx
            noOverflow1:
                mov     ebx, edx
                mov     eax, ebx
                mul     NOISE
                mov     ecx, seed
                mov     eax, edx
                mov     edx, ecx
                rcl     ecx, 13
                sub     edx, 11
                sub     ecx, edx
                mov     seed, ecx
                mul     ecx
                shl     edx, 15
                sub     ebx, edx	
                jge     bxOk1
                xor     ebx, ebx
            bxOk1:
                and     ebx, 0x1ff0000
                rcl     ebx, 8
                sbb     ebx, 0
                mov     eax, ebx
                shr     eax, 24
                mul     al
                mul     eax
                shr     eax, 16
                mul     eax
                shr     eax, 16
                and     eax, 0xff00
                mov     ecx, eax
                shr     ecx, 8
                or      ecx, eax
                mov     eax, ecx
                shl     ecx, 16
                or      ecx, eax
                mov     [edi], ecx
            }
#elif defined(USE_ASM_GCC_X86)
            asm volatile (
                "movl %[scale], %%ecx\n\t"
                "movl %[x], %%eax\n\t"
                "movl %[y], %%ebx\n\t"
                "subl %[cntrx], %%eax\n\t"
                "subl %[cntry], %%ebx\n\t"
                "sall %%cl, %%eax\n\t"
                "imull %%eax, %%eax\n\t"
                "sall %%cl, %%ebx\n\t"
                "xchgl %%eax, %%ebx\n\t"
                "imull %%eax, %%eax\n\t"
                "movl $16777216, %%edx\n\t"
                "addl %%eax, %%ebx\n\t"
                "subl %%ebx, %%edx\n\t"
                "jnc 1f\n\t"
                "xorl %%edx, %%edx\n\t"
                "1:\n\t"
                "movl %%edx, %%ebx\n\t"
                "movl %%ebx, %%eax\n\t"
                "mull %[noise]\n\t"
                "movl %[seed], %%ecx\n\t"
                "movl %%edx, %%eax\n\t"
                "movl %%ecx, %%edx\n\t"
                "rcll $13, %%ecx\n\t"
                "subl $11, %%edx\n\t"
                "subl %%edx, %%ecx\n\t"
                "movl %%ecx, %[seed]\n\t"
                "mull %%ecx\n\t"
                "shll $15, %%edx\n\t"
                "subl %%edx, %%ebx\n\t"
                "jge 2f\n\t"
                "xorl %%ebx, %%ebx\n\t"
                "2:\n\t"
                "andl $0x1ff0000, %%ebx\n\t"
                "rcll $8, %%ebx\n\t"
                "sbbl $0, %%ebx\n\t"
                "movl %%ebx, %%eax\n\t"
                "shrl $24, %%eax\n\t"
                "mulb %%al\n\t"
                "mull %%eax\n\t"
                "shrl $16, %%eax\n\t"
                "mull %%eax\n\t"
                "shrl $16, %%eax\n\t"
                "andl $0xff00, %%eax\n\t"
                "movl %%eax, %%ecx\n\t"
                "shrl $8, %%ecx\n\t"
                "orl %%eax, %%ecx\n\t"
                "movl %%ecx, %%eax\n\t"
                "shll $16, %%ecx\n\t"
                "orl %%eax, %%ecx\n\t"
                "movl %%ecx, (%[pixel])"
                : [seed] "+r" (seed)
                : [x] "r" (x), [y] "r" (y), [cntrx] "r" (cntrx), [cntry] "r" (cntry), 
                  [scale] "r" (scale), [noise] "r" (NOISE), [pixel] "r" (pCurrentPixel)
                : "eax", "ebx", "ecx", "edx", "cc", "memory"
            );
#else
            int dx = x - cntrx;
            int dy = y - cntry;

            const int32_t dxScaled = static_cast<int32_t>(static_cast<uint32_t>(dx) << scale);
            const int32_t dyScaled = static_cast<int32_t>(static_cast<uint32_t>(dy) << scale);

            const uint32_t distSq =
                static_cast<uint32_t>(static_cast<int64_t>(dxScaled) * dxScaled) +
                static_cast<uint32_t>(static_cast<int64_t>(dyScaled) * dyScaled);

            uint32_t ebxVal = X86E::ClampedUnsignedSub(16777216u, distSq);

            const uint32_t noiseHigh = X86E::MulHigh32(ebxVal, NOISE);
            const bool carryFromNoiseMul = (noiseHigh != 0);

            const uint32_t rotatedSeed = X86E::RotateLeftThroughCarry32(seed, 13, carryFromNoiseMul).value;
            seed = rotatedSeed - (seed - 11);

            const uint32_t seedMulHighShifted = X86E::MulHigh32(noiseHigh, seed) << 15;
            ebxVal = X86E::ClampedSignedSub(ebxVal, seedMulHighShifted);

            ebxVal &= 0x1ff0000u;
            const X86E::RclResult rotated = X86E::RotateLeftThroughCarry32(ebxVal, 8, false);
            ebxVal = rotated.value - (rotated.carryOut ? 1u : 0u);

            uint32_t eax = ebxVal >> 24;
            eax = eax * eax;
            eax = static_cast<uint32_t>((static_cast<uint64_t>(eax) * eax) >> 16);
            eax = static_cast<uint32_t>((static_cast<uint64_t>(eax) * eax) >> 16);
            eax &= 0xff00u;

            const uint8_t v = static_cast<uint8_t>(eax >> 8);
            *pCurrentPixel = (uint32_t(v) << 24) | (uint32_t(v) << 16) | (uint32_t(v) << 8) | v;
#endif
        }
    }

    glowTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(glowTexture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(glowTexture, TEXTURE_WRAP_CLAMP);
}

void Blob::Load()
{
    g_Blob = this;

    constexpr int BLOBLET_DIM = 8;
    constexpr int BLOB_DIM = 32;

    std::vector<uint16_t> blobIndices;
    BuildCubeSphere(BLOB_DIM, unitSphereNormals, blobIndices);
    UploadStaticMesh(unitSphereNormals, blobIndices, blobVAO, blobStaticVBO, blobEBO);
    blobIndexCount = (unsigned int)blobIndices.size();
    numVertsPerFace = (int)(unitSphereNormals.size() / 6);

    changingVertices.resize(unitSphereNormals.size());

    rlEnableVertexArray(blobVAO);
    blobDynamicVBO = rlLoadVertexBuffer(nullptr, (int)(changingVertices.size() * sizeof(Vector4)), true);
    rlSetVertexAttribute(1, 4, RL_FLOAT, false, sizeof(Vector4), 0);
    rlEnableVertexAttribute(1);
    rlDisableVertexArray();

    std::vector<Vector3> blobletPositions;
    std::vector<uint16_t> blobletIndices;
    BuildCubeSphere(BLOBLET_DIM, blobletPositions, blobletIndices);
    UploadStaticMesh(blobletPositions, blobletIndices, blobletVAO, blobletStaticVBO, blobletEBO);
    blobletIndexCount = (unsigned int)blobletIndices.size();

    Restart();

    blobShader = LoadShaderFromMemory(
#include "shaders/blob.vert.inl"
    ,
#include "shaders/blob.frag.inl"
    );

    blobLoc_mvp = GetShaderLocation(blobShader, "mvp");
    blobLoc_eyePos = GetShaderLocation(blobShader, "eyePos");
    blobLoc_scaling = GetShaderLocation(blobShader, "scaling");
    blobLoc_ooScaling = GetShaderLocation(blobShader, "ooScaling");
    blobLoc_center = GetShaderLocation(blobShader, "center");
    blobLoc_baseColor = GetShaderLocation(blobShader, "baseColor");
    blobLoc_ambientColor = GetShaderLocation(blobShader, "ambientColor");

    blobletShader = LoadShaderFromMemory(
#include "shaders/bloblet.vert.inl"
    ,
#include "shaders/bloblet.frag.inl"
    );

    bloLoc_mvp = GetShaderLocation(blobletShader, "mvp");
    bloLoc_eyePos = GetShaderLocation(blobletShader, "eyePos");
    bloLoc_center = GetShaderLocation(blobletShader, "center");
    bloLoc_scaleDir = GetShaderLocation(blobletShader, "scaleDir");
    bloLoc_scalePerp = GetShaderLocation(blobletShader, "scalePerp");
    bloLoc_scaleDirPMP = GetShaderLocation(blobletShader, "scaleDirPMP");
    bloLoc_baseColor = GetShaderLocation(blobletShader, "baseColor");
    bloLoc_ambientColor = GetShaderLocation(blobletShader, "ambientColor");
    bloLoc_alphaScale = GetShaderLocation(blobletShader, "alphaScale");

    BuildGlowTexture();
}

void Blob::Unload()
{
    if (blobVAO)
    {
        rlUnloadVertexArray(blobVAO);
        rlUnloadVertexBuffer(blobStaticVBO);
        rlUnloadVertexBuffer(blobDynamicVBO);
        rlUnloadVertexBuffer(blobEBO);
        blobVAO = 0;
    }
    if (blobletVAO)
    {
        rlUnloadVertexArray(blobletVAO);
        rlUnloadVertexBuffer(blobletStaticVBO);
        rlUnloadVertexBuffer(blobletEBO);
        blobletVAO = 0;
    }
    if (blobShader.id) UnloadShader(blobShader);
    if (blobletShader.id) UnloadShader(blobletShader);
    if (glowTexture.id) UnloadTexture(glowTexture);
    blobShader = Shader{};
    blobletShader = Shader{};
    glowTexture = Texture2D{};
}

void Blob::ZeroChangingVertices()
{
    for (size_t i = 0; i < unitSphereNormals.size(); i++)
    {
        const Vector3& n = unitSphereNormals[i];
        changingVertices[i] = { n.x, n.y, n.z, 1.0f };
    }
}

void Blob::PrepareChangingVertices()
{
    const BlobBump* boi[MAX_BLOB_BUMPS];

    for (int face = 0; face < 6; face++)
    {
        int numBoi = 0;
        for (int i = 0; i < numBlobBumps; i++)
            if (blobBumps[i].facesOfInterest & (1 << face))
                boi[numBoi++] = &blobBumps[i];

        int base = face * numVertsPerFace;
        for (int v = 0; v < numVertsPerFace; v++)
        {
            const Vector3& usNormal = unitSphereNormals[base + v];
            Vector4 accum{ usNormal.x, usNormal.y, usNormal.z, 0.0f };

            for (int j = numBoi - 1; j >= 0; j--)
            {
                const BlobBump* b = boi[j];
                Vector3 delta = Vector3Subtract(usNormal, b->position);
                float dist2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                if (dist2 < b->radius2)
                {
                    float dist2Mo = dist2 * b->ooRadius2 - 1.0f;

                    float displacement = radius * b->magnitude * dist2Mo * dist2Mo;
                    float perturbAmount = -4.0f * b->magnitude * b->ooRadius2 * dist2Mo;

                    Vector3 lnorm = AddScaled(usNormal, delta, perturbAmount);
                    QuickNormalize(&lnorm);

                    accum.x += lnorm.x;
                    accum.y += lnorm.y;
                    accum.z += lnorm.z;
                    accum.w += displacement;
                }
            }

            changingVertices[base + v] = accum;
        }
    }
}

void Blob::Restart()
{
    numBloblets = 0;
    numBlobBumps = 0;

    while (numBlobBumps < MAX_BLOB_BUMPS)
    {
        if (blobBumps[numBlobBumps++].Create(-0.3f, (numBloblets < MAX_BLOBLETS) ? &bloblets[numBloblets] : nullptr))
            numBloblets++;
    }

    ZeroChangingVertices();
}

void Blob::AdvanceTime(float elapsedTime, float dt)
{
    if (elapsedTime < BLOB_STATIC_END_TIME)
    {
        return;
    }

    for (int i = 0; i < numBloblets; i++)
        bloblets[i].Update(elapsedTime, dt);

    for (int i = 0; i < numBlobBumps; i++)
    {
        if (blobBumps[i].Update(elapsedTime, dt, (numBloblets < MAX_BLOBLETS) ? &bloblets[numBloblets] : nullptr))
            numBloblets++;
    }

    PrepareChangingVertices();
}

void Blob::GetLightForPosition(Vector3 queryPosition, Vector3* outLightPos, float* outIntensity) const
{
    float totalWeights = 0.0f;
    Vector3 avPos{ 0, 0, 0 };
    float avIntensity = 0.0f;

    {
        float dist2 = Vector3DistanceSqr(queryPosition, position);
        dist2 = std::max(dist2, 1e-6f);
        float weight = 1.0f / dist2;
        avIntensity += 4.0f * lightIntensity * weight;
        avPos = AddScaled(avPos, position, weight);
        totalWeights += weight;
    }

    for (int i = 0; i < numBloblets; i++)
    {
        float dist2 = Vector3DistanceSqr(queryPosition, bloblets[i].position);
        dist2 = std::max(dist2, 1e-6f);
        float weight = 1.0f / dist2;
        avIntensity += lightIntensity * weight;
        avPos = AddScaled(avPos, bloblets[i].position, weight);
        totalWeights += weight;
    }

    float ooTotalWeights = 1.0f / totalWeights;
    *outLightPos = Vector3Scale(avPos, ooTotalWeights);
    *outIntensity = ooTotalWeights * avIntensity;
}

void Blob::Render(const Camera3D& camera, float pulseIntensity, float blobIntensity,
                   float baseBlobIntensity, float elapsedTime)
{
    lightIntensity = blobIntensity + pulseIntensity;
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD,
                                     (float)GetScreenWidth() / (float)GetScreenHeight(),
                                     0.4f, 800.0f);
    Matrix viewProj = MatrixMultiply(view, proj);

    float curRad = radius * (1.0f + 1.3f * sqrtf(std::max(0.0f, pulseIntensity)));

    {
        float alpha = std::min(blobIntensity, 1.0f) * 255.0f;
        Color tint = { 0xa0, 0xff, 0x40, (unsigned char)alpha };

        float haloRadius = curRad * 5.2f;
        float haloSize = haloRadius * 2.0f;

        BeginBlendMode(BLEND_ADDITIVE);
        rlDisableDepthMask();
        DrawBillboardPro(camera, glowTexture,
                          Rectangle{ 0, 0, (float)glowTexture.width, (float)glowTexture.height },
                          position, camera.up,
                          Vector2{ haloSize, haloSize }, Vector2{ haloSize * 0.5f, haloSize * 0.5f },
                          0.0f, tint);
        rlEnableDepthMask();
        EndBlendMode();
    }

    Vector3 scaledRadius = { curRad * scale.x, curRad * scale.y, curRad * scale.z };
    Vector3 ooScaledRadius = { 1.0f / scaledRadius.x, 1.0f / scaledRadius.y, 1.0f / scaledRadius.z };

    float colorIntensity = BLOB_BASE_INTENSITY + 4.0f * (1.2f * baseBlobIntensity + 0.8f * pulseIntensity);
    colorIntensity *= std::min(1.0f, elapsedTime * 4.0f);
    Vector4 litColor = Vector4Scale(color, colorIntensity);
    Vector4 ambientColor = Vector4Scale(color, 0.0f);

    BeginBlendMode(BLEND_ALPHA);
    BeginShaderMode(blobShader);
    SetShaderValueMatrix(blobShader, blobLoc_mvp, viewProj);
    SetShaderValue(blobShader, blobLoc_eyePos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobShader, blobLoc_scaling, &scaledRadius, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobShader, blobLoc_ooScaling, &ooScaledRadius, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobShader, blobLoc_center, &position, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobShader, blobLoc_baseColor, &litColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(blobShader, blobLoc_ambientColor, &ambientColor, SHADER_UNIFORM_VEC4);

    rlUpdateVertexBuffer(blobDynamicVBO, changingVertices.data(),
                          (int)(changingVertices.size() * sizeof(Vector4)), 0);

    rlEnableVertexArray(blobVAO);
    rlDrawVertexArrayElements(0, (int)blobIndexCount, nullptr);
    rlDisableVertexArray();
    EndShaderMode();

    Vector4 blobletColor = Vector4Scale(color, 0.3f * blobIntensity);
    Vector4 blobletAmbient = Vector4Scale(color, 0.2f);

    BeginShaderMode(blobletShader);
    SetShaderValueMatrix(blobletShader, bloLoc_mvp, viewProj);
    SetShaderValue(blobletShader, bloLoc_eyePos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(blobletShader, bloLoc_baseColor, &blobletColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(blobletShader, bloLoc_ambientColor, &blobletAmbient, SHADER_UNIFORM_VEC4);
    float alphaScale = 2.0f;
    SetShaderValue(blobletShader, bloLoc_alphaScale, &alphaScale, SHADER_UNIFORM_FLOAT);

    rlEnableVertexArray(blobletVAO);
    for (int i = 0; i < numBloblets; i++)
    {
        const Bloblet& bl = bloblets[i];

        float perp = bl.radius / sqrtf(bl.wobble);
        float parallelMinusPerp = bl.radius * bl.wobble - perp;
        Vector3 scaleDirPMP = Vector3Scale(bl.direction, parallelMinusPerp);

        SetShaderValue(blobletShader, bloLoc_center, &bl.position, SHADER_UNIFORM_VEC3);
        SetShaderValue(blobletShader, bloLoc_scaleDir, &bl.direction, SHADER_UNIFORM_VEC3);
        SetShaderValue(blobletShader, bloLoc_scalePerp, &perp, SHADER_UNIFORM_FLOAT);
        SetShaderValue(blobletShader, bloLoc_scaleDirPMP, &scaleDirPMP, SHADER_UNIFORM_VEC3);

        rlDrawVertexArrayElements(0, (int)blobletIndexCount, nullptr);
    }
    rlDisableVertexArray();
    EndShaderMode();
    EndBlendMode();
}
