#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <cmath>
#include<iomanip>
#include<format>

#define DEFAULT_SIZE 16
#define BITRATE 32
#define FILE_MAX_SIZE (1<<64) - 1
#define STATE_SIZE 80
#define HEX( x ) setw(2) << setfill('0') << hex << (int)(x)
using namespace std;

char defaultValue = DEFAULT_SIZE * 8 - 1;

#pragma region Data Types

typedef union
{
    unsigned char val;
    struct
    {
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

typedef struct
{
    int bitrate;
    int capacity;
    byte bytes[STATE_SIZE];
}state;

typedef struct
{
    unsigned long long size;
    byte* value;
} bitstring;

#pragma endregion

#pragma region Reading Data

bitstring readFile(const char fileName[])
{
    ifstream t;
    int length;
    t.open(fileName, std::ifstream::binary);
    t.seekg(0, std::ios::end);
    length = t.tellg();
    t.seekg(0, std::ios::beg);
    char* str;
    str = (char*)calloc(length, sizeof(char));
    t.read(str, length);
    bitstring aux;
    aux.size = length;
    aux.value = (byte*)calloc(aux.size, sizeof(byte));
    for (int i = 0; i < aux.size; i++)
        aux.value[i].val = str[i];

    free(str);
    return aux;
}

#pragma endregion

#pragma region Printing Data

void printBytes(bitstring str)
{
    for (unsigned long long i = 0; i < str.size; i++)
        printf("%3d ", str.value[i].val);
    cout << endl;
}

void printChars(bitstring str)
{
    for (unsigned long long i = 0; i < str.size; i++)
        cout << str.value[i].val;
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

void copyBytes(byte* dest, byte* source, unsigned long long size, unsigned long long offset = 0)
{
    for (int i = 0; i < size; i++)
        dest[offset + i].val = source[i].val;
}

void invert(byte* value, byte* invertedValue, unsigned long long size)
{
    for (unsigned long long i = 0; i < size; i++)
    {
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

    if (andSum)
    {
        v[0] = 3;
        v[1] = 1;
        v[2] = 0;
        v[3] = 2;
    }

    if (orSum)
    {
        v[0] = 0;
        v[1] = 2;
        v[2] = 3;
        v[3] = 1;
    }
}

void shuffleBytes(byte* value, byte* shuffledValue, unsigned long long size)
{
    int blockSize = size / 4;
    int v[4] = {0, 1, 2, 3};
    for (unsigned long long i = 0; i < size; i += blockSize)
        updatePermutation(value[i], v);

    copyBytes(shuffledValue, value + v[0] * blockSize, blockSize);
    copyBytes(shuffledValue, value + v[1] * blockSize, blockSize, blockSize);
    copyBytes(shuffledValue, value + v[2] * blockSize, blockSize, 2 * blockSize);
    copyBytes(shuffledValue, value + v[3] * blockSize, blockSize, 3 * blockSize);
}

void xorBytes(byte* value1, byte* value2, byte* xoredValue, unsigned long long size)
{
    for (unsigned long long i = 0; i < size; i++)
    {
        xoredValue[i].val = value1[i].val ^ value2[i].val;
    }
}

int count(byte* bytes, unsigned long long size, unsigned long long blockSize)
{
    int sum = 0;
    for (unsigned long long i = 0; i < size; i += blockSize)
        sum += bytes[i].val;

    int value = (sum % 13) + 8;
    return value;
}

void clearBytes(bitstring str)
{
    for (unsigned long long i = 0; i < str.size; i++)
    {
        str.value[i].val = 0;
    }
}

void clearState(state& s)
{
    for (int i = 0; i < STATE_SIZE; i++)
        s.bytes[i].val = 0;

}

int mergeBytes(byte msb, byte lsb)
{
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

int sbox(byte input)
{
    byte output = {0};
    int x[4];
    x[0] = input.b4;
    x[1] = input.b5;
    x[2] = input.b6;
    x[3] = input.b7;
    output.b4 = x[0] ^ x[1] ^ x[3] ^ 1;
    output.b5 = x[0] ^ x[2] ^ x[3];
    output.b6 = x[1] ^ x[2] ^ x[3];
    output.b7 = x[0];

    return output.val;
}

void sbox(state& s)
{
    int blockSize = STATE_SIZE / 4;

    byte w0[STATE_SIZE / 4], w1[STATE_SIZE / 4], w2[STATE_SIZE / 4], w3[STATE_SIZE / 4];
    copyBytes(w0, s.bytes, blockSize);
    copyBytes(w1, s.bytes + blockSize, blockSize);
    copyBytes(w2, s.bytes + 2 * blockSize, blockSize);
    copyBytes(w3, s.bytes + 3 * blockSize, blockSize);

    for (int i = 0; i < blockSize; i++)
    {
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

void pbox(state& s)
{
    byte word[4], shuffled[4];
    for (int i = 0; i < STATE_SIZE; i++)
    {
        int pos = i + 1, cnt = 1;
        word[0].val = s.bytes[i].val;
        while (cnt < 4)
        {
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
        while (cnt < 4)
        {
            if (pos >= STATE_SIZE)
                pos = pos % STATE_SIZE;
            s.bytes[pos].val = shuffled[cnt].val;
            cnt++;
            pos++;
        }
    }
}

void f(state& s, int rounds)
{
    for (unsigned long long i = 1; i <= rounds; i++)
    {
        sbox(s);
        pbox(s);
    }
}

#pragma endregion

#pragma region Initialization

void initializeState(state& s, byte* key, byte* iv)
{
    byte niv[DEFAULT_SIZE], shuffledKey[DEFAULT_SIZE], xoredValue[DEFAULT_SIZE];
    invert(iv, niv, DEFAULT_SIZE);
    shuffleBytes(key, shuffledKey, DEFAULT_SIZE);
    xorBytes(key, iv, xoredValue, DEFAULT_SIZE);

    copyBytes(s.bytes, key, DEFAULT_SIZE);
    copyBytes(s.bytes, niv, DEFAULT_SIZE, DEFAULT_SIZE);
    copyBytes(s.bytes, shuffledKey, DEFAULT_SIZE, 2 * DEFAULT_SIZE);
    copyBytes(s.bytes, iv, DEFAULT_SIZE, 3 * DEFAULT_SIZE);
    copyBytes(s.bytes, xoredValue, DEFAULT_SIZE, 4 * DEFAULT_SIZE);

    f(s, count(s.bytes, STATE_SIZE, 2));
}

void fillByteArray(bitstring& str)
{

    str.value = (byte*)realloc(str.value, sizeof(byte) * (str.size + 1));
    str.value[str.size].val = defaultValue + 1;
    str.size++;
    while (str.size % BITRATE != 0)
    {
        str.value = (byte*)realloc(str.value, sizeof(byte) * (str.size + 1));
        str.value[str.size].val = 0;
        str.size++;
    }
}

void processAD(state& s, bitstring ad)
{
    for (unsigned long long i = 0; i < ad.size; i += BITRATE)
    {
        xorBytes(s.bytes, ad.value + i, s.bytes, BITRATE);
        f(s, count(ad.value + i, BITRATE, 1));
    }
}

#pragma endregion

#pragma region Crypt

byte* encrypt(state& s, byte key[], bitstring plaintext, bitstring ciphertext)
{
    byte* aux = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE * (plaintext.size / BITRATE));
    unsigned long long auxPos = 0;

    for (unsigned long long i = 0; i < plaintext.size; i += BITRATE)
    {
        xorBytes(s.bytes, plaintext.value + i, s.bytes, s.bitrate);
        copyBytes(ciphertext.value, s.bytes, s.bitrate, i);
        f(s, count(s.bytes + s.capacity, s.bitrate, 1));

        byte xoredValue[DEFAULT_SIZE];
        xorBytes(key, s.bytes + s.bitrate, xoredValue, DEFAULT_SIZE);

        copyBytes(aux, xoredValue, DEFAULT_SIZE, auxPos * DEFAULT_SIZE);
        auxPos++;
    }

    return aux;
}

byte* decrypt(state& s, byte key[], bitstring plaintext, bitstring ciphertext)
{
    byte* aux = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE * (plaintext.size / BITRATE));
    unsigned long long auxPos = 0;

    for (unsigned long long i = 0; i < plaintext.size; i += BITRATE)
    {
        xorBytes(s.bytes, ciphertext.value + i, s.bytes, s.bitrate);
        copyBytes(plaintext.value, s.bytes, s.bitrate, i);
        copyBytes(s.bytes, ciphertext.value + i, s.bitrate);
        f(s, count(s.bytes + s.capacity, s.bitrate, 1));

        byte xoredValue[DEFAULT_SIZE];
        xorBytes(key, s.bytes + s.bitrate, xoredValue, DEFAULT_SIZE);

        copyBytes(aux, xoredValue, DEFAULT_SIZE, auxPos * DEFAULT_SIZE);
        auxPos++;
    }

    return aux;
}

#pragma endregion

#pragma region Tag

byte* getTag(state& s, byte* aux, byte* ciphertext, unsigned long long plaintextSize)
{
    f(s, count(s.bytes, DEFAULT_SIZE, 1));
    byte* t, * shuffledAux;
    t = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE);
    unsigned long long auxSize = DEFAULT_SIZE * (plaintextSize / BITRATE);
    shuffledAux = (byte*)malloc(sizeof(byte) * auxSize);
    shuffleBytes(aux, shuffledAux, auxSize);
    xorBytes(s.bytes + s.bitrate, shuffledAux, t, DEFAULT_SIZE);

    free(shuffledAux);
    return t;
}

bool validTag(byte* receivedTag, byte* tag)
{
    for (int i = 0; i < DEFAULT_SIZE; i++)
        if (receivedTag[i].val != tag[i].val)
            return false;

    return true;
}
#pragma endregion

#pragma region Cryptoanalysis

double calculateEntropy(bitstring str)
{
    double entropy = 0;

    unsigned char* frequency = (unsigned char*)calloc(256, sizeof(unsigned char));

    for (int i = 0; i < str.size; i++)
        frequency[str.value[i].val]++;

    for (int i = 0; i < 256; i++)
        if (frequency[i] != 0)
        {
            double p = (double)frequency[i] / str.size;
            entropy = entropy + p * (log(p) / log(2));
        }

    free(frequency);

    return -entropy;
}

double histogramUniformity(bitstring str)
{
    unsigned char* frequency = (unsigned char*)calloc(256, sizeof(unsigned char));

    for (int i = 0; i < str.size; i++)
        frequency[str.value[i].val]++;

    double chi_square = 0;

    double estimated = str.size / 256.0;

    for (int i = 0; i < 256; i++)
        chi_square += pow(frequency[i] - estimated, 2) / estimated;

    return chi_square;
}

double uaci(bitstring plaintext, bitstring ciphertext)
{
    double UACI = 0;


    for (unsigned int i = 0; i < plaintext.size; i++)
        UACI += abs(plaintext.value[i].val - ciphertext.value[i].val);


    return (UACI / (plaintext.size * 255)) * 100;
}

double npcr(bitstring plaintext, bitstring ciphertext)
{
    double NPCR = 0;


    for (unsigned int i = 0; i < plaintext.size; i++)
        NPCR += (plaintext.value[i].val == ciphertext.value[i].val) ? 0 : 1;

    return (NPCR / plaintext.size) * 100;
}

double correlationCoefficient(bitstring plaintext, bitstring ciphertext)
{
    double e_in = 0;
    double d_in = 0;
    double e_out = 0;
    double d_out = 0;
    double cov = 0;

    for (unsigned int i = 0; i < plaintext.size; i++)
    {
        e_in += plaintext.value[i].val;
        e_out += +ciphertext.value[i].val;
    }

    e_in /= plaintext.size;
    e_out /= plaintext.size;


    for (unsigned int i = 0; i < plaintext.size; i++)
    {
        d_in += pow(plaintext.value[i].val - e_in, 2);
        d_out += pow(ciphertext.value[i].val - e_out, 2);
        cov += (plaintext.value[i].val - e_in) * (ciphertext.value[i].val - e_out);
    }

    d_in /= plaintext.size;
    d_out /= plaintext.size;
    cov /= plaintext.size;

    return cov / (sqrt(d_in * d_out));
}


#pragma endregion


int main()
{
    bool shouldDecrypt = false;
    byte* key, * iv;
    srand((unsigned)time(0));
    key = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE);
    iv = (byte*)malloc(sizeof(byte) * DEFAULT_SIZE);
    state s;
    s.bitrate = BITRATE;
    s.capacity = STATE_SIZE - BITRATE;
    double entropyMin = 7.9;
    double ktestMin = 0.93;
    double ktestMax = 0.97;
    int kPassedTests = 0;
    ofstream kout("kstestResults.txt");


    for (int count = 1; count <= 500; count++)
    {
        int passedTests = 0;
        char filename[105];
        snprintf(filename, 105, "results%d.txt", count);
        ofstream out(filename);
        out << "Key\tIv\tPlaintext size\tAssociated data size\tPlaintext entropy";
        out << "\tCiphertext entropy\tPlaintext histogram";
        out << "\tCiphertext histogram\tUACI\tNPCR\tCorrelation coefficient\n";

        cout << "STARTING FOR KEY " << count << " AT ";
        std::time_t t = std::time(0);   // get time now
        std::tm now;
        localtime_s(&now, &t);
        cout << (now.tm_year + 1900) << '-' << (now.tm_mon + 1) << '-' << now.tm_mday << " " << now.tm_hour << ":" << now.tm_min << endl;
        for (int i = 0; i < DEFAULT_SIZE; i++)
        {
            iv[i].val = rand();
            key[i].val = rand();
        }

        for (int i = 0; i < DEFAULT_SIZE; i++)
            out << HEX(key[i].val);
        out << "\t";
        for (int i = 0; i < DEFAULT_SIZE; i++)
            out << HEX(iv[i].val);
        out << "\n";

        out << std::dec;
        for (int plaintextCount = 1; plaintextCount <= 500; plaintextCount++)
        {
            bitstring ad, plaintext;

            unsigned long long adSize = rand() % 5000000 + 15;
            unsigned long long plaintextSize = rand() % 5000000 + 15;

            ad.size = adSize;
            ad.value = (byte*)calloc(ad.size, sizeof(byte));
            plaintext.size = plaintextSize;
            plaintext.value = (byte*)calloc(plaintext.size, sizeof(byte));

            for (int i = 0; i < ad.size; i++)
                ad.value[i].val = rand();

            for (int i = 0; i < plaintext.size; i++)
                plaintext.value[i].val = rand();

            out << "\t\t" << plaintext.size << "\t" << ad.size << "\t";

            bitstring ciphertext;

            clearState(s);
            initializeState(s, key, iv);
            fillByteArray(ad);
            fillByteArray(plaintext);
            ciphertext.size = plaintext.size + DEFAULT_SIZE;
            ciphertext.value = (byte*)calloc(ciphertext.size, sizeof(byte));

            processAD(s, ad);
            byte* aux = encrypt(s, key, plaintext, ciphertext);

            byte* t = getTag(s, aux, ciphertext.value, plaintext.size);
            copyBytes(ciphertext.value, t, DEFAULT_SIZE, plaintext.size);

            double ciphertextEntropy = calculateEntropy(ciphertext);
            if (ciphertextEntropy > entropyMin)
                passedTests++;

            out << calculateEntropy(plaintext) << "\t" << ciphertextEntropy  << "\t";
            out << histogramUniformity(plaintext) << "\t" << histogramUniformity(ciphertext) << "\t";
            out << uaci(plaintext, ciphertext) << "\t" << npcr(plaintext, ciphertext) << "\t";
            out << correlationCoefficient(plaintext, ciphertext) << endl;


            delete(ad.value);
            delete(plaintext.value);
            delete(ciphertext.value);
        }

        double ktestValue = ((double)passedTests) / 500;
        kout << count << "\t" << ktestValue << endl;
        if (ktestMin <= ktestValue && ktestValue <= ktestMax)
            kPassedTests++;

    }

    kout << "Final Ktest: " << ((double)kPassedTests) / 500;

    delete(key);
    delete(iv);

    return 0;
}