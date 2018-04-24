#include "ChunkedStream.h"

ChunkedStream::ChunkedStream(ReadWriteStream *stream, size_t resultSize /* = 512 */):
	StreamTransformer(stream, nullptr, resultSize, resultSize - 12)
{
}

int ChunkedStream::encode(uint8_t* source, size_t* sourceLength,
						  uint8_t* target, size_t targetLength)
{
	if(*sourceLength == 0) {
		const char* end = "0\r\n\r\n";
		memcpy(target, end, strlen(end));
		return strlen(end);
	}

	int offset = 0;
	char chunkSize[5] = {0};
	// <chunkSize>"\r\n"
	ultoa(*sourceLength, chunkSize, 10);

	memcpy(target, chunkSize, strlen(chunkSize));
	offset += strlen(chunkSize);

	// \r\n
	memcpy(target + offset, "\r\n", 2);
	offset += 2;

	memcpy(target + offset, source, *sourceLength);
	offset += *sourceLength;

	// \r\n
	memcpy(target + offset, "\r\n", 2);
	offset += 2;

	return offset;
}
