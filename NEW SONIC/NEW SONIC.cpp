#include "framework.h"
#include "NEW SONIC.h"
#include <mmsystem.h>
#include <d2d1.h>
#include <dwrite.h>
#include "ErrH.h"
#include "FCheck.h"
#include "D2BMPLOADER.h"
#include "intersect.h"
#include "soniceng.h"
#include <vector>
#include <ctime>
#include <chrono>
#include <fstream>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "errh.lib")
#pragma comment(lib, "fcheck.lib")
#pragma comment(lib, "d2bmploader.lib")
#pragma comment(lib, "intersect.lib")
#pragma comment(lib, "soniceng.lib")

#define bWinClassName L"MySonicGame"

#define tmp_file ".\\res\\data\\temp.dat"
#define Ltmp_file L".\\res\\data\\temp.dat"
#define save_file L".\\res\\data\\save.dat"
#define record_file L".\\res\\data\\record.dat"
#define help_file L".\\res\\data\\help.dat"
#define snd_file L".\\res\\snd\\main.wav"

#define mNew 1001
#define mSpeed 1002
#define mExit 1003
#define mSave 1004
#define mLoad 1005
#define mHoF 1006

#define record 2001
#define first_record 2002
#define no_record 2003

WNDCLASS bWinClass = { 0 };
HINSTANCE bIns = nullptr;
HWND bHwnd = nullptr;
HMENU bBar = nullptr;
HMENU bMain = nullptr;
HMENU bStore = nullptr;
HICON mainIcon = nullptr;
HCURSOR mainCursor = nullptr;
HCURSOR outCursor = nullptr;
HDC PaintDC = nullptr;
PAINTSTRUCT bPaint = { 0 };
MSG bMsg = { 0 };
BOOL bRet = 0;
POINT cur_pos = { 0,0 };

//////////////////////////////////////////////////////

bool pause = false;
bool sound = true;
bool show_help = false;
bool in_client = true;
bool b1Hglt = false;
bool b2Hglt = false;
bool b3Hglt = false;
bool name_set = false;

D2D1_RECT_F b1Rect = { 0, 0, scr_width / 3 - 50.0f, 50.0f };
D2D1_RECT_F b2Rect = { scr_width / 3, 0, scr_width * 2 / 3 - 50.0f, 50.0f };
D2D1_RECT_F b3Rect = { scr_width * 2 / 3, 0, scr_width, 50.0f };

wchar_t current_player[16] = L"A HEDGEHOG";

int game_speed = 1;
int score = 0;
int mins = 0;
int secs = 0;
UINT bTimer = -1;

/////////////////////////////////////////////////

ID2D1Factory* iDrawFactory = nullptr;
ID2D1HwndRenderTarget* Draw = nullptr;

ID2D1RadialGradientBrush* BckgBrush = nullptr;
ID2D1SolidColorBrush* TxtBrush = nullptr;
ID2D1SolidColorBrush* HgltBrush = nullptr;
ID2D1SolidColorBrush* InactBrush = nullptr;

ID2D1Bitmap* bmpField = nullptr;
ID2D1Bitmap* bmpSky = nullptr;
ID2D1Bitmap* bmpBrick = nullptr;
ID2D1Bitmap* bmpBush = nullptr;
ID2D1Bitmap* bmpGoldBrick = nullptr;
ID2D1Bitmap* bmpGold = nullptr;
ID2D1Bitmap* bmpPlatform = nullptr;
ID2D1Bitmap* bmpRip = nullptr;
ID2D1Bitmap* bmpPortal = nullptr;
ID2D1Bitmap* bmpTree = nullptr;

ID2D1Bitmap* bmpMushroom = nullptr;
ID2D1Bitmap* bmpDizzy = nullptr;

ID2D1Bitmap* bmpSonicL[6] = { nullptr };
ID2D1Bitmap* bmpSonicR[6] = { nullptr };

/////////////////////////////////////////////////

template <typename T> concept CanBeReleased = requires(T var)
{
    var.Release();
};
template <CanBeReleased GARBAGE> bool Collect(GARBAGE** what)
{
    if ((*what))
    {
        (*what)->Release();
        (*what) = nullptr;
        return true;
    }

    return false;
}
void LogError(LPCWSTR what)
{
    std::wofstream log(L".\\res\\data\\error.log", std::ios::app);
    log << what << L" ! Time stamp of this error: " << std::chrono::system_clock::now() << std::endl;
    log.close();
}
void ReleaseMem()
{
    Collect(&iDrawFactory);
    Collect(&Draw);
    Collect(&BckgBrush);
    Collect(&TxtBrush);
    Collect(&HgltBrush);
    Collect(&InactBrush);
    
    Collect(&bmpField);
    Collect(&bmpSky);
    Collect(&bmpBrick);
    Collect(&bmpGoldBrick);
    Collect(&bmpBush);
    Collect(&bmpGold);
    Collect(&bmpPlatform);
    Collect(&bmpRip);
    Collect(&bmpPortal);
    Collect(&bmpTree);

    Collect(&bmpMushroom);
    Collect(&bmpDizzy);

    for (int i = 0; i < 6; ++i)Collect(&bmpSonicL[i]);
    for (int i = 0; i < 6; ++i)Collect(&bmpSonicR[i]);
}
void ErrExit(int what)
{
    MessageBeep(MB_ICONERROR);
    MessageBoxW(NULL, ErrHandle(what), L"КРИТИЧНА ГРЕШКА !", MB_OK | MB_APPLMODAL | MB_ICONERROR);

    ReleaseMem();
    std::remove(tmp_file);
    exit(1);
}
void InitGame()
{
    game_speed = 1;
    score = 0;
    mins = 0;
    secs = 0;

    wcscpy_s(current_player, L"A HEDGEHOG");
    name_set = false;
}

void GameOver()
{
    KillTimer(bHwnd, bTimer);
    PlaySound(NULL, NULL, NULL);



    bMsg.message = WM_QUIT;
    bMsg.wParam = 0;
}

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT ReceivedMsg, WPARAM wParam, LPARAM lParam)
{
    switch (ReceivedMsg)
    {
    case WM_INITDIALOG:
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)(mainIcon));
        return true;
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;

        case IDOK:
            if (GetDlgItemText(hwnd, IDC_NAME, current_player, 15) < 1)
            {
                if (!name_set)wcscpy_s(current_player, L"A HEDGEHOG");
                if (sound)mciSendString(L"play .\\res\\snd\\negative.wav", NULL, NULL, NULL);
                MessageBox(bHwnd, L"Ха, ха, ха ! Забрави си името !", L"Забраватор !", MB_OK | MB_APPLMODAL | MB_ICONEXCLAMATION);
                EndDialog(hwnd, IDCANCEL);
                break;
            }
            EndDialog(hwnd, IDOK);
            break;
        }
        break;
    }

    return (INT_PTR)(FALSE);
}
LRESULT CALLBACK WinProc(HWND hwnd, UINT ReceivedMsg, WPARAM wParam, LPARAM lParam)
{
    switch (ReceivedMsg)
    {
    case WM_CREATE:
        SetTimer(hwnd, bTimer, 1000, NULL);
        srand((unsigned int)(time(0)));

        bBar = CreateMenu();
        bMain = CreateMenu();
        bStore = CreateMenu();
        
        AppendMenu(bBar, MF_POPUP, (UINT_PTR)(bMain), L"Основно меню");
        AppendMenu(bBar, MF_POPUP, (UINT_PTR)(bStore), L"Меню за данни");
        
        AppendMenu(bMain, MF_STRING, mNew, L"Нова игра");
        AppendMenu(bMain, MF_STRING, mSpeed, L"Турбо режим");
        AppendMenu(bMain, MF_SEPARATOR, NULL, NULL);
        AppendMenu(bMain, MF_STRING, mExit, L"Изход");

        AppendMenu(bStore, MF_STRING, mSave, L"Запази игра");
        AppendMenu(bStore, MF_STRING, mLoad, L"Зареди игра");
        AppendMenu(bStore, MF_SEPARATOR, NULL, NULL);
        AppendMenu(bStore, MF_STRING, mHoF, L"Зала на славата");
        SetMenu(hwnd, bBar);
        InitGame();
        break;

    case WM_CLOSE:
        pause = true;
        if (sound)mciSendString(L"play .\\res\\snd\\select.wav", NULL, NULL, NULL);
        if (MessageBox(hwnd, L"Ако излезеш, ще загубиш тази игра !\n\nНаистина ли излизаш ?", L"Изход",
            MB_OK | MB_APPLMODAL | MB_ICONQUESTION) == IDNO)
        {
            pause = false;
            break;
        }
        GameOver();
        break;

    case WM_PAINT:
        PaintDC = BeginPaint(hwnd, &bPaint);
        FillRect(PaintDC, &bPaint.rcPaint, CreateSolidBrush(RGB(150, 150, 150)));
        EndPaint(hwnd, &bPaint);
        break;

    case WM_SETCURSOR:
        GetCursorPos(&cur_pos);
        ScreenToClient(hwnd, &cur_pos);

        if (LOWORD(lParam) == HTCLIENT)
        {
            if (!in_client)
            {
                in_client = true;
                pause = false;
            }

            if (cur_pos.y <= 50)
            {
                if (cur_pos.x >= b1Rect.left && cur_pos.x <= b1Rect.right)
                {
                    if (!b1Hglt)
                    {
                        b1Hglt = true;
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                    }
                    if (b2Hglt || b3Hglt)
                    {
                        b2Hglt = false;
                        b3Hglt = false;
                    }
                }
                if (cur_pos.x >= b2Rect.left && cur_pos.x <= b2Rect.right)
                {
                    if (!b2Hglt)
                    {
                        b2Hglt = true;
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                    }
                    if (b1Hglt || b3Hglt)
                    {
                        b1Hglt = false;
                        b3Hglt = false;
                    }
                }
                if (cur_pos.x >= b3Rect.left && cur_pos.x <= b3Rect.right)
                {
                    if (!b3Hglt)
                    {
                        b3Hglt = true;
                        if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                    }
                    if (b1Hglt || b2Hglt)
                    {
                        b1Hglt = false;
                        b2Hglt = false;
                    }
                }

                SetCursor(outCursor);
                return true;
            }
            else
            {
                if (b1Hglt || b2Hglt || b3Hglt)
                {
                    if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                    b1Hglt = false;
                    b2Hglt = false;
                    b3Hglt = false;
                }
                SetCursor(mainCursor);
                return true;
            }
        }
        else
        {
            if (in_client)
            {
                in_client = false;
                pause = true;
            }
            if (b1Hglt || b2Hglt || b3Hglt)
            {
                if (sound)mciSendString(L"play .\\res\\snd\\click.wav", NULL, NULL, NULL);
                b1Hglt = false;
                b2Hglt = false;
                b3Hglt = false;
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return true;
        }
        break;

    case WM_TIMER:
        if (pause)break;
        secs++;
        mins = secs / 60;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case mNew:
            pause = true;
            if (sound)mciSendString(L"play .\\res\\snd\\select.wav", NULL, NULL, NULL);
            if (MessageBox(hwnd, L"Ако рестартираш, ще загубиш тази игра !\n\nНаистина ли рестартираш ?", L"Рестарт",
                MB_OK | MB_APPLMODAL | MB_ICONQUESTION) == IDNO)
            {
                pause = false;
                break;
            }
            InitGame();
            pause = false;
            break;

        case mSpeed:
            pause = true;
            if (sound)mciSendString(L"play .\\res\\snd\\select.wav", NULL, NULL, NULL);
            if (MessageBox(hwnd, L"Сигурен ли си, че пускаш турбо-режим ?", L"Турбо режим",
                MB_OK | MB_APPLMODAL | MB_ICONQUESTION) == IDNO)
            {
                pause = false;
                break;
            }
            game_speed++;
            pause = false;
            break;

        case mExit:
            SendMessage(hwnd, WM_CLOSE, NULL, NULL);
            break;



        }
        break;



    default:return DefWindowProc(hwnd, ReceivedMsg, wParam, lParam);
    }

    return (LRESULT)(FALSE);
}




int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    




    std::remove(tmp_file);
    ReleaseMem();
    return (int) bMsg.wParam;
}