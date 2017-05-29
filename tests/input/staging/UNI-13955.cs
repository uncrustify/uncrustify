// The closing parenthesis is being indented twice.
bool success = GenerateSecondaryUVSet(
        &mesh.vertices[0].x, mesh.vertices.size(),
        &triUV[0].x, &triList[0], triSrcPoly.size() ? &triSrcPoly[0] : 0, triCount,
        &outUV[0].x, param, errorBuffer, bufferSize
    );