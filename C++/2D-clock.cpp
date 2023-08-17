/**

	Часы.
	Win32 приложение, выводит на экран изображение классических часов со стрелками и числовыми подписями, отображающими текущее время.
	Использует Direct2D для рендеринга 2D-графики.
	Также отображает счётчик FPS.

**/

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <chrono>
#pragma comment(lib, "d2d1")
#pragma comment(lib, "DWrite")

#include "basewin.h"

template <class T> void SafeRelease(T** ppT) {
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

class MainWindow : public BaseWindow<MainWindow> {
	ID2D1Factory* pFactory;
	ID2D1HwndRenderTarget* pRenderTarget;
	ID2D1SolidColorBrush* pBrush;
	IDWriteFactory* pDWriteFactory;
	IDWriteTextFormat* pDWriteTextFormat;
	D2D1_ELLIPSE ellipse;
	float hAngle, mAngle, sAngle;
	UINT32 upsFinal, upsCounter, fpsFinal, fpsCounter;
	SYSTEMTIME time;
	std::chrono::time_point<std::chrono::steady_clock> timer, lframe;

	void CalculateLayout();
	HRESULT CreateGraphicResourses();
	void DiscardGraphicResources();
	void OnPaint();
	void Resize();

public:
	MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL) {}
	PCWSTR ClassName() const { return L"Circle Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Update();
};

void MainWindow::CalculateLayout() {
	if (pRenderTarget != NULL) {
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		const float x = size.width / 2;
		const float y = size.height / 2;
		const float radius = min(x, y);
		ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
	}
}

void MainWindow::Update() {
	upsCounter++;
	if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - timer)).count() > 0) {
		GetLocalTime(&time);
		hAngle = (time.wHour % 12) * 30 + time.wMinute * 0.5f;
		mAngle = (time.wMinute % 60) * 6 + time.wSecond * 0.1f;
		sAngle = (time.wSecond % 60) * 6;
		timer = std::chrono::high_resolution_clock::now();
		upsFinal = upsCounter;
		upsCounter = 0;
		fpsFinal = fpsCounter;
		fpsCounter = 0;
	}
	InvalidateRect(m_hwnd, NULL, FALSE);
}

HRESULT MainWindow::CreateGraphicResourses() {
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL) {
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		if (SUCCEEDED(hr)) {
			hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 0), &pBrush);
		}
		if (SUCCEEDED(hr)) {
			hr = pDWriteFactory->CreateTextFormat(L"Consolas", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL, 15.0f, L"ru", &pDWriteTextFormat);
		}
		if (SUCCEEDED(hr)) {
			CalculateLayout();
		}
	}
	return hr;
}

void MainWindow::DiscardGraphicResources() {
	SafeRelease(&pBrush);
	SafeRelease(&pRenderTarget);
	SafeRelease(&pDWriteTextFormat);
}

void MainWindow::OnPaint() {
	HRESULT hr = CreateGraphicResourses();
	if (SUCCEEDED(hr)) {
		fpsCounter++;

		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		pRenderTarget->BeginDraw();

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		pRenderTarget->FillEllipse(ellipse, pBrush);
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(hAngle, ellipse.point));
		pBrush->SetColor(D2D1::ColorF(0, 0, 0));
		pRenderTarget->DrawLine(ellipse.point, D2D1::Point2F(ellipse.point.x, ellipse.point.y - ellipse.radiusY * 0.9f), pBrush, 3.0f);
		pRenderTarget->DrawText(std::to_wstring(time.wHour).c_str(), std::to_wstring(time.wHour).size(), pDWriteTextFormat, D2D1::RectF(ellipse.point.x - 10, ellipse.point.y - ellipse.radiusY, ellipse.point.x + 15, ellipse.point.y - ellipse.radiusY * 0.9), pBrush);
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(mAngle, ellipse.point));
		pRenderTarget->DrawLine(ellipse.point, D2D1::Point2F(ellipse.point.x, ellipse.point.y - ellipse.radiusY * 0.9f), pBrush, 2.0f);
		pRenderTarget->DrawText(std::to_wstring(time.wMinute).c_str(), std::to_wstring(time.wMinute).size(), pDWriteTextFormat, D2D1::RectF(ellipse.point.x - 10, ellipse.point.y - ellipse.radiusY, ellipse.point.x + 15, ellipse.point.y - ellipse.radiusY * 0.9), pBrush);
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(sAngle, ellipse.point));
		pRenderTarget->DrawLine(ellipse.point, D2D1::Point2F(ellipse.point.x, ellipse.point.y - ellipse.radiusY * 0.9f), pBrush);
		pRenderTarget->DrawText(std::to_wstring(time.wSecond).c_str(), std::to_wstring(time.wSecond).size(), pDWriteTextFormat, D2D1::RectF(ellipse.point.x - 10, ellipse.point.y - ellipse.radiusY, ellipse.point.x + 15, ellipse.point.y - ellipse.radiusY * 0.9), pBrush);
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		std::wstring str = L"UPS: " + std::to_wstring(upsFinal) + L" FPS: " + std::to_wstring(fpsFinal) + L" FPS_calc: " + std::to_wstring(
		1000000.0 / std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-lframe).count()
		);
		lframe = std::chrono::high_resolution_clock::now();
		pRenderTarget->DrawText(str.c_str(), str.size(), pDWriteTextFormat, D2D1::RectF(0,0,ellipse.radiusX * 2, ellipse.radiusY), pBrush);
		pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 0));

		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
			DiscardGraphicResources();
		}
		EndPaint(m_hwnd, &ps);
	}
}

void MainWindow::Resize() {
	if (pRenderTarget != NULL) {
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		pRenderTarget->Resize(D2D1::SizeU(rc.right, rc.bottom));
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
	MainWindow win;

	if (!win.Create(L"Circle", WS_OVERLAPPEDWINDOW)) {
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);

	MSG msg;
	msg.message = WM_NULL;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			win.Update();
		}
	}

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))
			|| FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(pDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory)))) {
			return -1;
		}
		Update();
		return 0;
	case WM_DESTROY:
		DiscardGraphicResources();
		SafeRelease(&pDWriteFactory);
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		OnPaint();
		return 0;
	case WM_SIZE:
		Resize();
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}