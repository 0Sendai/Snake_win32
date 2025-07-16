#define WIN32_LEAN_AND_MEAN 
#define BLOCK_SIZE			15
#define SCORE_LIMIT			500

#include <Windows.h>
#include <malloc.h>
#include <stdlib.h>
#include "resource.h"

struct block
{
	int x, y;
	struct block* next;
} blocks;

struct dot_
{
	int x, y;
} dot;

enum Directions {
	LEFT, RIGHT, UP, DOWN
} next_dir = RIGHT;

enum game_state_ {
	RUN, START_PAUSE, CONT_PAUSE, MAIN_MENU
} game_state = RUN;

enum screen_type_ {
	START, PAUSE, GAME_OVER
};

UINT    timer_elapse = 60, score = 0;
RECT    r;
HBRUSH  green, red;
HPEN    nullpen;
HBITMAP background = NULL;
WCHAR	scorestr[SCORE_LIMIT];

void release_blocks(struct block* bp) {
	struct block* first = bp;
	bp = bp->next;
	struct block* bp2;
	while (bp) {
		bp2 = bp->next;
		free(bp);
		bp = bp2;
	}
	first->next = NULL;
}

void decrease_timer(HWND hwnd) {
	timer_elapse += 20;
	KillTimer(hwnd, IDT_TIMER);
	SetTimer(hwnd, IDT_TIMER, timer_elapse, NULL);
}

void increase_timer(HWND hwnd) {
	timer_elapse -= 20;
	KillTimer(hwnd, IDT_TIMER);
	SetTimer(hwnd, IDT_TIMER, timer_elapse, NULL);
}

BOOL CALLBACK delete_buttons(HWND hwnd, LPARAM lparam) {
	DestroyWindow(hwnd);
	return TRUE;
}

void draw_continue_buttons(HWND hwnd) {
	CreateWindow(L"BUTTON", L"CONTINUE", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 190, 390, 150, 90, hwnd, (HMENU)IDB_RESUME, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), NULL);
	CreateWindow(L"BUTTON", L"QUIT", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 450, 390, 150, 90, hwnd, (HMENU)IDB_QUIT, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), NULL);
}

void draw_start_buttons(HWND hwnd) {
	CreateWindow(L"BUTTON", L"START", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 190, 390, 150, 90, hwnd, (HMENU)IDB_NEW, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), NULL);
	CreateWindow(L"BUTTON", L"QUIT", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 450, 390, 150, 90, hwnd, (HMENU)IDB_QUIT, (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), NULL);
}

void escape(HWND hwnd) {
	if (game_state == RUN)
	{
		game_state = START_PAUSE;
		KillTimer(hwnd, IDT_TIMER);
		SetTimer(hwnd, IDT_TIMER2, 100, NULL);
		//draw_continue_buttons(hwnd);
		
	}
	else if (game_state == CONT_PAUSE)
	{
		game_state = RUN;
		KillTimer(hwnd, IDT_TIMER2);
		SetTimer(hwnd, IDT_TIMER, timer_elapse, NULL);

		EnumChildWindows(hwnd, delete_buttons, 0);
	}
}

void handle_keys(HWND hwnd, WPARAM button) {
	switch (button)
	{
	case VK_LEFT:
		next_dir = next_dir != RIGHT ? LEFT : RIGHT;
		break;
	case VK_RIGHT:
		next_dir = next_dir != LEFT  ? RIGHT : LEFT;
		break;
	case VK_DOWN:
		next_dir = next_dir != UP    ? DOWN  : UP;
		break;
	case VK_UP:
		next_dir = next_dir != DOWN  ? UP    : DOWN;
		break;
	case VK_ESCAPE:
		escape(hwnd);
		break;
	case 'Z':
		decrease_timer(hwnd);
		break;
	case 'X':
		increase_timer(hwnd);
		break;
	}


}

void set_next_dir(struct block* end_b, struct block* new_b) {
	switch (next_dir)
	{
	case UP:
		new_b->x = end_b->x;
		new_b->y = end_b->y - BLOCK_SIZE + 1;
		break;

	case DOWN:
		new_b->x = end_b->x;
		new_b->y = end_b->y + BLOCK_SIZE - 1;
		break;

	case RIGHT:
		new_b->x = end_b->x + BLOCK_SIZE - 1;
		new_b->y = end_b->y;
		break;

	case LEFT:
		new_b->x = end_b->x - BLOCK_SIZE + 1;
		new_b->y = end_b->y;
		break;
	}
}

void move_snake() {
	struct block* bp = &blocks;
	while (bp->next) {
		bp->x = bp->next->x;
		bp->y = bp->next->y;
		bp = bp->next;
	}
}

void set_gameover_state(HWND hwnd) {
	KillTimer(hwnd, IDT_TIMER);
	SetTimer(hwnd, IDT_TIMER2, 100, 0);
}

BOOL check_collision(HWND hwnd, struct block* bp) {
	if (bp->x + BLOCK_SIZE > r.right || bp->x < 0 ||
		bp->y + BLOCK_SIZE > r.bottom || bp->y < 0)
	{
		set_gameover_state(hwnd);
		return FALSE;
	}
	struct block* b_check = &blocks;
	while (b_check != bp) {
		if (b_check->x == bp->x && b_check->y == bp->y) {
			set_gameover_state(hwnd);
			return FALSE;
		}
		b_check = b_check->next;
	}

	return TRUE;
}

void gen_dot() {
	dot.x = rand() % (r.right - BLOCK_SIZE);
	dot.y = rand() % (r.bottom - BLOCK_SIZE);
}

BOOL dot_eaten(struct block* bp) {
	return (bp->x <= dot.x && bp->x + BLOCK_SIZE > dot.x
		|| bp->x < dot.x + BLOCK_SIZE && bp->x + BLOCK_SIZE >= dot.x + BLOCK_SIZE) &&
		(bp->y <= dot.y && bp->y + BLOCK_SIZE > dot.y
			|| bp->y < dot.y + BLOCK_SIZE && bp->y + BLOCK_SIZE >= dot.y + BLOCK_SIZE);
}
void increase_snake(HWND hwnd, struct block* bp) {
	struct block* new_b = (struct block*)malloc(sizeof(struct block));
	if (!new_b) {
		KillTimer(hwnd, IDT_TIMER);
		MessageBox(hwnd, L"Memory error", L"Error", MB_OK | MB_ICONEXCLAMATION);
		return ;
	}
	set_next_dir(bp, new_b);
	bp->next = new_b;
	new_b->next = NULL;
}

void draw_menu(HWND hwnd, HDC hdc, enum screen_type_ screen_type, HFONT font, RECT* textr) {
	if (screen_type == PAUSE)
	{
		SetTextColor(hdc, RGB(255, 255, 255));
		DrawText(hdc, L"PAUSE", -1, textr, DT_CENTER | DT_NOCLIP);
		DeleteObject(font);
		font = CreateFontW(40, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Arial");
		SetRect(textr, 0, 100, r.right, 100);
		SelectObject(hdc, font);
		DrawText(hdc, L"Controls:", -1, textr, DT_CENTER | DT_NOCLIP);

		DeleteObject(font);
		font = CreateFontW(30, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Arial");
		SelectObject(hdc, font);
		SetRect(textr, 0, 150, r.right, 100);
		DrawText(hdc, L"Use ←↑↓→ to control snake", -1, textr, DT_CENTER | DT_NOCLIP);
		SetRect(textr, 0, 190, r.right, 100);
		DrawText(hdc, L"Use Z, X to change game speed", -1, textr, DT_CENTER | DT_NOCLIP);
		SetRect(textr, 0, 230, r.right, 100);
		DrawText(hdc, L"Press Esc to pause", -1, textr, DT_CENTER | DT_NOCLIP);
		DeleteObject(font);
	}
	else if (screen_type == GAME_OVER)
	{
		SetTextColor(hdc, RGB(255, 255, 255));
		DrawText(hdc, L"GAME OVER", -1, textr, DT_CENTER | DT_NOCLIP);
		DeleteObject(font);
		font = CreateFontW(40, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Arial");
		SetRect(textr, 0, 100, r.right, 100);
		SelectObject(hdc, font);
		WCHAR str[SCORE_LIMIT];
		wsprintf(str, L"Your score: %d", score);
		DrawText(hdc, str, -1, textr, DT_CENTER | DT_NOCLIP);
	}
	else if (screen_type == START)
	{
		SetTextColor(hdc, RGB(0, 0, 0));
		DrawText(hdc, L"SNAKE", -1, textr, DT_CENTER | DT_NOCLIP);
		DeleteObject(font);
		font = CreateFontW(40, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Arial");
		SetRect(textr, 0, 100, r.right, 100);
		SelectObject(hdc, font);
		DrawText(hdc, L"Controls:", -1, textr, DT_CENTER | DT_NOCLIP);

		DeleteObject(font);
		font = CreateFontW(30, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Arial");
		SelectObject(hdc, font);
		SetRect(textr, 0, 150, r.right, 100);
		DrawText(hdc, L"Use ←↑↓→ to control snake", -1, textr, DT_CENTER | DT_NOCLIP);
		SetRect(textr, 0, 190, r.right, 100);
		DrawText(hdc, L"Use Z, X to change game speed", -1, textr, DT_CENTER | DT_NOCLIP);
		SetRect(textr, 0, 230, r.right, 100);
		DrawText(hdc, L"Press Esc to pause", -1, textr, DT_CENTER | DT_NOCLIP);
		DeleteObject(font);
	}
}

void shadow_screen(HWND hwnd, HDC hdc, HDC blendDC) {
	BITMAPINFO bi = { 0 };
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = r.right;
	bi.bmiHeader.biHeight = -r.bottom;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	void* pvBits = NULL;
	background = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (!background)
	{
		MessageBox(hwnd, L"Error while painting.", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
	HBITMAP oldbmp = SelectObject(blendDC, background);
	DeleteObject(oldbmp);
	BitBlt(blendDC, 0, 0, 800, 600, hdc, 0, 0, SRCCOPY);
	DWORD* pixels = (DWORD*)pvBits;
	for (int i = 0; i < r.right * r.bottom; i++) {
		BYTE* px = (BYTE*)&pixels[i];
		BYTE alpha = 128;

		px[0] = (px[0] * (255 - alpha)) / 255;
		px[1] = (px[1] * (255 - alpha)) / 255;
		px[2] = (px[2] * (255 - alpha)) / 255;

	}
}

void game_screen(HWND hwnd, HDC hdc, enum screen_type_ screen_type) {
	HDC blendDC = CreateCompatibleDC(hdc);
	if (screen_type == START)
	{
		background      = CreateCompatibleBitmap(hdc, 800, 600);
		HBITMAP oldbtm  = SelectObject(blendDC, background);
		HBRUSH brush    = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(blendDC, &r, brush);
		DeleteObject(oldbtm);
		DeleteObject(brush);
	}
	else if (screen_type == PAUSE)
	{
		shadow_screen(hwnd, hdc, blendDC);
		draw_continue_buttons(hwnd);
	}
	else
	{
		shadow_screen(hwnd, hdc, blendDC);
	}
	
	// Make fonts for header
	SetBkMode(blendDC, TRANSPARENT);
	RECT textr;
	HFONT font = CreateFontW(50, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Arial");
	SelectObject(blendDC, font);
	SetRect(&textr, 0, 30, r.right, 100);
	//

	draw_menu(hwnd, blendDC, screen_type, font, &textr);
	BitBlt(hdc, 0, 0, 800, 600, blendDC, 0, 0, SRCCOPY);
	DeleteObject(font);
	
	DeleteDC(blendDC);
	
}

void start_game(HWND hwnd) {
	if (game_state == CONT_PAUSE || game_state == MAIN_MENU)
	{
		game_state = RUN;
		KillTimer(hwnd, IDT_TIMER2);
	}
	gen_dot();
	blocks.x = 20;
	blocks.y = 50;
	next_dir = RIGHT;
	SetTimer(hwnd, IDT_TIMER, timer_elapse, NULL);
	SetWindowText(hwnd, L"Snake Score: 0");
	score = 0;
}

void init_game(HWND hwnd) {
	GetClientRect(hwnd, &r);
	red = CreateSolidBrush(RGB(255, 0, 0));
	green = CreateSolidBrush(RGB(0, 255, 0));
	nullpen = GetStockObject(NULL_PEN);

	game_state = MAIN_MENU;

	HDC hdc = GetDC(hwnd);
	game_screen(hwnd, hdc, START);
	ReleaseDC(hwnd, hdc);
	draw_start_buttons(hwnd);

	SetTimer(hwnd, IDT_TIMER2, 100, 0);
}

void draw_gameover_screen(HWND hwnd, HDC hdc) {
	if (blocks.next) release_blocks(&blocks);
	game_screen(hwnd, hdc, GAME_OVER);
	game_state = MAIN_MENU;
	draw_start_buttons(hwnd);
}

struct block* draw_snake_and_ret_tail(HDC hdc) {
	struct block* bp = &blocks;
	HBRUSH oldbr = SelectObject(hdc, red);
	HPEN   oldpen = SelectObject(hdc, nullpen);
	DeleteObject(oldbr);
	DeleteObject(oldpen);

	Rectangle(hdc, dot.x, dot.y, dot.x + BLOCK_SIZE, dot.y + BLOCK_SIZE);
	SelectObject(hdc, green);


	while (bp->next)
	{
		Rectangle(hdc, bp->x, bp->y, bp->x + BLOCK_SIZE, bp->y + BLOCK_SIZE);
		bp = bp->next;
	}

	Rectangle(hdc, bp->x, bp->y, bp->x + BLOCK_SIZE, bp->y + BLOCK_SIZE);
	return bp;
}

void change_title(HWND hwnd) {
	wsprintf(scorestr, L"Snake Score: %d", ++score);
	SetWindowText(hwnd, scorestr);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg)
	{
	case WM_CREATE:
		init_game(hwnd);
		break;

	case WM_DESTROY:
		DeleteObject(red);
		DeleteObject(green);
		DeleteObject(nullpen);
		if (blocks.next) release_blocks(&blocks);
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;

		HDC hdc = BeginPaint(hwnd, &ps);
		if (!hdc)
		{
			MessageBox(hwnd, L"Error while painting.", L"Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		
		if (game_state == START_PAUSE)
		{
			game_screen(hwnd, hdc, PAUSE);
			game_state = CONT_PAUSE;
		}
		else if (game_state == CONT_PAUSE || game_state == MAIN_MENU)
		{
			HDC memDC = CreateCompatibleDC(hdc);
			HBITMAP oldbmp = SelectObject(memDC, background);
			BitBlt(hdc, 0, 0, 800, 600, memDC, 0, 0, SRCCOPY);
			DeleteObject(oldbmp);
			DeleteDC(memDC);
		}
		else if (game_state == RUN)
		{
			DeleteObject(background);
			background = NULL;

			struct block* bp = draw_snake_and_ret_tail(hdc);

			if (!check_collision(hwnd, bp)) {
				draw_gameover_screen(hwnd, hdc);
				EndPaint(hwnd, &ps);
				return 0;
			}
			move_snake();
			if (dot_eaten(bp)) {
				increase_snake(hwnd, bp);
				change_title(hwnd);
				gen_dot();
			}
			else
				set_next_dir(bp, bp);
		}
		EndPaint(hwnd, &ps);
	}
	break;

	case WM_KEYDOWN:
		handle_keys(hwnd, wparam);
		break;

	case WM_TIMER:
	{
		switch (wparam)
		{
		case IDT_TIMER:
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		case IDT_TIMER2:
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
			
	}
	break;

	case WM_COMMAND:
	{
		switch (LOWORD(wparam))
		{
		case IDB_RESUME:
			escape(hwnd);
			break;
		case IDB_QUIT:
			PostQuitMessage(0);
			break;
		case IDB_NEW:
			EnumChildWindows(hwnd, delete_buttons, 0);
			start_game(hwnd);
			break;
		}
	}
	break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	srand(time(0));
	MSG msg;
	WNDCLASS wc;
	//wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.style = 0;
	wc.lpszClassName = L"Snake game";
	wc.hInstance = hInstance;
	wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszMenuName = 0;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNAKE));
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	if (!RegisterClass(&wc)) {
		MessageBox(0, L"Error while registering class!", L"Error", MB_ICONERROR | MB_OK);
		return 1;
	}
	HWND hwnd;
	int WindowStyle = WS_OVERLAPPED | WS_VISIBLE | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN;
	hwnd = CreateWindow(wc.lpszClassName, L"Snake",
		//WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
		WindowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, 0, 0, hInstance, 0);

	if (hwnd == NULL) {
		MessageBox(0, L"Error while creating window!", L"Error", MB_ICONERROR | MB_OK);
		return 1;
	}

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}