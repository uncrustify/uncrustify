/* Test rules chapter 6.8 */

/* no indentation */
#if COMINL_coMINIMUM_DELAY_TIME == COMINL_coENABLE
#include "MEMSRV.h"
#endif

/* already well indented  */
#ifndef COMINL_coAPPLI_TX_CONFIRMATION
   #error "Define COMINL_coAPPLI_TX_CONFIRMATION is undefined"
#endif

/* bad indentation */
#ifndef COMINL_coENABLE
 #error "Define COMINL_coENABLE is undefined"
#endif


#if COMINL_coSTART_STOP_PERIODIC == COMINL_coENABLE
void COMINL_vidInit(void)
{
   uint8           u8LocalMsgIdx;

   if (COMINL_kastrVarReceiveMsg[u8LocalMsgIdx].udtDeadlineMonTimer < COMINL_udtNB_MESSAGES_RX)
   {
      u8LocalMsgIdx = E_OK;
   }
   else
   {
      u8LocalMsgIdx = E_NOT_OK;
   }

/* nested #if...already well indented */
   #if COMINL_coRX_MESSAGE_VAR == COMINL_coENABLE
   /*!Trace to: VEES_R_11_04044_004.01*/
      for(u8LocalMsgIdx = 0; u8LocalMsgIdx < COMINL_udtNB_MESSAGES_RX; u8LocalMsgIdx++)
      {
         #if COMINL_coRX_DEADLINE_MONITORING == COMINL_coENABLE
            COMINL_kastrVarReceiveMsg[u8LocalMsgIdx].udtDeadlineMonTimer = 0;
         #else /* COMINL_coRX_DEADLINE_MONITORING == COMINL_coENABLE */
            COMINL_kastrVarReceiveMsg[u8LocalMsgIdx].udtINMDeadlineMonTimer = 0;
         #endif /* COMINL_coRX_DEADLINE_MONITORING == COMINL_coENABLE */
      }
   #endif /* COMINL_coRX_MESSAGE_VAR == COMINL_coENABLE */



/* nested #if... no indentation  */
#if COMINL_coTX_MESSAGE_VAR == COMINL_coENABLE
   /*!Trace to: VEES_R_11_04044_004.01*/
   for(u8LocalMsgIdx = 0; u8LocalMsgIdx < COMINL_udtNB_MESSAGES_TX; u8LocalMsgIdx++)
   {
#if COMINL_coTX_DEADLINE_MONITORING == COMINL_coENABLE
      COMINL_kastrVarTransmitMsg[u8LocalMsgIdx].udtDeadlineMonTimer = 0;
      COMINL_kastrVarTransmitMsg[u8LocalMsgIdx].bDeadlineMonEnable  = FALSE;
#endif /* COMINL_coTX_DEADLINE_MONITORING == COMINL_coENABLE */
#if COMINL_coTX_INM_DEADLINE_MONITORING == COMINL_coENABLE
      COMINL_kastrVarTransmitMsg[u8LocalMsgIdx].udtINMDeadlineMonTimer = 0;
#else /* COMINL_coTX_INM_DEADLINE_MONITORING == COMINL_coENABLE */
      COMINL_kastrVarTransmitMsg[u8LocalMsgIdx].udtMDTTimer   = 0;
      COMINL_kastrVarTransmitMsg[u8LocalMsgIdx].bMDTMsgToSend = FALSE;
#endif /*COMINL_coMINIMUM_DELAY_TIME == COMINL_coENABLE*/
#if COMINL_coMIXED_MODE != COMINL_coDISABLE
      COMINL_kastrVarTransmitMsg[u8LocalMsgIdx].udtPeriodicTimer = 0;
#endif
   }
#endif /* COMINL_coTX_MESSAGE_VAR == COMINL_coENABLE */
}
#endif

/*------------------------------- end of file --------------------------------*/
