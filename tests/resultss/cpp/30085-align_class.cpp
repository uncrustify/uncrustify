//---------------------------------------------------------------------------
//    Statics                                                               |
//---------------------------------------------------------------------------
void      CTdrFile::SetDistanceMode( TDistMode dm )   {   CTdrFile::ms_DistMode = dm;         }
TDistMode CTdrFile::GetDistanceMode( void )           {   return CTdrFile::ms_DistMode;      }
String    CTdrFile::GetDistanceModeUnits( void )      {   return ( CTdrFile::GetDistanceMode() == dmKM ) ? "km" : "Miles";   }
void      CTdrFile::SetBSTCompensation( bool bUseBST ){   ms_bCompBST = bUseBST;              }
void      CTdrFile::SetFactoryMode( bool bFactory )   {   ms_bFactory = bFactory;             }
bool      CTdrFile::GetFactoryMode( void )            {   return ms_bFactory;                 }

unsigned int      CAgentCharacter::iReferenceCount = 0;
IAgentEx*         CAgentCharacter::pAgentEx        = NULL;
CAgentNotifySink* CAgentCharacter::pSink           = NULL;

