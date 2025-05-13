/*
   혼자 사용할 용도로 만들었기 때문에 README 파일을 만들 생각이 없습니다.
   다만, 혹시 모를 사용자를 위해 주석을 남깁니다.

   정책 변경으로 인해 Win32 프로그램은 원래 환경과 다른 환경에서 실행될 경우
   윈도우 디펜스에 막히며 간혹, 트로이 목마 프로그램이라 오인하는 경우가 있습니다.

   때문에 실행 파일을 따로 올리지 않으며 코드를 읽을 줄 아시는 분들께만 권장드립니다.

   이 프로그램은 gcc 환경에서 제작되었으며 윈도우 운영체제와 함께 배포되는 msimg32 드라이브(DLL)가 필요합니다(기본 포함).

   배경이 투명한 단순 아날로그 시계 프로그램으로, 운영체제에서 지원하는 GDI 함수만을 이용합니다.
   곧, 시스템적 변경 사항이 전혀 없습니다.

   GNU 컴파일러로 빌드하는 프로그램이며 빌드 구문은 다음과 같습니다.
   g++ -o AnlogClock.exe AnalogClock.cpp -mwindows -lmsimg32

   기존엔 다중 모니터를 지원하지 않는 프로그램이었으나, 작성자가 다중 모니터를 사용하게 되어 관련 기능을 추가하였습니다.

   지원되는 기능은 다음과 같습니다.
   1. F1 : 현재 모니터의 좌상단으로 이동합니다.
   2. F2 : 현재 모니터의 우상단으로 이동합니다.
   3. F3 : 현재 모니터의 좌하단으로 이동합니다.
   4. F4 : 현재 모니터의 우하단으로 이동합니다.
   5. Ctrl + 0, Ctrl + Numpad(0): 다음 모니터를 검색합니다. 곧, 프로그램이 다음 모니터로 이동합니다.

   추후 현재 날짜를 보여주거나, 마지막 위치를 기억하는 등의 기능이 추가될 수 있습니다. 
   25.05.06, 08:12(PM) 최종 수정
 */

#include <windows.h>
#include <dbt.h>
#include <math.h>
#define CLASS_NAME TEXT("AnalogClock from Windows")

#define min(a,b)	(((a) < (b)) ? (a) : (b))
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#define abs(a)		(((a) < 0) ? (-a) : (a))

void OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam);

void OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnQueryEndSession(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnDisplayChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
void OnDeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam);
HRESULT OnNcHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow){
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,0,
		hInst,
		NULL, LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW+1),
		NULL,
		CLASS_NAME,
		NULL
	};

	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindowEx(
			WS_EX_CLIENTEDGE | WS_EX_TOPMOST | WS_EX_LAYERED,
			CLASS_NAME,
			CLASS_NAME,
			WS_POPUP, //WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,
			(HMENU)NULL,
			hInst,
			NULL
			);

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	while(GetMessage(&msg, nullptr, 0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	switch(iMessage){
		case WM_CREATE:
			OnCreate(hWnd, wParam, lParam);
			return 0;

		case WM_PAINT:
			OnPaint(hWnd, wParam, lParam);
			return 0;

		case WM_TIMER:
			OnTimer(hWnd, wParam, lParam);
			return 0;

		case WM_SIZE:
			OnSize(hWnd, wParam, lParam);
			return 0;

		case WM_KEYDOWN:
			OnKeyDown(hWnd, wParam, lParam);
			return 0;

		case WM_QUERYENDSESSION:
			OnQueryEndSession(hWnd, wParam, lParam);
			return TRUE;

		case WM_DISPLAYCHANGE:
			OnDisplayChange(hWnd, wParam, lParam);
			return 0;

		case WM_DEVICECHANGE:
			OnDeviceChange(hWnd, wParam, lParam);
			break;

		case WM_DESTROY:
			OnDestroy(hWnd, wParam, lParam);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}























BOOL bFind;
RECT crt, wrt, srt, rtMultipleMonitor;
POINT Mouse, MaxSize, MinSize;
HBITMAP hBitmap, hBitTemp;
SYSTEMTIME lt;
HPEN hSecond, hMinute, hHour;

int sx,sy, cx,cy, BorderSize, Position, cMonitors, nMonitor;

struct bwAttributes{
	COLORREF	rgb;
	BYTE		Opacity;
	DWORD		Flags;
};

struct bwAttributes Attr;

struct Tick{
	float x,y;
};

void DrawClock(HWND hWnd, HDC hdc);

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBitmap){
	HDC hMemDC = CreateCompatibleDC(hdc);
	HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);

	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BitBlt(hdc, x,y, bmp.bmWidth, bmp.bmHeight, hMemDC, 0,0, SRCCOPY);

	SelectObject(hMemDC, hOld);
	DeleteDC(hMemDC);
}

void SetAttribute(HWND hWnd, struct bwAttributes Attr){
	SetLayeredWindowAttributes(hWnd, Attr.rgb, Attr.Opacity, Attr.Flags);
}

void GetAttribute(HWND hWnd, struct bwAttributes *Attr){
	GetLayeredWindowAttributes(hWnd, &Attr->rgb, &Attr->Opacity, &Attr->Flags);
}

void OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam){
	sx = GetSystemMetrics(SM_CXSCREEN);
	sy = GetSystemMetrics(SM_CYSCREEN);
	BorderSize = GetSystemMetrics(SM_CXEDGE);

	TCHAR str[128];
	wsprintf(str, TEXT("BorderSize = %d"), BorderSize);
	SetWindowText(hWnd, str);

	cx = cy = 400;
	SetWindowPos(hWnd, NULL, sx - cx, 0, cx, cy, SWP_NOZORDER);

	Attr = {RGB(255, 0, 255), 125, LWA_ALPHA | LWA_COLORKEY};
	SetAttribute(hWnd, Attr);

	hSecond = CreatePen(PS_SOLID, 3, RGB(255,255,0));
	hMinute = CreatePen(PS_SOLID, 5, RGB(0, 0, 255));
	hHour = CreatePen(PS_SOLID, 7, RGB(255, 0, 0));

	HRGN hWndRgn;
	hWndRgn = CreateEllipticRgn(BorderSize, BorderSize, cx-BorderSize, cy-BorderSize);
	SetWindowRgn(hWnd, hWndRgn, FALSE);

	MaxSize = {cx - BorderSize, cy - BorderSize};
	MinSize = {MaxSize.x >> 1, MaxSize.y >> 1};
	cMonitors = GetSystemMetrics(SM_CMONITORS);

	SetTimer(hWnd, 1, 100, NULL);
}

void OnQueryEndSession(HWND hWnd, WPARAM wParam, LPARAM lParam){
	DestroyWindow(hWnd);
}

void OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam){
	if(hBitmap){DeleteObject(hBitmap);}
	if(hBitTemp){DeleteObject(hBitTemp);}
	if(hSecond){DeleteObject(hSecond);}
	if(hMinute){DeleteObject(hMinute);}
	if(hHour){DeleteObject(hHour);}
	KillTimer(hWnd, 1);
	PostQuitMessage(0);
}

void OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam){
	if(wParam != SIZE_MINIMIZED){
		if(hBitmap != NULL){
			DeleteObject(hBitmap);
			hBitmap = NULL;
		}

		if(hBitTemp != NULL){
			DeleteObject(hBitTemp);
			hBitTemp = NULL;
		}
	}
}

void OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam){
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	DrawBitmap(hdc, 0,0, hBitmap);

	EndPaint(hWnd, &ps);
}


void OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam){
	switch(wParam){
		case 1:
			{
				GetLocalTime(&lt);

				HDC hdc = GetDC(hWnd);
				HDC hMemDC = CreateCompatibleDC(hdc);
				HDC hTempDC = CreateCompatibleDC(hdc);

				if(hBitmap == NULL){
					GetClientRect(hWnd, &crt);
					hBitmap = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
					hBitTemp = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
				}

				HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);
				HGDIOBJ hTempOld = SelectObject(hTempDC, hBitTemp);

				HBRUSH Mask = CreateSolidBrush(Attr.rgb);
				FillRect(hMemDC, &crt, Mask);
				DeleteObject(Mask);
				DrawClock(hWnd, hTempDC);
				// DrawClock(hWnd, hMemDC);
				TransparentBlt(hMemDC, 0,0, crt.right, crt.bottom, hTempDC, 0,0, crt.right, crt.bottom, Attr.rgb);
				// TransparentBlt(hdc, 0,0, crt.right, crt.bottom, hMemDC, 0,0, crt.right, crt.bottom, Attr.rgb);

				SelectObject(hTempDC, hTempOld);
				SelectObject(hMemDC, hOld);
				DeleteDC(hTempDC);
				DeleteDC(hMemDC);
				ReleaseDC(hWnd, hdc);
			}
			InvalidateRect(hWnd, NULL, FALSE);
			break;
	}
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData){
	MONITORINFOEX mi;

	bFind = TRUE;
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &mi);

	if((mi.dwFlags & MONITORINFOF_PRIMARY) == 0){
		(*(LPRECT)dwData) = *lprcMonitor;
		nMonitor = (nMonitor + 1) % cMonitors;
		return FALSE;
	}else{
		if(((nMonitor + 1) % cMonitors) == 0){
			(*(LPRECT)dwData) = *lprcMonitor;
			nMonitor = (nMonitor + 1) % cMonitors;
			return FALSE;
		}
		return TRUE;
	}
}

void OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam){
	BOOL bCtrl = (GetKeyState(VK_CONTROL) & 0x8000);

	switch(wParam){
		case VK_ESCAPE:
			if(MessageBox(hWnd, TEXT("Do you want quit this program?"), TEXT("Program: Analog Clock"), MB_YESNO) == IDYES){
				DestroyWindow(hWnd);
			}
			break;

		case VK_UP:
			GetAttribute(hWnd, &Attr);
			Attr.Opacity = min(255, max(50, Attr.Opacity + 5));
			SetAttribute(hWnd, Attr);
			break;

		case VK_DOWN:
			GetAttribute(hWnd, &Attr);
			Attr.Opacity = min(255, max(50, Attr.Opacity - 5));
			SetAttribute(hWnd, Attr);
			break;

		case VK_LEFT:
			GetWindowRect(hWnd, &wrt);
			InflateRect(&wrt, -5, -5);
			SetWindowPos(hWnd, NULL, wrt.left, wrt.top, max(MinSize.x, (wrt.right - wrt.left)), max(MinSize.y, (wrt.bottom - wrt.top)), SWP_NOZORDER | SWP_NOMOVE);
			{
				HRGN hWndRgn;
				GetClientRect(hWnd, &crt);
				hWndRgn = CreateEllipticRgn(BorderSize, BorderSize, max(MinSize.x, (crt.right - crt.left)) - BorderSize, max(MinSize.y, (crt.bottom - crt.top)) - BorderSize);
				SetWindowRgn(hWnd, hWndRgn, FALSE);
			}
			break;

		case VK_RIGHT:
			GetWindowRect(hWnd, &wrt);
			InflateRect(&wrt, 5, 5);
			SetWindowPos(hWnd, NULL, wrt.left, wrt.top, min(MaxSize.x, (wrt.right - wrt.left)), min(MaxSize.y, (wrt.bottom - wrt.top)), SWP_NOZORDER | SWP_NOMOVE);
			{
				HRGN hWndRgn;
				GetClientRect(hWnd, &crt);
				hWndRgn = CreateEllipticRgn(BorderSize, BorderSize, min(MaxSize.x, (crt.right - crt.left)) - BorderSize, min(MaxSize.y, (crt.bottom - crt.top)) - BorderSize);
				SetWindowRgn(hWnd, hWndRgn, FALSE);
			}
			break;

		case VK_F1:
			if(bFind){
				SetWindowPos(hWnd, NULL, rtMultipleMonitor.left, rtMultipleMonitor.top, 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}else{
				SetWindowPos(hWnd, NULL, 0,0, 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}
			break;

		case VK_F2:
			GetWindowRect(hWnd, &wrt);
			if(bFind){
				SetWindowPos(hWnd, NULL, rtMultipleMonitor.right - (crt.right - crt.left), rtMultipleMonitor.top, 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}else{
				SetWindowPos(hWnd, NULL, sx - (crt.right - crt.left), 0, 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}
			break;

		case VK_F3:
			if(bFind){
				SetWindowPos(hWnd, NULL, rtMultipleMonitor.left, rtMultipleMonitor.bottom - (crt.bottom - crt.top), 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}else{
				SetWindowPos(hWnd, NULL, 0, sy - (crt.bottom - crt.top), 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}
			break;

		case VK_F4:
			if(bFind){
				SetWindowPos(hWnd, NULL, rtMultipleMonitor.right - (crt.right - crt.left), rtMultipleMonitor.bottom - (crt.bottom - crt.top), 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}else{
				SetWindowPos(hWnd, NULL, sx - (crt.right - crt.left), sy - (crt.bottom - crt.top), 0,0, SWP_NOSIZE | SWP_NOZORDER);
			}
			break;

		case 0x30:
		case VK_NUMPAD0:
			if(bCtrl && cMonitors > 1){
				EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&rtMultipleMonitor);
				SetWindowPos(hWnd, NULL, rtMultipleMonitor.left, rtMultipleMonitor.top, 0,0, SWP_NOZORDER | SWP_NOSIZE);
				GetWindowRect(hWnd, &wrt);
			}
			break;
	}
}

void DrawClock(HWND hWnd, HDC hdc){
	POINT Origin, Hand;

	int iWidth, iHeight, iRadius;
	float dRadian, cosf, sinf, PI = 3.1415926535897932384626433832795L;
	float Quadrant = 270.f, Arc = 180.f, Circular = Arc * 2, x, y;

	GetClientRect(hWnd, &crt);

	HBRUSH Mask = CreateSolidBrush(Attr.rgb);
	FillRect(hdc, &crt, Mask);

	iWidth = crt.right - crt.left;
	iHeight = crt.bottom - crt.top;
	iRadius = min(iWidth, iHeight) >> 1;

	Origin = {iWidth >> 1, iHeight >> 1};

	HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	HGDIOBJ hOldPen = SelectObject(hdc, hSecond);

	int XRadius = iRadius - BorderSize * 3;
	int YRadius = iRadius - BorderSize * 3;
	Ellipse(hdc, Origin.x - XRadius, Origin.y - YRadius, Origin.x + XRadius, Origin.y + YRadius);

	for(int i=0; i<12; i++){
		MSG msg;

		if(PeekMessage(&msg, nullptr, 0,0, PM_REMOVE)){
			if(msg.message == WM_QUIT){DestroyWindow(0); break;}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		dRadian = fmod((float)(30.f * i) + Quadrant, Circular) * PI / Arc;
		cosf = cos(dRadian);
		sinf = sin(dRadian);

		// TODO : Tick Marks
		Tick pt1 = {Origin.x + iRadius * 0.95f * cosf, Origin.y + iRadius * 0.95f * sinf};
		Tick pt2 = {Origin.x + iRadius * 0.9f * cosf, Origin.y + iRadius * 0.9f * sinf};

		MoveToEx(hdc, pt1.x, pt1.y, NULL);
		LineTo(hdc, pt2.x, pt2.y);
	}

	// Second
	dRadian = fmod((float)(6.f * lt.wSecond) + Quadrant, Circular) * PI / Arc;
	Hand.x = (int)(Origin.x + iRadius * 0.95f * cos(dRadian));
	Hand.y = (int)(Origin.y + iRadius * 0.95f * sin(dRadian));
	MoveToEx(hdc, Origin.x, Origin.y, NULL);
	LineTo(hdc, Hand.x, Hand.y);

	// Minute
	SelectObject(hdc, hMinute);
	dRadian = fmod((float)(6.f * lt.wMinute) + Quadrant, Circular) * PI / Arc;
	Hand.x = (int)(Origin.x + iRadius * 0.9f * cos(dRadian));
	Hand.y = (int)(Origin.y + iRadius * 0.9f * sin(dRadian));
	MoveToEx(hdc, Origin.x, Origin.y, NULL);
	LineTo(hdc, Hand.x, Hand.y);

	// hour
	SelectObject(hdc, hHour);
	dRadian = fmod((float)(30.f * (lt.wHour % 12)) + 0.5f * lt.wMinute + Quadrant, Circular) * PI / Arc;
	Hand.x = (int)(Origin.x + iRadius * 0.5f * cos(dRadian));
	Hand.y = (int)(Origin.y + iRadius * 0.5f * sin(dRadian));
	MoveToEx(hdc, Origin.x, Origin.y, NULL);
	LineTo(hdc, Hand.x, Hand.y);

	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
	if(Mask){DeleteObject(Mask);}
}

void OnDisplayChange(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&rtMultipleMonitor);
}

void OnDeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam) {

	/*
	   BOOL EnumDisplayDevices(
	   LPCTSTR lpDevice,				// NULL이면 기본 디스플레이 목록 조회
	   DWORD iDevNum,					// 조회할 장치의 인덱스
	   PDISPLAY_DEVICE lpDisplayDevice, // 결과를 받을 구조체
	   DWORD dwFlags					// 일반적으로 0
	   );
	 */

	/*
	   typedef struct _DISPLAY_DEVICE {
	   DWORD cb;              // 구조체 크기 (반드시 설정해야 함)
	   TCHAR DeviceName[32];  // 디스플레이 이름
	   TCHAR DeviceString[128]; // 디바이스 설명 (모델명 등)
	   DWORD StateFlags;      // 현재 상태 플래그 (활성화 여부 등)
	   TCHAR DeviceID[128];   // PnP 디바이스 ID
	   TCHAR DeviceKey[128];  // 레지스트리 경로
	   } DISPLAY_DEVICE;
	 */

	DEV_BROADCAST_HDR *hdr = (DEV_BROADCAST_HDR*)lParam;
	DISPLAY_DEVICE dd = {0,};

	int idx = 0,
		CurrentMonitors = 0;
	switch(wParam){
		case DBT_DEVNODES_CHANGED:
		case DBT_DEVICEARRIVAL:
			dd.cb = sizeof(dd);
			while(EnumDisplayDevices(NULL, idx, &dd, 0)){
				if(dd.StateFlags & DISPLAY_DEVICE_ACTIVE){
					CurrentMonitors++;
				}
				idx++;
			}

			if(cMonitors != CurrentMonitors){
				cMonitors = CurrentMonitors;
				EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&rtMultipleMonitor);
			}
			break;
	}
}
