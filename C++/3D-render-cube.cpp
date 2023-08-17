/**

	Пробовал работать с Direct3D. Данный код рендерит радужный куб с определённого ракурса.
	Здесь далеко не полный код, я к нему ещё HLSL-шейдеры писал.

**/

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <string>

#pragma comment(lib, "d3d11")

#include "basewin.h"

template <class T> void SafeRelease(T** ppT) {
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

struct float3 {
	float x;
	float y;
	float z;
	float3(float a, float b, float c) : x(a), y(b), z(c) {}
};

struct vertex {
	float3 pos;
	float3 color;
	vertex(float3 pos_, float3 color_) : pos(pos_), color(color_) {}
};

class MainWindow : public BaseWindow<MainWindow> {
	ID3D11Device* p3dDevice;
	ID3D11DeviceContext* p3dDeviceContext;
	IDXGISwapChain1* p3dSwapChain;
	ID3D11RenderTargetView* pRenderTarget;

	BYTE* LoadShader(LPCWSTR filename, SIZE_T &size);

public:
	MainWindow() : p3dDevice(nullptr), p3dDeviceContext(nullptr), p3dSwapChain(nullptr) {}
	PCWSTR ClassName() const { return L"gametest"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HRESULT Init();
	HRESULT LoadCube();
};

BYTE* MainWindow::LoadShader(LPCWSTR filename, SIZE_T &size) {
	HANDLE file = CreateFile((std::wstring(L"C:\\Users\\user\\source\\repos\\test1\\Debug\\") + std::wstring(filename)).c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) return nullptr;
	DWORD fileSizeHigh;
	DWORD fileSize = GetFileSize(file, &fileSizeHigh);
	if (fileSize == INVALID_FILE_SIZE) return nullptr;
	size = fileSize;
	BYTE* buffer = new BYTE[fileSize];
	if (!ReadFile(file, buffer, fileSize, &fileSizeHigh, NULL)) return nullptr;
	CloseHandle(file);
	return buffer;
}

HRESULT MainWindow::LoadCube() {
	vertex vertexData[] = {
		{float3(0,0,0), float3(0,0,0)},{float3(0,1,0), float3(1,1,1)},
		{float3(1,1,0), float3(1,0,0)},{float3(1,0,0), float3(0,0,1)},
		{float3(0,0,1), float3(1,1,0)},{float3(0,1,1), float3(0,1,1)},
		{float3(1,1,1), float3(1,0,1)},{float3(1,0,1), float3(0.5f,1,0.5f)}
	};
	unsigned short indexData[] = {
		1, 0, 2,
		2, 0, 3,

		0, 1, 4,
		1, 5, 4,

		1, 2, 5,
		2, 6, 5,

		2, 3, 6,
		3, 7, 6,

		3, 4, 7,
		3, 0, 4,

		4, 5, 6,
		6, 7, 4
	};
	const D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"MODEL_SPACE_POSITION",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D11_APPEND_ALIGNED_ELEMENT,
		D3D11_INPUT_PER_VERTEX_DATA,
		0},
	{"COLOR",
	0,
	DXGI_FORMAT_R32G32B32_FLOAT,
	0,
	D3D11_APPEND_ALIGNED_ELEMENT,
	D3D11_INPUT_PER_VERTEX_DATA,
	0}
	};
	ID3D11InputLayout* pInputLayout;
	SIZE_T vertexShaderSize = 0, pixelShaderSize = 0;
	BYTE* vertexShaderBytecode = LoadShader(L"VertexShader.cso", vertexShaderSize),* pixelShaderBytecode = LoadShader(L"PixelShader.cso", pixelShaderSize);
	p3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBytecode , vertexShaderSize, &pInputLayout);
	p3dDeviceContext->IASetInputLayout(pInputLayout);
	ID3D11Buffer* vertexBuffer, *indexBuffer;
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.ByteWidth = sizeof(vertex) * ARRAYSIZE(vertexData);
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA vertexBufferSubresource;
	vertexBufferSubresource.pSysMem = vertexData;
	vertexBufferSubresource.SysMemPitch = 0;
	vertexBufferSubresource.SysMemSlicePitch = 0;
	p3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubresource, &vertexBuffer);
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.ByteWidth = sizeof(unsigned short) * ARRAYSIZE(indexData);
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA indexBufferSubresource;
	indexBufferSubresource.pSysMem = indexData;
	indexBufferSubresource.SysMemPitch = 0;
	indexBufferSubresource.SysMemSlicePitch = 0;
	p3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferSubresource, &indexBuffer);
	UINT stride = sizeof(vertex);
	UINT offset = 0;
	p3dDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	p3dDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	p3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11VertexShader* pVS;
	p3dDevice->CreateVertexShader(vertexShaderBytecode, vertexShaderSize, NULL, &pVS);
	ID3D11PixelShader* pPS;
	p3dDevice->CreatePixelShader(pixelShaderBytecode, pixelShaderSize, NULL, &pPS);
	p3dDeviceContext->VSSetShader(pVS, nullptr, 0);
	p3dDeviceContext->PSSetShader(pPS, nullptr, 0);
	p3dDeviceContext->OMSetRenderTargets(1, &pRenderTarget, nullptr);
	const float clear[] = { 0, 1.0f, 0, 1.0f };
	p3dDeviceContext->ClearRenderTargetView(pRenderTarget, clear);
	p3dDeviceContext->DrawIndexed(ARRAYSIZE(indexData), 0, 0);
	p3dSwapChain->Present(1, 0);
	SafeRelease(&pInputLayout);
	SafeRelease(&indexBuffer);
	SafeRelease(&vertexBuffer);
	SafeRelease(&pVS);
	SafeRelease(&pPS);
	delete[] vertexShaderBytecode, pixelShaderBytecode;
	return S_OK;
}

HRESULT MainWindow::Init() {
	HRESULT hr = S_OK;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_1
	};
	hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &p3dDevice, NULL, &p3dDeviceContext);
	if (FAILED(hr)) return hr;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Stereo = false;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.Flags = 0;
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	IDXGIDevice2* pDXGIDevice;
	hr = p3dDevice->QueryInterface(__uuidof(IDXGIDevice2), (void**)&pDXGIDevice);
	if (FAILED(hr)) return hr;
	IDXGIAdapter* pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&pDXGIAdapter);
	if (FAILED(hr)) return hr;
	IDXGIFactory2* pDXGIFactory;
	hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pDXGIFactory);
	if (FAILED(hr)) return hr;
	hr = pDXGIFactory->CreateSwapChainForHwnd(p3dDevice, m_hwnd, &swapChainDesc, NULL, NULL, &p3dSwapChain);
	if (FAILED(hr)) return hr;
	SafeRelease(&pDXGIFactory);
	SafeRelease(&pDXGIAdapter);
	SafeRelease(&pDXGIDevice);
	ID3D11Texture2D* backBuffer;
	hr = p3dSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (FAILED(hr)) return hr;
	hr = p3dDevice->CreateRenderTargetView(backBuffer, nullptr, &pRenderTarget);
	if (FAILED(hr)) return hr;
	D3D11_TEXTURE2D_DESC backBufferDesc;
	backBuffer->GetDesc(&backBufferDesc);
	SafeRelease(&backBuffer);
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(backBufferDesc.Width);
	viewport.Height = static_cast<float>(backBufferDesc.Height);
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	p3dDeviceContext->RSSetViewports(1, &viewport);
	return hr;
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
			//win.LoadCube();
		}
	}

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		if (FAILED(Init())) {
			return -1;
		}
		return 0;
	case WM_DESTROY:
		SafeRelease(&p3dDevice);
		SafeRelease(&p3dDeviceContext);
		SafeRelease(&p3dSwapChain);
		SafeRelease(&pRenderTarget);
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		LoadCube();
		return 0;
	case WM_SIZE:
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}