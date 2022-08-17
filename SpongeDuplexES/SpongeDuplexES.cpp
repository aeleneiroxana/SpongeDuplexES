#include <iostream>
#include <fstream>
#include <random>
#define DEFAULT_SIZE 16
#define BITRATE 32
#define FILE_MAX_SIZE (1<<64) - 1
#define STATE_SIZE 80
using namespace std;

#pragma region Data Types

typedef union {
	unsigned char val;
	struct {
		unsigned int b7 : 1;
		unsigned int b6 : 1;
		unsigned int b5 : 1;
		unsigned int b4 : 1;
		unsigned int b3 : 1;
		unsigned int b2 : 1;
		unsigned int b1 : 1;
		unsigned int b0 : 1;
	};
}byte;

typedef struct {
	int r;
	int c;
	byte bytes[STATE_SIZE];
}state;
#pragma endregion

#pragma region Reading Data
char* readFile(const char fileName[]) {
	ifstream in(fileName);
	size_t size = 1;
	char* str;
	char ch;
	size_t len = 0;
	str = (char*)realloc(NULL, sizeof(*str) * size);
	if (!str)return str;
	while (in.get(ch)) {
		str[len++] = ch;
		if (len == size) {
			str = (char*)realloc(str, sizeof(*str) * (size += 16));
			if (!str)return str;
		}
	}
	str[len++] = '\0';

	return (char*)realloc(str, sizeof(*str) * len);
}
#pragma endregion

#pragma region Printing Data

void printBytes(byte value[], int size)
{
	for (unsigned long long i = 0; i < size; i++)
		cout << (int)value[i].val << " ";
	cout << endl;
}

void printChars(byte value[], int size)
{
	for (unsigned long long i = 0; i < size; i++)
		cout << value[i].val;
	cout << endl;
}

void printState(state s)
{
	for (unsigned long long i = 0; i < STATE_SIZE; i++)
	{
		cout << (int)s.bytes[i].val << " ";
		if (i % 16 == 15)
			cout << endl;
	}
	cout << endl;
}

#pragma endregion

void invert(byte value[], byte invertedValue[], unsigned long long size)
{
	for (unsigned long long i = 0; i < size; i++) {
		invertedValue[i].val = 0;
		invertedValue[i].b0 = (value[i].b0) ^ 1;
		invertedValue[i].b1 = (value[i].b1) ^ 1;
		invertedValue[i].b2 = (value[i].b2) ^ 1;
		invertedValue[i].b3 = (value[i].b3) ^ 1;
		invertedValue[i].b4 = (value[i].b4) ^ 1;
		invertedValue[i].b5 = (value[i].b5) ^ 1;
		invertedValue[i].b6 = (value[i].b6) ^ 1;
		invertedValue[i].b7 = (value[i].b7) ^ 1;
	}
}

void updatePermutation(byte value, int v[4])
{
	int andSum = value.b4 & value.b5 & value.b6 & value.b7;
	int orSum = value.b4 | value.b5 | value.b6 | value.b7;

	if (value.b5)
	{
		int aux = v[0];
		v[0] = v[2];
		v[2] = aux;
	}
	if (value.b6)
	{
		int aux = v[1];
		v[1] = v[3];
		v[3] = aux;
	}
	if (value.b7)
	{
		int aux = v[0];
		v[0] = v[1];
		v[1] = aux;
	}
	if (value.b4)
	{
		int aux = v[2];
		v[2] = v[3];
		v[3] = aux;
	}

	if (andSum) {
		v[0] = 3;
		v[1] = 1;
		v[2] = 0;
		v[3] = 2;
	}

	if (orSum == 0) {
		v[0] = 0;
		v[1] = 2;
		v[2] = 3;
		v[3] = 1;
	}
}

void shuffleBytes(byte value[], byte shuffledValue[], unsigned long long size) {
	int blockSize = size / 4;
	int v[4] = { 0, 1, 2, 3 };
	for (unsigned long long i = blockSize - 1; i < size; i += blockSize)
		updatePermutation(value[i], v);

	memcpy(shuffledValue, value + v[0] * blockSize, sizeof(byte) * blockSize);
	memcpy(shuffledValue + blockSize, value + v[1] * blockSize, sizeof(byte) * blockSize);
	memcpy(shuffledValue + 2 * blockSize, value + v[2] * blockSize, sizeof(byte) * blockSize);
	memcpy(shuffledValue + 3 * blockSize, value + v[3] * blockSize, sizeof(byte) * blockSize);
}

void xorBytes(byte value1[], byte value2[], byte xoredValue[], unsigned long long size)
{
	for (unsigned long long i = 0; i < size; i++) {
		xoredValue[i].val = value1[i].val ^ value2[i].val;
	}
}

int each(byte bytes[], unsigned long long size, unsigned long long blockSize) {
	int sum = 0;
	for (unsigned long long i = blockSize - 1; i < size; i += blockSize)
		sum += bytes[i].b7;

	return sum + 4;
}

void clearBytes(byte bytes[], unsigned long long size) {
	for (unsigned long long i = 0; i < size; i++) {
		bytes[i].val = 0;
	}
}

//finished but keep it for reference
int sbox(byte input) {
	byte output = { 0 };
	int x[4] = { input.b4, input.b5, input.b6, input.b7 };
	output.b4 = x[0] ^ x[1] ^ x[3] ^ 1;
	output.b5 = x[0] ^ x[2] ^ x[3];
	output.b6 = x[1] ^ x[2] ^ x[3];
	output.b7 = x[0];

	return output.val;
}

void sbox(state& s) {
	int blockSize = STATE_SIZE / 4;
	byte w0[STATE_SIZE / 4], w1[STATE_SIZE / 4], w2[STATE_SIZE / 4], w3[STATE_SIZE / 4], defVal[STATE_SIZE / 4];
	for (unsigned long long i = 0; i < blockSize; i++)
		defVal[i].val = 128;
	xorBytes(s.bytes, s.bytes + blockSize, w0, blockSize);
	xorBytes(w0, s.bytes + 3 * blockSize, w0, blockSize);
	xorBytes(w0, defVal, w0, blockSize);
	xorBytes(s.bytes, s.bytes + 2 * blockSize, w1, blockSize);
	xorBytes(w1, s.bytes + 3 * blockSize, w1, blockSize);
	xorBytes(s.bytes + blockSize, s.bytes + 2 * blockSize, w2, blockSize);
	xorBytes(w2, s.bytes + 3 * blockSize, w2, blockSize);
	memcpy(w3, s.bytes, sizeof(byte) * blockSize);

	memcpy(s.bytes, w0, sizeof(byte) * blockSize);
	memcpy(s.bytes + 1 * blockSize, w1, sizeof(byte) * blockSize);
	memcpy(s.bytes + 2 * blockSize, w2, sizeof(byte) * blockSize);
	memcpy(s.bytes + 3 * blockSize, w3, sizeof(byte) * blockSize);
}

void pbox(state& s) {
	int blockSize = STATE_SIZE / 4;
	byte w0[STATE_SIZE / 4], w1[STATE_SIZE / 4], w2[STATE_SIZE / 4], w3[STATE_SIZE / 4];
	shuffleBytes(s.bytes, w0, blockSize);
	shuffleBytes(s.bytes + 1 * blockSize, w1, blockSize);
	shuffleBytes(s.bytes + 2 * blockSize, w2, blockSize);
	shuffleBytes(s.bytes + 3 * blockSize, w3, blockSize);

	memcpy(s.bytes, w0, sizeof(byte) * blockSize);
	memcpy(s.bytes + 1 * blockSize, w1, sizeof(byte) * blockSize);
	memcpy(s.bytes + 2 * blockSize, w2, sizeof(byte) * blockSize);
	memcpy(s.bytes + 3 * blockSize, w3, sizeof(byte) * blockSize);
}

void f(state& s, int rounds) {
	for (unsigned long long i = 1; i <= rounds; i++) {
		sbox(s);
		pbox(s);
	}

	if (rounds % 2)
		sbox(s);
	else
		pbox(s);
}

void initializeState(state& s, byte key[DEFAULT_SIZE], byte iv[DEFAULT_SIZE]) {
	byte niv[DEFAULT_SIZE], shuffledKey[DEFAULT_SIZE], xoredValue[DEFAULT_SIZE];
	invert(iv, niv, DEFAULT_SIZE);
	shuffleBytes(key, shuffledKey, DEFAULT_SIZE);
	xorBytes(key, iv, xoredValue, DEFAULT_SIZE);

	memcpy(s.bytes, key, sizeof(byte) * DEFAULT_SIZE);
	memcpy(s.bytes + DEFAULT_SIZE, niv, sizeof(byte) * DEFAULT_SIZE);
	memcpy(s.bytes + 2 * DEFAULT_SIZE, shuffledKey, sizeof(byte) * DEFAULT_SIZE);
	memcpy(s.bytes + 3 * DEFAULT_SIZE, iv, sizeof(byte) * DEFAULT_SIZE);
	memcpy(s.bytes + 4 * DEFAULT_SIZE, xoredValue, sizeof(byte) * DEFAULT_SIZE);

	f(s, each(s.bytes, STATE_SIZE, 2));
}

byte* fillByteArray(byte* bytes, const char* chars, unsigned long long& size) {
	for (unsigned long long i = 0;i < size; i++)
	{
		bytes[i].val = chars[i];
	}

	bytes = (byte*)realloc(bytes, sizeof(byte) * (size + 1));
	bytes[size].val = 128;
	size++;
	while (size % BITRATE != 0)
	{
		bytes = (byte*)realloc(bytes, sizeof(byte) * (size + 1));
		bytes[size].val = 0;
		size++;
	}
	return bytes;
}

void processAD(state& s, byte ad[], unsigned long long adSize) {
	for (unsigned long long i = 0; i < adSize; i += BITRATE)
	{
		xorBytes(s.bytes, ad + i, s.bytes, BITRATE);
		f(s, each(ad + i, BITRATE, 1));
	}
}

byte* encrypt(state& s, byte key[], byte plaintext[], byte ciphertext[], unsigned long long plaintextSize) {
	byte* aux = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE * (plaintextSize / BITRATE));
	unsigned long long auxPos = 0;

	for (unsigned long long i = 0; i < plaintextSize; i += BITRATE)
	{
		xorBytes(s.bytes, plaintext + i, s.bytes, BITRATE);
		memcpy(ciphertext + i, s.bytes, sizeof(byte) * BITRATE);
		f(s, each(plaintext + i, BITRATE, 1));

		f(s, each(plaintext, plaintextSize, 1));

		byte xoredValue[DEFAULT_SIZE];
		xorBytes(key, s.bytes + s.r, xoredValue, DEFAULT_SIZE);

		memcpy(aux + auxPos * DEFAULT_SIZE, xoredValue, DEFAULT_SIZE);
		auxPos++;
	}

	return aux;
}

byte* getTag(state& s, byte aux[], byte ciphertext[], unsigned long long plaintextSize) {
	f(s, each(s.bytes, DEFAULT_SIZE, 1));
	byte t[DEFAULT_SIZE], * shuffledAux;
	unsigned long long auxSize = DEFAULT_SIZE * (plaintextSize / BITRATE);
	shuffledAux = (byte*)malloc(sizeof(byte) * auxSize);
	shuffleBytes(aux, shuffledAux, auxSize);
	xorBytes(s.bytes + s.r, shuffledAux, t, DEFAULT_SIZE);

	free(shuffledAux);
	return t;
}

byte* decrypt(state& s, byte key[], byte plaintext[], byte ciphertext[], unsigned long long plaintextSize) {
	byte* aux = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE * (plaintextSize / BITRATE));
	unsigned long long auxPos = 0;
	for (unsigned long long i = 0; i < plaintextSize; i += BITRATE)
	{
		xorBytes(s.bytes, ciphertext + i, s.bytes, BITRATE);
		memcpy(plaintext + i, s.bytes, sizeof(byte) * BITRATE);
		f(s, each(plaintext + i, BITRATE, 1));

		byte xoredValue[DEFAULT_SIZE];
		xorBytes(key, s.bytes + s.r, xoredValue, DEFAULT_SIZE);

		memcpy(aux + auxPos * DEFAULT_SIZE, xoredValue, DEFAULT_SIZE);
		auxPos++;
	}

	return aux;
}

bool validTag(byte receivedTag[], byte tag[]) {
	for (int i = 0; i < DEFAULT_SIZE; i++)
		if (receivedTag[i].val != tag[i].val)
			return false;

	return true;
}

int main()
{
	//get initial values and initialize state
	byte key[DEFAULT_SIZE], iv[DEFAULT_SIZE];
	state s = { BITRATE, STATE_SIZE - BITRATE };
	for (int i = 0; i < DEFAULT_SIZE; i++)
	{
		key[i].val = rand();
		iv[i].val = rand();
	}

	initializeState(s, key, iv);

	//read associated data and plaintext
	char* adChar = readFile("associatedData.txt");
	char* plaintextChar = readFile("plaintext.txt");

	if (strlen(adChar) > FILE_MAX_SIZE || strlen(plaintextChar) > FILE_MAX_SIZE)
	{
		cout << "PLAINTEXT OR ASSOCIATED DATA TOO BIG";
		free(adChar);
		free(plaintextChar);
		return 0;
	}

	unsigned long long adSize = strlen(adChar);
	unsigned long long plaintextSize = strlen(plaintextChar);

	//preprocess associated data and plaintext
	byte* ad, * plaintext, * ciphertext;
	ad = (byte*)malloc(sizeof(byte) * adSize);
	ad = fillByteArray(ad, adChar, adSize);
	plaintext = (byte*)malloc(sizeof(byte) * plaintextSize);
	plaintext = fillByteArray(plaintext, plaintextChar, plaintextSize);
	ciphertext = (byte*)malloc(sizeof(byte) * (plaintextSize + DEFAULT_SIZE));

	//encrypt
	processAD(s, ad, adSize);
	byte* aux = encrypt(s, key, plaintext, ciphertext, plaintextSize);
	byte* t = getTag(s, aux, ciphertext, plaintextSize);
	memcpy(ciphertext + plaintextSize, t, sizeof(byte) * DEFAULT_SIZE);

	cout << "PLAINTEXT TEXT: ";
	printChars(plaintext, plaintextSize);
	cout << "CIPHERTEXT BYTES: ";
	printBytes(ciphertext, plaintextSize + DEFAULT_SIZE);
	cout << "TAG BYTES: ";
	printBytes(t, DEFAULT_SIZE);

	clearBytes(plaintext, plaintextSize);

	//decrypt
	initializeState(s, key, iv);
	processAD(s, ad, adSize);
	free(aux);
	aux = decrypt(s, key, plaintext, ciphertext, plaintextSize);
	byte* rt = getTag(s, aux, ciphertext, plaintextSize);
	cout << "PLAINTEXT TEXT: ";
	printChars(plaintext, plaintextSize);
	cout << "TAG BYTES: ";
	printBytes(rt, DEFAULT_SIZE);
	if (validTag(t, rt))
		cout << "VALID TAG";
	else
		cout << "INVALID TAG";

	free(ad);
	free(adChar);
	free(plaintext);
	free(plaintextChar);
	free(ciphertext);

	return 0;
}