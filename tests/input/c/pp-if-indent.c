/* this is a comment */

/* should be in a H file but put in this file to avoid multiplying the number of test files */
#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "STD_TYPES.h"
#ifdef COMINL_coENABLE_1
#ifdef COMINL_coENABLE_2
#include "def.h"
#endif
#endif

#if (COMINL_coMINIMUM_DELAY_TIME_1 == COMINL_coENABLE)
#if (COMINL_coMINIMUM_DELAY_TIME_2 == COMINL_coENABLE)
#include "def1.h"
#define COMINL_coMINIMUM_DELAY_TIME_1
#include "def2.h"
#endif
#endif

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


#ifdef COMINL_coENABLE_3
typedef enum
{
  BUFREQ_OK            =0,
     BUFREQ_E_NOT_OK      =     1,
   BUFREQ_E_BUSY=2,
BUFREQ_E_OVFL    =3
}
BufReq_ReturnType;
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


void myfunction(void)
{
int i;
#ifdef COMINL_coTX_MESSAGE_VAR
#ifndef COMINL_coMIXED_MODE
#pragma MyPragma
int j;
#endif
#endif
int k;
}


#endif /* COMSTACK_TYPES_H */

/*------------------------------- end of file --------------------------------*/
