//https://learn.microsoft.com/zh-cn/microsoft-edge/webview2/samples/webview2apissample

#include <Windows.h>
#include <wrl.h>

#include <string>

#include "WebView2.h"
#include "wil/com.h"

using namespace Microsoft::WRL;

// The main window class name.
static wchar_t szWindowClass[] = L"DesktopApp";

// The string that appears in the application's title bar.
static wchar_t szTitle[] = L"WebView sample";

HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webview;

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
	)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex)) {
		MessageBox(NULL, L"Call to RegisterClassEx failed!",
			L"Windows Desktop Guided",
			NULL);
		return 1;
	}

	//存储到全局变量
	hInst = hInstance;

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,
		1200,900,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	//begin:webview2
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[hWnd](HRESULT result, ICoreWebView2Environment* env)->HRESULT {
				env->CreateCoreWebView2Controller(hWnd,
					Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
						[hWnd](HRESULT result, ICoreWebView2Controller* controller)->HRESULT {
							if (controller != nullptr) {
								webviewController = controller;
								webviewController->get_CoreWebView2(&webview);
							}

							// Add a few settings for the webview
							// The demo step is redundant since the values are the default settings
							wil::com_ptr<ICoreWebView2Settings> settings;
							webview->get_Settings(&settings);
							settings->put_IsScriptEnabled(TRUE);
							settings->put_AreDefaultScriptDialogsEnabled(TRUE);
							settings->put_IsWebMessageEnabled(TRUE);

							// Resize WebView to fit the bounds of the parent window
							RECT bounds;
							GetClientRect(hWnd, &bounds);
							webviewController->put_Bounds(bounds);

							// Schedule an async task to navigate to Google
							webview->Navigate(L"https://www.google.com/");

							//导航事件
							EventRegistrationToken token;
							webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
								[](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args)->HRESULT {
									wil::unique_cotaskmem_string uri;
									args->get_Uri(&uri);
									std::wstring source(uri.get());
									if (source.substr(0, 5) != L"https") {
										args->put_Cancel(true);
									}
									return S_OK;
								}).Get(), &token);

							//脚本
							webview->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
							webview->ExecuteScript(L"window.document.URL", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
								[](HRESULT errorCode, LPCWSTR resultObjectAsJson)->HRESULT {
									LPCWSTR URL = resultObjectAsJson;
									//doSomethingWithURL(URL);
									return S_OK;
								}).Get());

							//主机通信
							//Set an event handler for the host to return received message back to the web content
							webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
								[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) ->HRESULT {
									wil::unique_cotaskmem_string message;
									args->TryGetWebMessageAsString(&message);
									// processMessage(&message);
									webview->PostWebMessageAsString(message.get());
									return S_OK;
								}).Get(), &token);

							// Schedule an async task to add initialization script that
							// 1) Add an listener to print message from the host
							// 2) Post document URL to the host
							webview->AddScriptToExecuteOnDocumentCreated(
								L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
								L"window.chrome.webview.postMessage(window.document.URL);",
								nullptr);

							return S_OK;
						}).Get());
				return S_OK;
			}).Get());

	//end:webview2

	if (!hWnd) 
	{
		MessageBox(NULL, L"Call to CreateWindow failed!",
			L"Windows Desktop Guided",
			NULL);
		return 1;
	}

	ShowWindow(hWnd,nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CoUninitialize();
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
	{
		if(webviewController != nullptr) {
			RECT bounds;
			GetClientRect(hWnd,&bounds);
			webviewController->put_Bounds(bounds);
		}
	}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}
