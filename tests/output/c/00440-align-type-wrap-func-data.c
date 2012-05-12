typedef VAR(uint8 **, COMINL_VAR_8BIT) COMINL_u8Truc_t;
typedef uint8                          Toto_t;

typedef FUNC(Std_ReturnType, SPI_CODE)(*MyFuncPtr1_t)
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);

typedef FUNC(Std_ReturnType, SPI_CODE)(*MyFuncPtr2_t)
(
   Spi_ChannelType,
   int,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA),
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA),
   Spi_NumberOfDataType
);

typedef int                                             prm2_t;
typedef P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr_t;
typedef P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr_t;
typedef Spi_NumberOfDataType                            Length_t;

typedef P2FUNC(void, NVM_APPL_CODE, NvM_CbkFncPtr1Type)
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);

typedef P2FUNC(P2VAR(int, AUTOMATIC, SPI_APPL_DATA), NVM_APPL_CODE, NvM_CbkFncPtr2Type)
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);


extern VAR(uint8 **, COMINL_VAR_8BIT) COMINL_u8Truc;
extern uint8                          Toto;


/* Scalar */
extern uint8  Toto;
extern uint16 Truc;
extern uint32 pppp;

extern uint8           Toto;
extern uint16          Truc;
extern volatile uint32 pppp;

extern VAR(uint8, COMINL_VAR_8BIT)  COMINL_u8Truc;
extern VAR(uint16, COMINL_VAR_8BIT) COMINL_u16Pouf;


/* Pointers */
extern uint8      *Juo;
extern uint32Type *pouf;
extern uint1      *pap;
extern uint1      *point;

extern uint8          *Juo;
extern uint32Type     *pouf;
extern volatile uint1 *pap;
extern uint1          *point;

extern P2VAR(uint8, COMINL_VAR_8BIT)  COMINL_pu8Truc;
extern P2VAR(uint16, COMINL_VAR_8BIT) COMINL_pu16Pouf;


/* Array */
extern uint8                     COMINL_u8VarReceiveMsg[COMINL_udtNB_MESSAGES_RX];
extern COMINL_tstrVarTransmitMsg COMINL_kastrVarTransmitMsg[REDXSCOMINL_udtNB_MESSAGES_TX];

extern uint8                     COMINL_u8VarReceiveMsg[COMINL_udtNB_MESSAGES_RX][2][4];
extern uint8                     COMINL_u8treztze[1][2][4][8];
extern uint16                    COMINL_u16treztze[2][4];
extern COMINL_tstrVarTransmitMsg COMINL_kastrVarTransmitMsg[REDXSCOMINL_udtNB_MESSAGES_TX];

extern VAR(COMINL_tstrVarReceiveMsg, COMINL_CONST)  COMINL_kastrVarReceiveMsg[COMINL_udtNB_MESSAGES_RX];
extern VAR(COMINL_tstrVarTransmitMsg, COMINL_CONST) COMINL_kastrVarTransmitMsg[COMINL_udtNB_MESSAGES_TX];
extern VAR(uint8, COMINL_CONST)                     COMINL_u8VarReceiveMsg[COMINL_udtNB_MESSAGES_RX][2][4];
extern VAR(uint8, COMINL_CONST)                     COMINL_u8treztze[1][2][4][8];
extern VAR(uint16, COMINL_CONST)                    COMINL_u16treztze[2][4];
extern VAR(COMINL_tstrVarTransmitMsg, COMINL_CONST) COMINL_kastrVarTransmitMsg[REDXSCOMINL_udtNB_MESSAGES_TX];


/* Scalar */
extern const COMINL_tudtTickType COMINL_kudtMSG3_RX_SIZE_4_DM_7MS_DM;
extern const uint8               COMINL_kudtMSG4_RX_SIZE_7_DM_5MS_DM;
extern const COMINL_tudtTickType COMINL_kudtMSG4_TX_SIZE_1_DM_10MS_DM;
extern const uint8_least         COMINL_kudtMSG5_TX_SIZE_3_DM_7MS_DM;

extern CONST(COMINL_tudtTickType, COMINL_CONST) COMINL_kudtMSG3_RX_SIZE_4_DM_7MS_DM;
extern CONST(uint8, COMINL_CONST)               COMINL_kudtMSG4_RX_SIZE_7_DM_5MS_DM;
extern CONST(COMINL_tudtTickType, COMINL_CONST) COMINL_kudtMSG4_TX_SIZE_1_DM_10MS_DM;
extern CONST(uint8_least, COMINL_CONST)         COMINL_kudtMSG5_TX_SIZE_3_DM_7MS_DM;


/* Array */
/* in one line */
extern const uint8 COMINL_u8VarReceiveMsg[5];


/* in one line but { and } are in separate line*/
extern uint8 COMINL_kastrVarTransmitMsg[3];


/* Multi dimlensinnal array  */
/* in one line */
extern const uint8 COMINL_u8VarReceiveMsg[5][3];
extern uint8       COMINL_kastrVarTransmitMsg[5][3];
extern uint8       COMINL_kastrVarTransmitMsg[5][3];

FUNC(Std_ReturnType, SPI_CODE) Spi_SetupBuffers
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);

FUNC(Std_ReturnType, SPI_CODE) Spi_SetupBuffers
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
)
{
   int                                           v1;
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) v2;
   int                                           v3;


   if (v1 == v3)
   {
      v2++;
   }
}

void fcn
(
   int          p1,
   unsigned int p2,
   long         p3
);


P2FUNC(void, NVM_APPL_CODE, NvM_CbkFncPtr1)
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);

P2FUNC(P2VAR(int, AUTOMATIC, SPI_APPL_DATA), NVM_APPL_CODE, NvM_CbkFncPtr2)
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);


FUNC(Std_ReturnType, SPI_CODE)(*MyFuncPtr1)
(
   Spi_ChannelType                                 Channel,
   int                                             prm2,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA) SrcDataBufferPtr,
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA)   DesDataBufferPtr,
   Spi_NumberOfDataType                            Length
);

FUNC(Std_ReturnType, SPI_CODE)(*MyFuncPtr2)
(
   Spi_ChannelType,
   int,
   P2CONST(Spi_DataType, AUTOMATIC, SPI_APPL_DATA),
   P2VAR(Spi_DataType, AUTOMATIC, SPI_APPL_DATA),
   Spi_NumberOfDataType
);

VAR(uint8 **, COMINL_VAR_8BIT) COMINL_u8Truc;
uint8                          Toto;


/* Scalar */
uint8  Toto;
uint16 Truc;
uint32 pppp;

uint8           Toto;
uint16          Truc;
volatile uint32 pppp;

VAR(uint8, COMINL_VAR_8BIT)  COMINL_u8Truc;
VAR(uint16, COMINL_VAR_8BIT) COMINL_u16Pouf;


/* Pointers */
uint8      *Juo;
uint32Type *pouf;
uint1      *pap;
uint1      *point;

uint8          *Juo;
uint32Type     *pouf;
volatile uint1 *pap;
uint1          *point;

P2VAR(uint8, COMINL_VAR_8BIT)  COMINL_pu8Truc;
P2VAR(uint16, COMINL_VAR_8BIT) COMINL_pu16Pouf;


/* Array */
uint8                     COMINL_u8VarReceiveMsg[COMINL_udtNB_MESSAGES_RX];
COMINL_tstrVarTransmitMsg COMINL_kastrVarTransmitMsg[REDXSCOMINL_udtNB_MESSAGES_TX];

uint8                     COMINL_u8VarReceiveMsg[COMINL_udtNB_MESSAGES_RX][2][4];
uint8                     COMINL_u8treztze[1][2][4][8];
uint16                    COMINL_u16treztze[2][4];
COMINL_tstrVarTransmitMsg COMINL_kastrVarTransmitMsg[REDXSCOMINL_udtNB_MESSAGES_TX];

VAR(COMINL_tstrVarReceiveMsg, COMINL_CONST)  COMINL_kastrVarReceiveMsg[COMINL_udtNB_MESSAGES_RX];
VAR(COMINL_tstrVarTransmitMsg, COMINL_CONST) COMINL_kastrVarTransmitMsg[COMINL_udtNB_MESSAGES_TX];
VAR(uint8, COMINL_CONST)                     COMINL_u8VarReceiveMsg[COMINL_udtNB_MESSAGES_RX][2][4];
VAR(uint8, COMINL_CONST)                     COMINL_u8treztze[1][2][4][8];
VAR(uint16, COMINL_CONST)                    COMINL_u16treztze[2][4];
VAR(COMINL_tstrVarTransmitMsg, COMINL_CONST) COMINL_kastrVarTransmitMsg[REDXSCOMINL_udtNB_MESSAGES_TX];


/* Scalar */
const COMINL_tudtTickType COMINL_kudtMSG3_RX_SIZE_4_DM_7MS_DM;
const uint8               COMINL_kudtMSG4_RX_SIZE_7_DM_5MS_DM;
const COMINL_tudtTickType COMINL_kudtMSG4_TX_SIZE_1_DM_10MS_DM;
const uint8_least         COMINL_kudtMSG5_TX_SIZE_3_DM_7MS_DM;

CONST(COMINL_tudtTickType, COMINL_CONST) COMINL_kudtMSG3_RX_SIZE_4_DM_7MS_DM;
CONST(uint8, COMINL_CONST)               COMINL_kudtMSG4_RX_SIZE_7_DM_5MS_DM;
CONST(COMINL_tudtTickType, COMINL_CONST) COMINL_kudtMSG4_TX_SIZE_1_DM_10MS_DM;
CONST(uint8_least, COMINL_CONST)         COMINL_kudtMSG5_TX_SIZE_3_DM_7MS_DM;


/* Array */
/* in one line */
const uint8 COMINL_u8VarReceiveMsg[5];


/* in one line but { and } are in separate line*/
uint8 COMINL_kastrVarTransmitMsg[3];


/* Multi dimlensinnal array  */
/* in one line */
const uint8 COMINL_u8VarReceiveMsg[5][3];
uint8       COMINL_kastrVarTransmitMsg[5][3];
uint8       COMINL_kastrVarTransmitMsg[5][3];


/*------------------------------- end of file --------------------------------*/
