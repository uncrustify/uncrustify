floatNx3 randomRotationMatrices[3];
if (rotationRandomnessX > epsilon() || rotationRandomnessY > epsilon())
{
// Parameters are being double indented.
    floatNx3 rotationEuler = floatNx3(
        (GenerateRandom(randomSeed + intN(kParticleSystemExternalForcesRotationRandomnessXId)) * 2 - 1) * rotationRandomnessX,
        (GenerateRandom(randomSeed + intN(kParticleSystemExternalForcesRotationRandomnessYId)) * 2 - 1) * rotationRandomnessY,
        floatN(ZERO));
    eulerToMatrix(rotationEuler, randomRotationMatrices);

    toForce = mul(randomRotationMatrices, toForce);
}
