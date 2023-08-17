/**

	Бенчмарк-тест.
	Тестирует систему по трём показателям:
	- производительность вычислений (сложение целых чисел)
	- производительность графической подсистемы (рендеринг 2D-графики при помощи Direct2D)
	- производительность дисковой подсистемы (последовательные и случайные запись и чтение данных с диска)

	Результатом по каждому пункту является количество очков, прямо зависящее от времени исполнения фиксированного количества действий (с точностью до микросекунд).
	Т.е. чем меньше очков, тем лучше.

**/

#include "benchmark.h"

int StringToWString(std::wstring& ws, const std::string& s)
{
	std::wstring wsTmp(s.begin(), s.end());
	ws = wsTmp;
	return 0;
}

class DemoApp {
public:
	DemoApp();
	~DemoApp();

	HRESULT Initialize();
	void RunMessageLoop();
private:
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();
	HRESULT OnRender();
	void OnResize(UINT width, UINT height);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HWND m_hwnd;
	ID2D1Factory* m_pDirect2dFactory;
	IDWriteFactory* m_pDWriteFactory;
	IDWriteTextFormat* m_pDwriteTextFormat;
	ID2D1HwndRenderTarget* m_pRenderTarget;
	ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
	ID2D1SolidColorBrush* m_pCornflowerBlueBrush;
	ID2D1SolidColorBrush* pBlackBrush;
	std::chrono::steady_clock::time_point lastFrame;
	std::chrono::steady_clock::time_point start;
	float frames = 0;
	int total = 0;
};

DemoApp::DemoApp() :
	m_hwnd(NULL),
	m_pDirect2dFactory(NULL),
	m_pDWriteFactory(NULL),
	m_pDwriteTextFormat(NULL),
	m_pRenderTarget(NULL),
	m_pLightSlateGrayBrush(NULL),
	m_pCornflowerBlueBrush(NULL),
	pBlackBrush(NULL),
	start(std::chrono::high_resolution_clock::now())
{
	srand(12345);
}

DemoApp::~DemoApp()
{
	SafeRelease(&m_pDirect2dFactory);
	SafeRelease(&m_pDWriteFactory);
	SafeRelease(&m_pDwriteTextFormat);
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pLightSlateGrayBrush);
	SafeRelease(&m_pCornflowerBlueBrush);
	SafeRelease(&pBlackBrush);
}

void DemoApp::RunMessageLoop() {
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

HRESULT DemoApp::Initialize() {
	HRESULT hr;
	hr = CreateDeviceIndependentResources();
	if (SUCCEEDED(hr)) {
		WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = DemoApp::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(LONG_PTR);
		wcex.hInstance = HINST_THISCOMPONENT;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
		wcex.lpszClassName = L"D2D1DemoApp";

		RegisterClassEx(&wcex);

		FLOAT dpiX = 96.f, dpiY = 96.f;
		dpiX = (FLOAT)GetDpiForWindow(GetDesktopWindow());
		dpiY = dpiX;
		
		SetProcessDPIAware();

		m_hwnd = CreateWindow(L"D2D1DemoApp", L"Direct 2D Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			static_cast<UINT>(ceil(640.f * dpiX / 96.f)), static_cast<UINT>(ceil(480.f * dpiY / 96.f)), NULL, NULL, HINST_THISCOMPONENT, this);
		hr = m_hwnd ? S_OK : E_FAIL;
		if (SUCCEEDED(hr)) {
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
			UpdateWindow(m_hwnd);
		}
	}
	return hr;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	if (SUCCEEDED(CoInitialize(NULL))) {
		{
			DemoApp app;
			if (SUCCEEDED(app.Initialize())) {
				app.RunMessageLoop();
			}
		}
		CoUninitialize();
	}
	return 0;
}

HRESULT DemoApp::CreateDeviceIndependentResources() {
	HRESULT hr = S_OK;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
	if (SUCCEEDED(hr)) {
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (SUCCEEDED(hr)) {
			hr = m_pDWriteFactory->CreateTextFormat(L"Consolas", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 26.f, L"ru-ru", &m_pDwriteTextFormat);
		}
		if (SUCCEEDED(hr)) {
			m_pDwriteTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			m_pDwriteTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		}
	}
	return hr;
}

HRESULT DemoApp::CreateDeviceResources() {
	HRESULT hr = S_OK;
	if (!m_pRenderTarget) {
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
		hr = m_pDirect2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hwnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY), &m_pRenderTarget);
		if (SUCCEEDED(hr)) {
			hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &m_pLightSlateGrayBrush);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &m_pCornflowerBlueBrush);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);
		}
	}
	return hr;
}

void DemoApp::DiscardDeviceResources() {
	SafeRelease(&m_pRenderTarget);
	SafeRelease(&m_pCornflowerBlueBrush);
	SafeRelease(&m_pLightSlateGrayBrush);
}

LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;
	if (message == WM_CREATE) {
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		DemoApp* pDemoApp = (DemoApp*)pcs->lpCreateParams;
		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pDemoApp));
		result = 1;
	}
	else {
		DemoApp* pDemoApp = reinterpret_cast<DemoApp*>(static_cast<LONG_PTR>(::GetWindowLongW(hwnd, GWLP_USERDATA)));
		bool wasHandled = false;
		if (pDemoApp) {
			switch (message) {
			case WM_SIZE:
				{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				pDemoApp->OnResize(width, height);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_DISPLAYCHANGE:
				{
				InvalidateRect(hwnd, NULL, FALSE);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_PAINT:
				{
				pDemoApp->OnRender();
				ValidateRect(hwnd, NULL);
				if (pDemoApp->total < 15000)
					InvalidateRect(hwnd, NULL, FALSE);
				else
					DestroyWindow(hwnd);
				}
				result = 0;
				wasHandled = true;
				break;
			case WM_DESTROY:
				{
				PostQuitMessage(0);
				}
				result = 1;
				wasHandled = true;
				break;
			}
		}
		if (!wasHandled) {
			result = DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
	return result;
}

HRESULT DemoApp::OnRender() {
	HRESULT hr = S_OK;
	hr = CreateDeviceResources();
	if (SUCCEEDED(hr)) {
		m_pRenderTarget->BeginDraw();
		float now = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() % 1000000) / 1000000.f;
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
		D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();

		int width = static_cast<int>(rtSize.width);
		int height = static_cast<int>(rtSize.height);

		for (int i = 0; i < width; i += 50) {
			for (int j = 0; j < height; j += 50) {
				pBlackBrush->SetColor(D2D1::ColorF((rand() % 256) / 255.f, (rand() % 256) / 255.f, (rand() % 256) / 255.f, 0.2f));
				m_pRenderTarget->FillRectangle(D2D1::RectF(i, j, i + 50, j + 50), pBlackBrush);
			}
		}

		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(180.f * now, D2D1::Point2F(width/2, height/2)));

		D2D1_RECT_F rectangle1 = D2D1::RectF(
			rtSize.width / 2 - 50.0f,
			rtSize.height / 2 - 50.0f,
			rtSize.width / 2 + 50.0f,
			rtSize.height / 2 + 50.0f
		);

		D2D1_RECT_F rectangle2 = D2D1::RectF(
			rtSize.width / 2 - 100.0f,
			rtSize.height / 2 - 100.0f,
			rtSize.width / 2 + 100.0f,
			rtSize.height / 2 + 100.0f
		);
		m_pRenderTarget->FillRectangle(rectangle1, m_pLightSlateGrayBrush);
		m_pRenderTarget->DrawRectangle(rectangle2, m_pCornflowerBlueBrush);
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		pBlackBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
		frames = frames * (999.f / 1000.f) + (1.f / 1000.f) * (1000000.f / std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - lastFrame).count());
		std::string str = "fps: " + std::to_string((unsigned int)floor(frames));
		std::wstring text = std::wstring(str.begin(), str.end());
		D2D1_RECT_F rect = D2D1::RectF(0.f, 0.f, rtSize.width, 100.f);
		m_pRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pDwriteTextFormat, rect, pBlackBrush);
		hr = m_pRenderTarget->EndDraw();
		lastFrame = std::chrono::high_resolution_clock::now();
		total += 1;
	}
	if (hr == D2DERR_RECREATE_TARGET) {
		hr = S_OK;
		DiscardDeviceResources();
	}
	return hr;
}

void DemoApp::OnResize(UINT width, UINT height) {
	if (m_pRenderTarget) {
		m_pRenderTarget->Resize(D2D1::SizeU(width, height));
	}
}

uint64_t calcTest() {
#define TEST_ITER 25 * 1024 * 1024
	std::mt19937_64 gen(12345);
	std::uniform_int_distribution<uint64_t> dis;
	uint64_t* a = new uint64_t[TEST_ITER];
	for (size_t i = 0; i < TEST_ITER; i++) {
		a[i] = dis(gen);
	}
	uint64_t res = 0;
	for (int j = 0; j < 32; j++) {
		std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < TEST_ITER - 1; i++) {
			a[TEST_ITER - 1 - i] = a[i] + a[i + 1];
		}
		res += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
	}
	delete[] a;
	return res;
}

uint64_t memTest() {
#define TEST_FILES 50
	srand(12345);
	size_t len = 25 * 1024 * 1024;
	char* buf = new char[len];
	for (size_t i = 0; i < len; i++) {
		buf[i] = (char)(rand() % 256);
	}
	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_FILES; i++) {
		std::ofstream out("storage_test" + std::to_string(i) + ".data", std::ios::binary | std::ios::out);
		out.write(buf, len);
		out.close();
	}
	for (int i = 0; i < TEST_FILES; i++) {
		std::ifstream in("storage_test" + std::to_string(i) + ".data", std::ios::binary | std::ios::in);
		in.read(buf, len);
		in.close();
	}
	uint64_t res = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
	std::string fname = "";
	for (int i = 0; i < TEST_FILES; i++) {
		fname = "storage_test" + std::to_string(i) + ".data";
		std::remove(fname.c_str());
	}
	delete[] buf;
	return res;
}

uint64_t graphicsTest() {
	std::chrono::steady_clock::time_point start = std::chrono::high_resolution_clock::now();
	WinMain(NULL, NULL, NULL, 0);
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
}

int main() {
	std::cout << "Starting calculation test..." << std::endl;
	uint64_t ct = ceil(calcTest() / 1000 * 3.5);
	std::cout << "CALCS: " << ct << std::endl;
	std::cout << "Starting graphics test..." << std::endl;
	uint64_t gt = graphicsTest() / 1000;
	std::cout << "GRAPHICS: " << gt << std::endl;
	std::cout << "Starting storage test..." << std::endl;
	uint64_t st = ceil(memTest() / 1000 * 1.8);
	std::cout << "STORAGE: " << st << std::endl;
	std::cout << std::endl << "Scores table:" << std::endl;
	std::cout << std::setw(8) << "Model\t" << std::setw(15) << "CALCS test\t" << std::setw(15) << "GRAPHICS test\t" << std::setw(15) << "STORAGE test" << std::setw(15) << "RESULT" << std::endl;
	std::vector<std::vector<int>> res = { {4879, 5018, 4828}, {7410, 6982, 4675},  {13632,14221,31556}, {0,0,0} };
	res[res.size() - 1][0] = ct;
	res[res.size() - 1][1] = gt;
	res[res.size() - 1][2] = st;
	for (int i = 0; i < res.size(); i++) {
		if (i != res.size()-1)
			std::cout << std::setw(8) << "PC" + std::to_string(i+1) + "\t";
		else
			std::cout << std::setw(8) << "Your PC\t";
		for (auto now : res[i])
			std::cout << std::setw(15) << std::to_string(now) + "\t";
		std::cout << std::setw(15) << ceil(pow(res[i][0]/1000*res[i][1]/1000*res[i][2]/1000, 1./3)*1000);
		std::cout << std::endl;
	}
	std::cout << std::endl << "Normalized scores table:" << std::endl;
	std::cout << std::setw(8) << "Model\t" << std::setw(15) << "CALCS test\t" << std::setw(15) << "GRAPHICS test\t" << std::setw(15) << "STORAGE test" << std::setw(15) << "RESULT" << std::endl;
	for (int i = 0; i < res.size(); i++) {
		if (i != res.size() - 1)
			std::cout << std::setw(8) << "PC" + std::to_string(i + 1) + "\t";
		else
			std::cout << std::setw(8) << "Your PC\t";
		for (int j = 0; j < res[i].size(); j++)
			std::cout << std::setw(15) << std::setprecision(3) << (float)res[i][j]/res[2][j]  << "\t";
		std::cout << std::setw(15) << pow((float)res[i][0] * res[i][1] * res[i][2] / res[2][0] / res[2][1] / res[2][2], 1. / 3);
		std::cout << std::endl;
	}
	std::system("pause");
	return 0;
}