#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>

TCHAR szClassName[] = TEXT("Window");
DWORD g_uSide;
DWORD g_fAppRegistered;

BOOL RegisterAccessBar(HWND hwndAccessBar, BOOL fRegister)
{
	APPBARDATA abd;
	abd.cbSize = sizeof(APPBARDATA);
	abd.hWnd = hwndAccessBar;
	if (fRegister)
	{
		abd.uCallbackMessage = WM_APP;
		if (!SHAppBarMessage(ABM_NEW, &abd))
			return FALSE;
		g_uSide = ABE_RIGHT;
		g_fAppRegistered = TRUE;
	}
	else
	{
		SHAppBarMessage(ABM_REMOVE, &abd);
		g_fAppRegistered = FALSE;
	}
	return TRUE;
}

void PASCAL AppBarQuerySetPos(UINT uEdge, LPRECT lprc, PAPPBARDATA pabd)
{
	int iHeight = 0;
	int iWidth = 0;
	pabd->rc = *lprc;
	pabd->uEdge = uEdge;
	if ((uEdge == ABE_LEFT) || (uEdge == ABE_RIGHT))
	{
		iWidth = pabd->rc.right - pabd->rc.left;
		pabd->rc.top = 0;
		pabd->rc.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		iHeight = pabd->rc.bottom - pabd->rc.top;
		pabd->rc.left = 0;
		pabd->rc.right = GetSystemMetrics(SM_CXSCREEN);
	}

	SHAppBarMessage(ABM_QUERYPOS, pabd);

	switch (uEdge)
	{
	case ABE_LEFT:
		pabd->rc.right = pabd->rc.left + iWidth;
		break;
	case ABE_RIGHT:
		pabd->rc.left = pabd->rc.right - iWidth;
		break;
	case ABE_TOP:
		pabd->rc.bottom = pabd->rc.top + iHeight;
		break;
	case ABE_BOTTOM:
		pabd->rc.top = pabd->rc.bottom - iHeight;
		break;
	}
	SHAppBarMessage(ABM_SETPOS, pabd);
	MoveWindow(pabd->hWnd, pabd->rc.left, pabd->rc.top,
		pabd->rc.right - pabd->rc.left,
		pabd->rc.bottom - pabd->rc.top, TRUE);
}

void PASCAL AppBarPosChanged(PAPPBARDATA pabd)
{
	RECT rc;
	RECT rcWindow;
	int iHeight;
	int iWidth;
	rc.top = 0;
	rc.left = 0;
	rc.right = GetSystemMetrics(SM_CXSCREEN);
	rc.bottom = GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(pabd->hWnd, &rcWindow);
	iHeight = rcWindow.bottom - rcWindow.top;
	iWidth = rcWindow.right - rcWindow.left;
	switch (g_uSide)
	{
	case ABE_TOP:
		rc.bottom = rc.top + iHeight;
		break;
	case ABE_BOTTOM:
		rc.top = rc.bottom - iHeight;
		break;
	case ABE_LEFT:
		rc.right = rc.left + iWidth;
		break;
	case ABE_RIGHT:
		rc.left = rc.right - iWidth;
		break;
	}
	AppBarQuerySetPos(g_uSide, &rc, pabd);
}

void AppBarCallback(HWND hwndAccessBar, UINT_PTR uNotifyMsg, LPARAM lParam)
{
	APPBARDATA abd;
	const UINT_PTR uState = SHAppBarMessage(ABM_GETSTATE, &abd);
	abd.cbSize = sizeof(abd);
	abd.hWnd = hwndAccessBar;
	switch (uNotifyMsg)
	{
	case ABN_STATECHANGE:
		SetWindowPos(hwndAccessBar,
			(ABS_ALWAYSONTOP & uState) ? HWND_TOPMOST : HWND_BOTTOM,
			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		break;
	case ABN_FULLSCREENAPP:
		if (lParam)
		{
			SetWindowPos(hwndAccessBar,
				(ABS_ALWAYSONTOP & uState) ? HWND_TOPMOST : HWND_BOTTOM,
				0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		else if (uState & ABS_ALWAYSONTOP)
		{
			SetWindowPos(hwndAccessBar, HWND_TOPMOST,
				0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		break;
	case ABN_POSCHANGED:
		AppBarPosChanged(&abd);
		break;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static APPBARDATA abd;
	switch (msg)
	{
	case WM_CREATE:
		abd.cbSize = sizeof(APPBARDATA);
		abd.hWnd = hWnd;
		RegisterAccessBar(hWnd, TRUE);
		AppBarPosChanged(&abd);
		return	0;
	case WM_DESTROY:
		RegisterAccessBar(hWnd, FALSE);
		PostQuitMessage(0);
		return	0;
	case WM_ACTIVATE:
		abd.lParam = 0;
		SHAppBarMessage(ABM_ACTIVATE, &abd);
		return	0;
	case WM_WINDOWPOSCHANGED:
		abd.lParam = 0;
		SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
		break;
	case WM_EXITSIZEMOVE:
		AppBarPosChanged(&abd);
		break;
	case WM_NCHITTEST:
	{
		const LRESULT lHitTest = DefWindowProc(
			hWnd, WM_NCHITTEST, wParam, lParam);
		if (lHitTest == HTLEFT)return HTLEFT;
	}
	return HTCLIENT;
	case WM_APP:
		AppBarCallback(hWnd, wParam, lParam);
		return	0;
	}
	return(DefWindowProc(hWnd, msg, wParam, lParam));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("デスクトップ ツール バー"),
		WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME,
		CW_USEDEFAULT,
		0,
		256,
		0,
		0,
		0,
		hInstance,
		0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
