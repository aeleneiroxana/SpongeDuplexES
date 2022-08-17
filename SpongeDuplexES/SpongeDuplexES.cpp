#include <iostream>
#include <fstream>
#include <random>

#define DEFAULT_SIZE 16
#define BITRATE 32
#define FILE_MAX_SIZE (1<<64) - 1
#define STATE_SIZE 80

using namespace std;

char defaultValue = DEFAULT_SIZE * 8;

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

void printBytes(byte value[], unsigned long long size)
{
	for (unsigned long long i = 0; i < size; i++)
		printf("%3d ", value[i].val);
	cout << endl;
}

void printChars(byte value[], unsigned long long size)
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
		if (i % DEFAULT_SIZE == DEFAULT_SIZE - 1)
			cout << endl;
	}
	cout << endl;
}

#pragma endregion

#pragma region Helper Methods

void copyBytes(byte* dest, byte* source, unsigned long long size, unsigned long long offset = 0) {
	for (int i = 0; i < size; i++)
		dest[offset + i].val = source[i].val;
}

void invert(byte* value, byte* invertedValue, unsigned long long size)
{
	for (unsigned long long i = 0; i < size; i++) {
		invertedValue[i].val = value[i].val ^ defaultValue;
	}
}

void updatePermutation(byte value, int* v)
{
	bool andSum = (value.val == defaultValue);
	int orSum = (value.val == 0);

	if (value.b5 ^ value.b1)
	{
		int aux = v[0];
		v[0] = v[2];
		v[2] = aux;
	}
	if (value.b6 ^ value.b2)
	{
		int aux = v[1];
		v[1] = v[3];
		v[3] = aux;
	}
	if (value.b7 ^ value.b3)
	{
		int aux = v[0];
		v[0] = v[1];
		v[1] = aux;
	}
	if (value.b4 ^ value.b0)
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

	if (orSum) {
		v[0] = 0;
		v[1] = 2;
		v[2] = 3;
		v[3] = 1;
	}
}

void shuffleBytes(byte* value, byte* shuffledValue, unsigned long long size) {
	int blockSize = size / 4;
	int v[4] = { 0, 1, 2, 3 };
	for (unsigned long long i = 0; i < size; i += blockSize)
		updatePermutation(value[i], v);

	copyBytes(shuffledValue, value + v[0] * blockSize, blockSize);
	copyBytes(shuffledValue, value + v[1] * blockSize, blockSize, blockSize);
	copyBytes(shuffledValue, value + v[2] * blockSize, blockSize, 2 * blockSize);
	copyBytes(shuffledValue, value + v[3] * blockSize, blockSize, 3 * blockSize);
}

void xorBytes(byte* value1, byte* value2, byte* xoredValue, unsigned long long size)
{
	for (unsigned long long i = 0; i < size; i++) {
		xoredValue[i].val = value1[i].val ^ value2[i].val;
	}
}

int each(byte* bytes, unsigned long long size, unsigned long long blockSize) {
	int sum = 0;
	for (unsigned long long i = 0; i < size; i += blockSize)
		sum += bytes[i].val;

	int value = (sum % 47) + 16;
	return value;
}

void clearBytes(byte* bytes, unsigned long long size) {
	for (unsigned long long i = 0; i < size; i++) {
		bytes[i].val = 0;
	}
}

int mergeBytes(byte msb, byte lsb) {
	byte result;
	result.val = 0;
	result.b0 = msb.b4;
	result.b1 = msb.b5;
	result.b2 = msb.b6;
	result.b3 = msb.b7;
	result.b4 = lsb.b4;
	result.b5 = lsb.b5;
	result.b6 = lsb.b6;
	result.b7 = lsb.b7;

	return result.val;
}

#pragma endregion

#pragma region F Function

int sbox(byte input, bool lsb = true) {
	byte output = { 0 };
	int x[4];
	if (lsb)
	{
		x[0] = input.b4;
		x[1] = input.b5;
		x[2] = input.b6;
		x[3] = input.b7;
	}
	else {
		x[0] = input.b0;
		x[1] = input.b1;
		x[2] = input.b2;
		x[3] = input.b3;
	}
	output.b4 = x[0] ^ x[1] ^ x[3] ^ 1;
	output.b5 = x[0] ^ x[2] ^ x[3];
	output.b6 = x[1] ^ x[2] ^ x[3];
	output.b7 = x[0];

	return output.val;
}

void sbox(state& s) {
	int blockSize = STATE_SIZE / 4;

	byte w0[STATE_SIZE / 4], w1[STATE_SIZE / 4], w2[STATE_SIZE / 4], w3[STATE_SIZE / 4];
	copyBytes(w0, s.bytes, blockSize);
	copyBytes(w1, s.bytes + blockSize, blockSize);
	copyBytes(w2, s.bytes + 2 * blockSize, blockSize);
	copyBytes(w3, s.bytes + 3 * blockSize, blockSize);

	for (int i = 0; i < blockSize; i++) {
		byte ans[8], answ[8];

		ans[0].b4 = w0[i].b0;
		ans[0].b5 = w1[i].b0;
		ans[0].b6 = w2[i].b0;
		ans[0].b7 = w3[i].b0;
		ans[1].b4 = w0[i].b1;
		ans[1].b5 = w1[i].b1;
		ans[1].b6 = w2[i].b1;
		ans[1].b7 = w3[i].b1;
		ans[2].b4 = w0[i].b2;
		ans[2].b5 = w1[i].b2;
		ans[2].b6 = w2[i].b2;
		ans[2].b7 = w3[i].b2;
		ans[3].b4 = w0[i].b3;
		ans[3].b5 = w1[i].b3;
		ans[3].b6 = w2[i].b3;
		ans[3].b7 = w3[i].b3;
		ans[4].b4 = w0[i].b4;
		ans[4].b5 = w1[i].b4;
		ans[4].b6 = w2[i].b4;
		ans[4].b7 = w3[i].b4;
		ans[5].b4 = w0[i].b5;
		ans[5].b5 = w1[i].b5;
		ans[5].b6 = w2[i].b5;
		ans[5].b7 = w3[i].b5;
		ans[6].b4 = w0[i].b6;
		ans[6].b5 = w1[i].b6;
		ans[6].b6 = w2[i].b6;
		ans[6].b7 = w3[i].b6;
		ans[7].b4 = w0[i].b7;
		ans[7].b5 = w1[i].b7;
		ans[7].b6 = w2[i].b7;
		ans[7].b7 = w3[i].b7;

		for (int j = 0; j < 8; j += 2)
		{
			answ[j].val = sbox(ans[j]);
			answ[j + 1].val = sbox(ans[j + 1]);
		}

		w0[i].val = mergeBytes(answ[0], answ[1]);
		w1[i].val = mergeBytes(answ[2], answ[3]);
		w2[i].val = mergeBytes(answ[4], answ[5]);
		w3[i].val = mergeBytes(answ[6], answ[7]);
	}
	copyBytes(s.bytes, w0, blockSize);
	copyBytes(s.bytes, w1, blockSize, 1 * blockSize);
	copyBytes(s.bytes, w2, blockSize, 2 * blockSize);
	copyBytes(s.bytes, w3, blockSize, 3 * blockSize);
}

void pbox(state& s) {
	byte word[4], shuffled[4];
	for (int i = 0; i < STATE_SIZE; i++) {
		int pos = i + 1, cnt = 1;
		word[0].val = s.bytes[i].val;
		while (cnt < 4) {
			if (pos >= STATE_SIZE)
				pos = pos % STATE_SIZE;
			word[cnt].val = s.bytes[pos].val;
			cnt++;
			pos++;
		}
		shuffleBytes(word, shuffled, 4);
		cnt = 1;
		pos = i + 1;
		s.bytes[i].val = shuffled[0].val;
		while (cnt < 4) {
			if (pos >= STATE_SIZE)
				pos = pos % STATE_SIZE;
			s.bytes[pos].val = shuffled[cnt].val;
			cnt++;
			pos++;
		}
	}
}

void f(state& s, int rounds) {
	for (unsigned long long i = 1; i <= rounds; i++) {
		sbox(s);
		pbox(s);
	}
}

#pragma endregion

#pragma region Initialization

void initializeState(state& s, byte* key, byte* iv) {
	byte niv[DEFAULT_SIZE], shuffledKey[DEFAULT_SIZE], xoredValue[DEFAULT_SIZE];
	invert(iv, niv, DEFAULT_SIZE);
	shuffleBytes(key, shuffledKey, DEFAULT_SIZE);
	xorBytes(key, iv, xoredValue, DEFAULT_SIZE);

	copyBytes(s.bytes, key, DEFAULT_SIZE);
	copyBytes(s.bytes, niv, DEFAULT_SIZE, DEFAULT_SIZE);
	copyBytes(s.bytes, shuffledKey, DEFAULT_SIZE, 2 * DEFAULT_SIZE);
	copyBytes(s.bytes, iv, DEFAULT_SIZE, 3 * DEFAULT_SIZE);
	copyBytes(s.bytes, xoredValue, DEFAULT_SIZE, 4 * DEFAULT_SIZE);

	f(s, each(s.bytes, STATE_SIZE, 2));
}

byte* fillByteArray(byte* bytes, const char* chars, unsigned long long& size) {
	for (unsigned long long i = 0;i < size; i++)
	{
		bytes[i].val = chars[i];
	}

	bytes = (byte*)realloc(bytes, sizeof(byte) * (size + 1));
	bytes[size].val = defaultValue;
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

#pragma endregion

#pragma region Crypt

byte* encrypt(state& s, byte key[], byte plaintext[], byte ciphertext[], unsigned long long plaintextSize) {
	byte* aux = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE * (plaintextSize / BITRATE));
	unsigned long long auxPos = 0;

	for (unsigned long long i = 0; i < plaintextSize; i += BITRATE)
	{
		cout << i << " STATE BEFORE: ";
		printState(s);
		xorBytes(s.bytes, plaintext + i, s.bytes, s.r);
		copyBytes(ciphertext, s.bytes, s.r, i);
		cout << i << " STATE AFTER XOR: ";
		printState(s);
		f(s, each(s.bytes + s.c, s.r, 1));
		cout << i << " STATE AFTER F: ";
		printState(s);

		byte xoredValue[DEFAULT_SIZE];
		xorBytes(key, s.bytes + s.r, xoredValue, DEFAULT_SIZE);

		copyBytes(aux, xoredValue, DEFAULT_SIZE, auxPos * DEFAULT_SIZE);
		auxPos++;
	}

	return aux;
}

byte* decrypt(state& s, byte key[], byte plaintext[], byte ciphertext[], unsigned long long plaintextSize) {
	byte* aux = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE * (plaintextSize / BITRATE));
	unsigned long long auxPos = 0;

	for (unsigned long long i = 0; i < plaintextSize; i += BITRATE)
	{
		xorBytes(s.bytes, ciphertext + i, s.bytes, s.r);
		copyBytes(plaintext, s.bytes, s.r, i);
		copyBytes(s.bytes, ciphertext + i, s.r);
		f(s, each(s.bytes + s.c, s.r, 1));

		byte xoredValue[DEFAULT_SIZE];
		xorBytes(key, s.bytes + s.r, xoredValue, DEFAULT_SIZE);

		copyBytes(aux, xoredValue, DEFAULT_SIZE, auxPos * DEFAULT_SIZE);
		auxPos++;
	}

	return aux;
}

#pragma endregion

#pragma region Tag

byte* getTag(state& s, byte* aux, byte* ciphertext, unsigned long long plaintextSize) {
	f(s, each(s.bytes, DEFAULT_SIZE, 1));
	byte* t, * shuffledAux;
	t = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE);
	unsigned long long auxSize = DEFAULT_SIZE * (plaintextSize / BITRATE);
	shuffledAux = (byte*)malloc(sizeof(byte) * auxSize);
	shuffleBytes(aux, shuffledAux, auxSize);
	xorBytes(s.bytes + s.r, shuffledAux, t, DEFAULT_SIZE);

	free(shuffledAux);
	return t;
}

bool validTag(byte* receivedTag, byte* tag) {
	for (int i = 0; i < DEFAULT_SIZE; i++)
		if (receivedTag[i].val != tag[i].val)
			return false;

	return true;
}
#pragma endregion

int main()
{
	byte* key, * iv;
	key = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE);
	iv = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE);
	state s;
	s.r = BITRATE;
	s.c = STATE_SIZE - BITRATE;
	for (int i = 0; i < DEFAULT_SIZE; i++)
	{
		key[i].val = rand();
		iv[i].val = rand();
	}

	initializeState(s, key, iv);

	char* adChar = readFile("associatedData.txt");
	char* plaintextChar = readFile("plaintext.txt");
	unsigned long long adSize = strlen(adChar);
	unsigned long long plaintextSize = strlen(plaintextChar);

	byte* ad, * plaintext, * ciphertext;
	ad = (byte*)malloc(sizeof(byte) * adSize);
	ad = fillByteArray(ad, adChar, adSize);
	plaintext = (byte*)malloc(sizeof(byte) * plaintextSize);
	plaintext = fillByteArray(plaintext, plaintextChar, plaintextSize);
	ciphertext = (byte*)malloc(sizeof(byte) * (plaintextSize + DEFAULT_SIZE));

	processAD(s, ad, adSize);
	byte* aux = encrypt(s, key, plaintext, ciphertext, plaintextSize);
	byte* t = getTag(s, aux, ciphertext, plaintextSize);
	copyBytes(ciphertext, t, DEFAULT_SIZE, plaintextSize);

	cout << "PLAINTEXT TEXT: ";
	printChars(plaintext, plaintextSize);
	cout << "CIPHERTEXT BYTES: ";
	printBytes(ciphertext, plaintextSize + DEFAULT_SIZE);
	cout << endl << endl;

	initializeState(s, key, iv);
	processAD(s, ad, adSize);
	free(aux);
	clearBytes(plaintext, plaintextSize);

	aux = decrypt(s, key, plaintext, ciphertext, plaintextSize);
	byte* rt = getTag(s, aux, ciphertext, plaintextSize);
	cout << "PLAINTEXT TEXT: ";
	printChars(plaintext, plaintextSize);
	if (validTag(t, rt))
		cout << "VALID TAG";
	else
		cout << "INVALID TAG";

	free(key);
	free(iv);
	free(ad);
	free(adChar);
	free(plaintext);
	free(plaintextChar);
	free(ciphertext);

	return 0;
}