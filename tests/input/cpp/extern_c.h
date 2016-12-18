#ifndef cRecordMarking_HEADER
#define cRecordMarking_HEADER

#include "DIS/cPduSnapshot.h"

typedef void* disConnectionH;

#ifdef __cplusplus
extern "C"
#endif
{
	
	disConnectionH createDisConnection();
	
	void setAddressAndPort_DisConnect(disConnectionH record, const char *addr);
	
    /* Open network connection */
    int open_DisConnect(disConnectionH record);
	
    /* Close network connection */
    void close_DisConnect(disConnectionH record);

	/* Send one pdu */
    int sendPdu_DisConnect(disConnectionH record, pduSnapshotH pdu);
	
    /* Receive one pdu */
    int recvPdu_DisConnect(disConnectionH record, pduSnapshotH pdu);
	
	void FreeDisConnection(disConnectionH connection);
	
}
#endif

