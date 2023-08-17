/**

	Бенчмарк-тест.
	Тестирует систему по трём показателям:
	- производительность вычислений (сложение целых чисел)
	- производительность графической подсистемы (рендеринг 2D-графики при помощи Direct2D)
	- производительность дисковой подсистемы (последовательные и случайные запись и чтение данных с диска)

	Результатом по каждому пункту является количество очков, прямо зависящее от времени исполнения фиксированного количества действий (с точностью до микросекунд).
	Т.е. чем меньше очков, тем лучше.

**/

#pragma once
#include <Windows.h>
#include <iostream>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <string>
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

template<class Interface>
inline void SafeRelease(Interface** ppInterfaceToRelease) {
	if (*ppInterfaceToRelease != NULL) {
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif



#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define M_PI           3.14159265358979323846