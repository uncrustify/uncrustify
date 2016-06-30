char *DLLDetails[2][4] =
{
    {
        // 32 bit
        {"sce/vagconv2_32/vagconv2.dll"},
        {"_sceVagConvertInit@4"}, {"_sceVagConvert@12"}, {"_sceVagConvertFin@4"}
    },
    {
        // 64 bit
        {"sce/vagconv2_64/vagconv2.dll"},
        {"sceVagConvertInit"}, {"sceVagConvert"}, {"sceVagConvertFin"}
    }
};
