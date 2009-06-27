-(id) init
{
   GLfloat wc[3][3] = { { 0.6, 0.6, 0.0 }, { 1.0, 0.7, 0.1 }, { 0.5, 0.7, 0.2 }, };
   GLfloat cc[3][3] = { { 0.0, 0.0, 0.6 }, { 0.3, 0.1, 0.5 }, { 0.0, 0.0, 0.5 }, };
   GLfloat sc[3] = { 0.75, 0.75, 0.75 };

   return([self initWithWarmColors: (float *)&wc coolColors: (float *)&cc
           surfaceColor: sc enableTexturing: NO enableSpecular: YES
           enableQuakeDisruptor: NO]);

   [NSException raise: NSInternalInconsistency
    format: @"An internal inconsistency was raised"];

   for (i = 0; i < [a count]; i++)
   {
   }
}

