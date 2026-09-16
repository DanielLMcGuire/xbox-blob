#include "cam_control.h"
#include "../defines.h"
#include "../blob/blob_math.h"
#include <algorithm>
#include <cmath>

std::vector<CamControlNodeData> CameraController::svCameraListData()
{
    return {
        {  0,  +00, +00,  {+11.4f, -32.1f, +33.0f}, {+0.0f, +0.0f, +0.0f} },
        { 20,  +00, +00,  {+13.4f, -37.7f, +25.6f}, {+0.0f, +0.0f, +0.0f} },
        { 40,  +00, +00,  {+15.6f, -43.9f,  +8.8f}, {+0.0f, +0.0f, +0.0f} },
        { 60,  +00, +00,  {+16.0f, -45.0f, -12.8f}, {+0.0f, +0.0f, +0.0f} },
        { 90,  +00, +00,  {+18.2f, -51.2f, -29.6f}, {+0.0f, +0.0f, +0.0f} },

        {  0,  +00, +00,  {-55.4f, +19.7f, -31.5f}, {+0.0f, +0.0f, +0.0f} },
        { 30,  +00, +00,  {-55.4f, +19.7f, -31.5f}, {+0.0f, +0.0f, +0.0f} },
        { 45,  +00, +00,  {-39.5f,  -0.6f,  -7.8f}, {+0.0f, +0.0f, +0.0f} },
        { 60,  +00, +00,  { -4.3f, -35.5f, +16.6f}, {+0.0f, +0.0f, +0.0f} },
        { 70,  +00, +00,  {+31.1f, -32.6f, +17.6f}, {+0.0f, +0.0f, +0.0f} },
        { 80,  +00, +00,  {+57.7f,  -7.2f,  +3.3f}, {+0.0f, +0.0f, +0.0f} },
        { 95,  +00, +00,  {+70.9f,  +1.8f,  +3.1f}, {+0.0f, +0.0f, +0.0f} },
        
        {  0,  +00, +00,  {+34.7f, +25.9f, +12.3f}, {+0.0f, +0.0f, +0.0f} },
        { 25,  +00, +00,  {+42.3f,  +9.3f, +12.3f}, {+0.0f, +0.0f, +0.0f} },
        { 50,  +00, +00,  {+42.4f,  -8.8f, +12.3f}, {+0.0f, +0.0f, +0.0f} },
        { 75,  +00, +00,  {+34.4f, -26.3f, +12.3f}, {+0.0f, +0.0f, +0.0f} },
        { 95,  +00, +00,  {+30.7f, -48.1f, +14.3f}, {+0.0f, +0.0f, +0.0f} },

        {  0,  +00, +00,  {-50.1f,  -0.3f, -51.5f}, {+0.0f, +0.0f, +0.0f} },
        { 25,  +00, +00,  {-50.1f,  -0.3f, -51.5f}, {+0.0f, +0.0f, +0.0f} },
        { 75,  +00, +00,  {-50.1f,  -0.3f, -51.5f}, {+0.0f, +0.0f, +0.0f} },
        { 95,  +00, +00,  {-62.2f,  -0.4f, -12.0f}, {+0.0f, +0.0f, +0.0f} },
    };
}

void CameraController::init()
{
    std::vector<CamControlNodeData> data = svCameraListData();
    numNodes = (int)data.size();
    svCameraList.resize(numNodes);
    numPaths = 0;
    curPathNum = -1;
    for (int i = 0; i < numNodes; i++)
    {
        if (data[i].ucTime == 0) numPaths++;
        svCameraList[i].fTime = FINISH_START_TIME * ((float)data[i].ucTime) * 0.01f;
        svCameraList[i].ptPosition = data[i].ptPosition;
        svCameraList[i].vecLookAt = data[i].vecLookAt;
        svCameraList[i].tension = ((float)data[i].scTension) * 0.01f;
        svCameraList[i].bias = ((float)data[i].scBias) * 0.01f;
    }
    pickPath(-1);
}

void CameraController::pickPath(int path)
{
    if (path < 0)
        path = (int)((uint32_t)rng.Rand() & 0x7FFFFFFF);
    if (path >= numPaths) path = path % numPaths;
    curPathNum = path;

    int i;
    for (i = 0; i < numNodes; i++)
    {
        if (svCameraList[i].fTime == 0.0f)
        {
            if (!path) break;
            path--;
        }
    }
    curStartNode = i;
    for (i = curStartNode + 1; i < numNodes; i++)
        if (svCameraList[i].fTime == 0.0f) break;
    curVariableNodes = i - curStartNode;
    curNumNodes = curVariableNodes + NUM_FINISH_NODES;

    for (int j = 0; j < NUM_FINISH_NODES; j++)
    {
        finishNodes[j].fTime = FINISH_START_TIME + FINISH_TRANSITION_TIME * ((float)j) / ((float)(NUM_FINISH_NODES - 1));
        finishNodes[j].tension = 0.0f;
        finishNodes[j].bias = 0.0f;
    }
    fCameraLookatInterpStart = finishNodes[2].fTime;
    fOOCameraLookatInterpDelta = 1.0f / (finishNodes[5].fTime - fCameraLookatInterpStart);

    const CamControlNode *plast = &svCameraList[curStartNode + curVariableNodes - 1];
    CamControlNode *pthis = &finishNodes[0];

    const float slash_start_rad = -95.0f;
    const float slash_end_rad = 132.14f;
    const float cfYPositions[NUM_FINISH_NODES] = { +95.0f, +30.548f, -70.819f, -150.298f, -220.64f, -243.021f, -261.441f, -287.773f };
    const float cfZPositions[NUM_FINISH_NODES] = { 0.0f, 0.322f, 1.821f, 2.323f, -11.926f, -39.973f, -60.774f, -90.795f };
    const float cfMinStartDist = 100.0f;

    pthis->ptPosition = plast->ptPosition;
    Vector3 vel;
    if (curVariableNodes >= 2)
    {
        vel = Vector3Subtract(getNode(curVariableNodes - 1)->ptPosition, getNode(curVariableNodes - 2)->ptPosition);
        vel = Vector3Scale(vel, 1.0f / (getNode(curVariableNodes - 1)->fTime - getNode(curVariableNodes - 2)->fTime));
    }
    else
    {
        vel = {0.0f, 0.0f, 0.0f};
    }
    pthis->ptPosition = AddScaled(pthis->ptPosition, vel, (pthis->fTime - plast->fTime) * 0.7f);
    float vel_adj_len = Vector3Length(pthis->ptPosition);
    pthis->ptPosition = Vector3Scale(pthis->ptPosition, 1.0f / vel_adj_len);
    Vector3 slash_dir = pthis->ptPosition;
    float slash_y_offset = std::max(cfMinStartDist - slash_start_rad, vel_adj_len * 1.2f - slash_start_rad);
    pthis->ptPosition = Vector3Scale(pthis->ptPosition, slash_y_offset + slash_start_rad);
    pthis->vecLookAt = {0.0f, 0.0f, 0.0f};

    Vector3 up = {0.0f, 0.0f, 1.0f};
    Vector3 y_dir = Vector3Scale(slash_dir, -1.0f);
    Vector3 x_dir = Vector3Normalize(Vector3CrossProduct(y_dir, up));
    Vector3 z_dir = Vector3CrossProduct(x_dir, y_dir);

    xfSlash = MatrixIdentity();
    xfSlash.m0 = x_dir.x; xfSlash.m4 = y_dir.x; xfSlash.m8  = z_dir.x;
    xfSlash.m1 = x_dir.y; xfSlash.m5 = y_dir.y; xfSlash.m9  = z_dir.y;
    xfSlash.m2 = x_dir.z; xfSlash.m6 = y_dir.z; xfSlash.m10 = z_dir.z;

    ptSlashCenter = Vector3Scale(y_dir, -slash_end_rad - slash_y_offset);
    float y_basis = slash_end_rad;
    for (int j2 = 1; j2 < NUM_FINISH_NODES; j2++)
    {
        plast = pthis++;
        Vector3 pt_in_slash = {0.0f, cfYPositions[j2] + y_basis, cfZPositions[j2]};
        pthis->ptPosition = Vector3Add(Vector3Transform(pt_in_slash, xfSlash), ptSlashCenter);
        pthis->vecLookAt = {0.0f, 0.0f, 0.0f};
    }

    Vector3 t = {0.0f, slash_end_rad, 25.0f};
    ptFinalLookAt = Vector3Add(Vector3Transform(t, xfSlash), ptSlashCenter);

    for (int j = 0; j < curNumNodes; j++)
    {
        CamControlNode *node = getNode(j);
        node->vecVelocity = {0.0f, 0.0f, 0.0f};
        node->vecLookAtW = {0.0f, 0.0f, 0.0f};
        if (j)
        {
            Vector3 delta = Vector3Subtract(node->ptPosition, getNode(j - 1)->ptPosition);
            node->vecVelocity = AddScaled(node->vecVelocity, delta, (1.0f - node->tension) * (1.0f + node->bias) * 0.5f);
            delta = Vector3Subtract(node->vecLookAt, getNode(j - 1)->vecLookAt);
            node->vecLookAtW = AddScaled(node->vecLookAtW, delta, (1.0f - node->tension) * (1.0f + node->bias) * 0.5f);
        }
        if (j < curNumNodes - 1)
        {
            Vector3 delta = Vector3Subtract(getNode(j + 1)->ptPosition, node->ptPosition);
            node->vecVelocity = AddScaled(node->vecVelocity, delta, (1.0f - node->tension) * (1.0f - node->bias) * 0.5f);
            delta = Vector3Subtract(getNode(j + 1)->vecLookAt, node->vecLookAt);
            node->vecLookAtW = AddScaled(node->vecLookAtW, delta, (1.0f - node->tension) * (1.0f - node->bias) * 0.5f);
        }
    }
}

void CameraController::getPosition(float t, Vector3 *pPos, Vector3 *pLook, bool *pbRenderGeom, bool *pbRenderSlash) const
{
    if (t > FINISH_STOP_TIME)
    {
        *pPos = finishNodes[NUM_FINISH_NODES - 1].ptPosition;
        *pLook = ptFinalLookAt;
        *pbRenderSlash = true;
        *pbRenderGeom = false;
        return;
    }
    int i;
    for (i = 1; i < curNumNodes; i++)
        if (getNode(i)->fTime > t) break;

    if (i == curNumNodes)
    {
        *pPos = getNode(curNumNodes - 1)->ptPosition;
        *pLook = ptFinalLookAt;
        *pbRenderSlash = true;
        *pbRenderGeom = false;
        return;
    }
    if (i == 0)
    {
        *pPos = {0.0f, -90.0f, 0.0f};
        *pLook = {0.0f, 0.0f, 0.0f};
        *pbRenderSlash = true;
        *pbRenderGeom = false;
        return;
    }

    const CamControlNode *pprev = getNode(i - 1);
    const CamControlNode *pnext = getNode(i);
    float dtc = std::max(0.001f, pnext->fTime - pprev->fTime);
    float dtp = std::max(0.001f, (i >= 2) ? pprev->fTime - getNode(i - 2)->fTime : dtc);
    float dtn = std::max(0.001f, (i < curNumNodes - 1) ? getNode(i + 1)->fTime - pnext->fTime : dtc);
    (void)dtn;
    float uts = std::min(1.0f, std::max(0.0f, (t - pprev->fTime) / dtc));
    float utss = uts * uts;
    float utsss = utss * uts;
    float frac = -2.0f * utsss + 3.0f * utss;
    float s = (t - pprev->fTime) / ((1.0f - frac) * dtp + frac * dtc);
    float ss = s * s;
    float sss = ss * s;
    float cA = 2.0f * sss - 3.0f * ss + 1.0f;
    float cB = sss - 2.0f * ss + s;
    float cC = sss - ss;
    float cD = -2.0f * sss + 3.0f * ss;

    pPos->x = cA * pprev->ptPosition.x + cB * pprev->vecVelocity.x + cC * pnext->vecVelocity.x + cD * pnext->ptPosition.x;
    pPos->y = cA * pprev->ptPosition.y + cB * pprev->vecVelocity.y + cC * pnext->vecVelocity.y + cD * pnext->ptPosition.y;
    pPos->z = cA * pprev->ptPosition.z + cB * pprev->vecVelocity.z + cC * pnext->vecVelocity.z + cD * pnext->ptPosition.z;
    pLook->x = cA * pprev->vecLookAt.x + cB * pprev->vecLookAtW.x + cC * pnext->vecLookAtW.x + cD * pnext->vecLookAt.x;
    pLook->y = cA * pprev->vecLookAt.y + cB * pprev->vecLookAtW.y + cC * pnext->vecLookAtW.y + cD * pnext->vecLookAt.y;
    pLook->z = cA * pprev->vecLookAt.z + cB * pprev->vecLookAtW.z + cC * pnext->vecLookAtW.z + cD * pnext->vecLookAt.z;

    float sl = std::max(0.0f, std::min(1.0f, (t - fCameraLookatInterpStart) * fOOCameraLookatInterpDelta));
    float s_, c_;
    s_ = sinf(sl * PI);
    c_ = cosf(sl * PI);
    (void)s_;
    float interp = 0.5f * (1.0f - c_);
    *pLook = Vector3Scale(*pLook, 1.0f - interp);
    *pLook = AddScaled(*pLook, ptFinalLookAt, interp);

    *pbRenderSlash = (i > curVariableNodes - 1);
    *pbRenderGeom = (i < curNumNodes - 1);
}
