#include <consensus/consensus.h>

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
unsigned int dgpMaxBlockSerSize = 8000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
unsigned int dgpMaxBlockWeight = 8000000;

unsigned int dgpMaxBlockSize = 2000000; // qtum

/** The maximum allowed number of signature check operations in a block (network rule) */
int64_t dgpMaxBlockSigOps = 80000;

unsigned int dgpMaxProtoMsgLength = 8000000;

unsigned int dgpMaxTxSigOps = 16000;

void updateBlockSizeParams(unsigned int newBlockSize){
    unsigned int newSizeForParams=WITNESS_SCALE_FACTOR*newBlockSize;
    dgpMaxBlockSerSize=newSizeForParams;
    dgpMaxBlockWeight=newSizeForParams;
    dgpMaxBlockSigOps=(int64_t)(newSizeForParams/100);
    dgpMaxTxSigOps = (unsigned int)(dgpMaxBlockSigOps/5);
    dgpMaxProtoMsgLength=newSizeForParams;
}
