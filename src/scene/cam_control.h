#pragma once

#include "raylib.h"
#include "raymath.h"

#include "../util/qrand.h"

#include <vector>

struct CamControlNodeData
{
    unsigned char ucTime;
    signed char scTension, scBias;
    Vector3 ptPosition;
    Vector3 vecLookAt;
};

struct CamControlNode
{
    float fTime = 0.f;
    Vector3 ptPosition{};
    Vector3 vecVelocity{};
    Vector3 vecLookAt{};
    Vector3 vecLookAtW{};
    float tension = 0.f, bias = 0.f;
};

class CameraController
{
public:
    void init();

    void pickPath(int path = -1);

    void getPosition(float t, Vector3 *pPos, Vector3 *pLook, bool *pbRenderGeom, bool *pbRenderSlash) const;

    const Matrix &getSlashTransform() const { return xfSlash; }

private:
    static constexpr int NUM_FINISH_NODES = 8;

    static std::vector<CamControlNodeData> svCameraListData();
    std::vector<CamControlNode> svCameraList;
    CamControlNode finishNodes[NUM_FINISH_NODES];
    Matrix xfSlash = MatrixIdentity();
    Vector3 ptSlashCenter{};
    Vector3 ptFinalLookAt{};
    int numNodes = 0;
    int numPaths = 0;
    int curPathNum = -1;
    int curStartNode = 0;
    int curNumNodes = 0;
    int curVariableNodes = 0;
    float fCameraLookatInterpStart = 0.f, fOOCameraLookatInterpDelta = 0.f;
    QRand rng;

    CamControlNode *getNode(int i) { return (i < curVariableNodes) ? &svCameraList[i + curStartNode] : &finishNodes[i - curVariableNodes]; }
    const CamControlNode *getNode(int i) const { return (i < curVariableNodes) ? &svCameraList[i + curStartNode] : &finishNodes[i - curVariableNodes]; }
};
