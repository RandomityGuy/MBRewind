#pragma once
#include <cstdint>
#include <cstdbool>
#include <string>
/*
*	Original implementation of MemoryStream in C++
*/
class MemoryStream
{
	// THis is large cuz our replays uncompressed are large lol
	const int REALLOCATE_SIZE = 32768;

	uint8_t* buffer;
	size_t bufferSize;
	size_t properSize;
	size_t position;

	bool checkEos(bool error = true);
	void reallocate();
public:
	MemoryStream();
	~MemoryStream();
	void createFromBuffer(uint8_t* buffer, size_t count);
	bool readBool();
	int64_t readInt64();
	int32_t readInt32();
	int16_t readInt16();
	int8_t readInt8();
	uint64_t readUInt64();
	uint32_t readUInt32();
	uint16_t readUInt16();
	uint8_t readUInt8();
	float readFloat();
	double readDouble();
	char readChar();
	unsigned char readUChar();
	std::string readString();

	template<typename T>
	void write(T);

	template<typename T>
	T read();

	void writeBool(bool);
	void writeInt64(int64_t);
	void writeInt32(int32_t);
	void writeInt16(int16_t);
	void writeInt8(int8_t);
	void writeUInt64(uint64_t);
	void writeUInt32(uint32_t);
	void writeUInt16(uint16_t);
	void writeUInt8(uint8_t);
	void writeFloat(float);
	void writeDouble(double);
	void writeChar(char);
	void writeUChar(unsigned char);
	void writeString(std::string);
	void seek(size_t position);
	size_t tell();
	size_t length();
	uint8_t* getBuffer();
};

