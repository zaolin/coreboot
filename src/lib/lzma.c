/* 

Coreboot interface to memory-saving variant of LZMA decoder

(C)opyright 2006 Carl-Daniel Hailfinger
Released under the GNU GPL v2 or later

Parts of this file are based on C/7zip/Compress/LZMA_C/LzmaTest.c from the LZMA
SDK 4.42, which is written and distributed to public domain by Igor Pavlov.

*/

#include "lzmadecode.c"
#include <console/console.h>
#include <string.h>


unsigned long ulzma(unsigned char * src, unsigned char * dst)
{
	unsigned char properties[LZMA_PROPERTIES_SIZE];
	UInt32 outSize;
	SizeT inProcessed;
	SizeT outProcessed;
	int res;
	CLzmaDecoderState state;
	SizeT mallocneeds;
	unsigned char scratchpad[15980];

	memcpy(properties, src, LZMA_PROPERTIES_SIZE);
	outSize = *(UInt32 *)(src + LZMA_PROPERTIES_SIZE);
	if (LzmaDecodeProperties(&state.Properties, properties, LZMA_PROPERTIES_SIZE) != LZMA_RESULT_OK) {
		printk_warning("Incorrect stream properties\n");
		return 0;
	}
	mallocneeds = (LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
	if (mallocneeds > 15980) {
		printk_warning("Decoder scratchpad too small!\n");
		return 0;
	}
	state.Probs = (CProb *)scratchpad;
	res = LzmaDecode(&state, src + LZMA_PROPERTIES_SIZE + 8, (SizeT)0xffffffff, &inProcessed,
		dst, outSize, &outProcessed);
	if (res != 0) {
		printk_warning("Decoding error = %d\n", res);
		return 0;
	}
	return outSize;
}
