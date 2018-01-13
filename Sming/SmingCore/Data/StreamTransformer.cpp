/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * @author Slavey Karadzhov <slaff@attachix.com>
 *
 ****/

#include "StreamTransformer.h"

#include "../../Services/WebHelpers/base64.h"

StreamTransformer::StreamTransformer(ReadWriteStream *stream, StreamTransformerCallback callback, size_t readSize/* = 1024 */)
{
	this->stream = stream;
	this->callback = callback;
	this->readSize = readSize;
}

StreamTransformer::~StreamTransformer()
{
	delete tempStream;
	delete stream;
	tempStream = NULL;
	stream = NULL;
}

size_t StreamTransformer::write(uint8_t charToWrite)
{
	return stream->write(charToWrite);
}

size_t StreamTransformer::write(const uint8_t *buffer, size_t size)
{
	return stream->write(buffer, size);
}

uint16_t StreamTransformer::readMemoryBlock(char* data, int bufSize)
{
	if(stream == NULL || stream->isFinished()) {
		return 0;
	}

	if(tempStream == NULL) {
		tempStream = new CircularBuffer(readSize + 10);
	}

	if(!tempStream->isFinished()) {
		return tempStream->readMemoryBlock(data, bufSize);
	}

	// pump new data into the stream
	int len = readSize;
	char buffer[len];
	len = stream->readMemoryBlock(buffer, len);
	stream->seek(std::max(len, 0));
	if(len < 1) {
		return 0;
	}

	int encodedLength = 0;
	uint8_t *encodedData = callback((uint8_t *)buffer, len, &encodedLength);
	tempStream->write(encodedData, encodedLength);
	delete[] encodedData;

	return tempStream->readMemoryBlock(data, bufSize);
}

//Use base class documentation
bool StreamTransformer::seek(int len)
{
	return tempStream->seek(len);
}

//Use base class documentation
bool StreamTransformer::isFinished()
{
	return (stream->isFinished() && tempStream->isFinished());
}

// Some Transformers...

uint8_t* base64StreamTransformer(uint8_t* data, int length, int* outputLength)
{
	*outputLength = (length / 3 )* 4;
	uint8_t* hash = new uint8_t[*outputLength];
	base64_encode(length, data, *outputLength, (char *)hash);
	return hash;
}

