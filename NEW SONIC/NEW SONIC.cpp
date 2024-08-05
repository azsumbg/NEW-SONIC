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

bool need_left_field = false;
bool need_right_field = false;

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

IDWriteFactory* iWriteFactory = nullptr;
IDWriteTextFormat* nrmText = nullptr;
IDWriteTextFormat* bigText = nullptr;

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

// GAME VARS ***********************************

engine::Creature Sonic = nullptr;

std::vector<engine::FieldItem> vFields;


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

    Collect(&iWriteFactory);
    Collect(&nrmText);
    Collect(&bigText);
    
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

    need_left_field = false;
    need_right_field = false;

    wcscpy_s(current_player, L"A HEDGEHOG");
    name_set = false;

    Collect(&Sonic);
    Sonic = engine::CreatureFactory(100.0f, creature_type::sonic);

    if (!vFields.empty())
    {
        for (int i = 0; i < vFields.size(); i++)Collect(&vFields[i]);
    }
    vFields.clear();

    vFields.push_back(engine::CreateFieldFactory(field_type::field, -scr_width, scr_height - 100.0f));
    vFields.push_back(engine::CreateFieldFactory(field_type::field, 0, scr_height - 100.0f));
    vFields.push_back(engine::CreateFieldFactory(field_type::field, scr_width, scr_height - 100.0f));
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

    case WM_KEYDOWN:
        if (Sonic)
        {
            switch (LOWORD(wParam))
            {
            case VK_RIGHT:
                if (Sonic->NowJumping())break;
                Sonic->dir = dirs::right;
                Sonic->Move((float)(game_speed));
                break;

            case VK_LEFT:
                if (Sonic->NowJumping())break;
                Sonic->dir = dirs::left;
                Sonic->Move((float)(game_speed));
                break;

            case VK_UP:
                Sonic->Jump();
                break;

            case VK_DOWN:
                Sonic->dir = dirs::stop;
                break;

            default:
                Sonic->dir = dirs::stop;
                break;
            }
        }
        break;


    default:return DefWindowProc(hwnd, ReceivedMsg, wParam, lParam);
    }

    return (LRESULT)(FALSE);
}

void CreateResources()
{
    int first_x = GetSystemMetrics(SM_CXSCREEN) / 2 - (int)(scr_width / 2);
    if (GetSystemMetrics(SM_CXSCREEN) < first_x + scr_width || GetSystemMetrics(SM_CYSCREEN) < scr_height + 50)ErrExit(eScreen);

    mainIcon = (HICON)(LoadImage(NULL, L".\\res\\main.ico", IMAGE_ICON, 256, 256, LR_LOADFROMFILE));
    if (!mainIcon)ErrExit(eIcon);

    mainCursor = LoadCursorFromFile(L".\\res\\main.ani");
    outCursor = LoadCursorFromFile(L".\\res\\out.ani");
    if (!mainCursor || !outCursor)ErrExit(eCursor);

    bWinClass.lpszClassName = bWinClassName;
    bWinClass.hInstance = bIns;
    bWinClass.lpfnWndProc = &WinProc;
    bWinClass.hbrBackground = CreateSolidBrush(RGB(150, 150, 150));
    bWinClass.hIcon = mainIcon;
    bWinClass.hCursor = mainCursor;
    bWinClass.style = CS_DROPSHADOW;

    if (!RegisterClass(&bWinClass))ErrExit(eClass);
    else bHwnd = CreateWindowW(bWinClassName, L"MY NEW SONIC 2.0", WS_CAPTION | WS_SYSMENU, first_x, 50,
        (int)(scr_width), (int)(scr_height), NULL, NULL, bIns, NULL);
    if (!bHwnd)ErrExit(eWindow);
    else
    {
        ShowWindow(bHwnd, SW_SHOWDEFAULT);

        HRESULT hr = S_OK;

        D2D1_GRADIENT_STOP gStops[2] = { 0 };
        ID2D1GradientStopCollection* gColl = nullptr;

        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &iDrawFactory);
        if (hr != S_OK)
        {
            LogError(L"Error creating primary DrawFactory");
            ErrExit(eD2D);
        }

        if (iDrawFactory)
        {
            hr = iDrawFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(bHwnd, D2D1::SizeU((UINT32)(scr_width), (UINT32)(scr_height))), &Draw);
            if (hr != S_OK)
            {
                LogError(L"Error creating primary Draw HwndRenderTarget");
                ErrExit(eD2D);
            }

            if (Draw)
            {
                gStops[0].position = 0;
                gStops[0].color = D2D1::ColorF(D2D1::ColorF::MediumSlateBlue);
                gStops[1].position = 1.0f;
                gStops[1].color = D2D1::ColorF(D2D1::ColorF::Indigo);

                hr = Draw->CreateGradientStopCollection(gStops, 2, &gColl);
                if (hr != S_OK)
                {
                    LogError(L"Error creating GradientStopCollection");
                    ErrExit(eD2D);
                }
                
                if (gColl)
                {
                    hr = Draw->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(scr_width / 2, 25.0f),
                        D2D1::Point2F(0, 0), scr_width / 2, 25.0f), gColl, &BckgBrush);
                    if (hr != S_OK)
                    {
                        LogError(L"Error creating primary BackgroundBrush");
                        ErrExit(eD2D);
                    }
                    Collect(&gColl);
                }

                hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::YellowGreen), &TxtBrush);
                if (hr != S_OK)
                {
                    LogError(L"Error creating primary TxtBrush");
                    ErrExit(eD2D);
                }

                hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &HgltBrush);
                if (hr != S_OK)
                {
                    LogError(L"Error creating primary HgltBrush");
                    ErrExit(eD2D);
                }

                hr = Draw->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkGray), &InactBrush);
                if (hr != S_OK)
                {
                    LogError(L"Error creating primary InactBrush");
                    ErrExit(eD2D);
                }
            }
        }

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 
            reinterpret_cast<IUnknown**>(&iWriteFactory));
        if (hr != S_OK)
        {
            LogError(L"Error creating iWriteFactory");
            ErrExit(eD2D);
        }

        if (iWriteFactory)
        {
            hr = iWriteFactory->CreateTextFormat(L"Sitka", NULL, DWRITE_FONT_WEIGHT_BLACK, DWRITE_FONT_STYLE_OBLIQUE,
                DWRITE_FONT_STRETCH_NORMAL, 24.0F, L"", &nrmText);
            if (hr != S_OK)
            {
                LogError(L"Error creating nrmText");
                ErrExit(eD2D);
            }

            hr = iWriteFactory->CreateTextFormat(L"Sitka", NULL, DWRITE_FONT_WEIGHT_BLACK, DWRITE_FONT_STYLE_OBLIQUE,
                DWRITE_FONT_STRETCH_NORMAL, 64.0F, L"", &bigText);
            if (hr != S_OK)
            {
                LogError(L"Error creating biText");
                ErrExit(eD2D);
            }
        }
        
        //BITMAPS **************************

        if (Draw)
        {
            bmpBrick = Load(L".\\res\\img\\field\\brick.png", Draw);
            if (!bmpBrick)
            {
                LogError(L"Error loading bmpBrick");
                ErrExit(eD2D);
            }

            bmpGoldBrick = Load(L".\\res\\img\\field\\goldbrick.png", Draw);
            if (!bmpGoldBrick)
            {
                LogError(L"Error loading bmpGoldBrick");
                ErrExit(eD2D);
            }

            bmpGold = Load(L".\\res\\img\\field\\gold.png", Draw);
            if (!bmpGold)
            {
                LogError(L"Error loading bmpGold");
                ErrExit(eD2D);
            }

            bmpBush = Load(L".\\res\\img\\field\\bush.png", Draw);
            if (!bmpBush)
            {
                LogError(L"Error loading bmpBush");
                ErrExit(eD2D);
            }

            bmpField = Load(L".\\res\\img\\field\\field.png", Draw);
            if (!bmpField)
            {
                LogError(L"Error loading bmpField");
                ErrExit(eD2D);
            }

            bmpSky = Load(L".\\res\\img\\field\\sky.png", Draw);
            if (!bmpSky)
            {
                LogError(L"Error loading bmpSky");
                ErrExit(eD2D);
            }

            bmpPlatform = Load(L".\\res\\img\\field\\platform.png", Draw);
            if (!bmpPlatform)
            {
                LogError(L"Error loading bmpPlatform");
                ErrExit(eD2D);
            }

            bmpPortal = Load(L".\\res\\img\\field\\portal.png", Draw);
            if (!bmpPortal)
            {
                LogError(L"Error loading bmpPortal");
                ErrExit(eD2D);
            }

            bmpRip = Load(L".\\res\\img\\field\\Rip.png", Draw);
            if (!bmpRip)
            {
                LogError(L"Error loading bmpRip");
                ErrExit(eD2D);
            }

            bmpTree = Load(L".\\res\\img\\field\\Tree.png", Draw);
            if (!bmpTree)
            {
                LogError(L"Error loading bmpTree");
                ErrExit(eD2D);
            }

            bmpMushroom = Load(L".\\res\\img\\evil\\mushroom.png", Draw);
            if (!bmpMushroom)
            {
                LogError(L"Error loading bmpMushroom");
                ErrExit(eD2D);
            }

            bmpDizzy = Load(L".\\res\\img\\evil\\dizzy.png", Draw);
            if (!bmpDizzy)
            {
                LogError(L"Error loading bmpDizzy");
                ErrExit(eD2D);
            }

            for (int i = 0; i < 6; i++)
            {
                wchar_t name[100] = L".\\res\\img\\sonic\\left\\";
                wchar_t add[5] = L"\0";
                wsprintf(add, L"%d", i);
                wcscat_s(name, add);
                wcscat_s(name, L".gif");
                bmpSonicL[i] = Load(name, Draw);
                if (!bmpSonicL[i])
                {
                    LogError(L"Error loading bmpSonicL");
                    ErrExit(eD2D);
                }
            }
            for (int i = 0; i < 6; i++)
            {
                wchar_t name[100] = L".\\res\\img\\sonic\\right\\";
                wchar_t add[5] = L"\0";
                wsprintf(add, L"%d", i);
                wcscat_s(name, add);
                wcscat_s(name, L".gif");
                bmpSonicR[i] = Load(name, Draw);
                if (!bmpSonicR[i])
                {
                    LogError(L"Error loading bmpSonicR");
                    ErrExit(eD2D);
                }
            }
        }

        //////////////////////////////////////

        D2D1_RECT_F UpR = { -500.0f,-150.0f,0,0 };
        D2D1_RECT_F DownR = { scr_width + 300.0f,scr_height + 150.0f,scr_width + 700.0f,scr_height + 300.0f };

        wchar_t UpTxt[21] = L"РАЗХОДКАТА НА ТАРЛЬО";
        wchar_t DownTxt[12] = L"dev. Daniel";

        bool up_ok = false;
        bool down_ok = false;

        mciSendString(L"play .\\res\\snd\\intro.wav", NULL, NULL, NULL);

        while (!up_ok || !down_ok)
        {
            if (!up_ok)
            {
                if (UpR.left < scr_width / 2 - 200.0f)
                {
                    UpR.left+=4.0f;
                    UpR.right+=4.0f;
                }
                if (UpR.top < scr_height / 2 - 150.0f)
                {
                    UpR.top+=4.0f;
                    UpR.bottom+=4.0f;
                }

                if (UpR.left >= scr_width / 2 - 200.0f && UpR.top >= scr_height / 2 - 150.0f)up_ok = true;
            }
            if (!down_ok)
            {
                if (DownR.left > scr_width / 2 - 150.0f)
                {
                    DownR.left -= 4.0f;
                    DownR.right -= 4.0f;
                }
                if (DownR.top > scr_height / 2 + 150.0f)
                {
                    DownR.top -= 4.0f;
                    DownR.bottom -= 4.0f;
                }

                if (DownR.left <= scr_width / 2 - 150.0f && DownR.top <= scr_height / 2 + 150.0f)down_ok = true;
            }

            if (Draw && BckgBrush && TxtBrush && bigText)
            {
                Draw->BeginDraw();
                Draw->Clear(D2D1::ColorF(D2D1::ColorF::Indigo));
                Draw->DrawText(UpTxt, 21, bigText, UpR, TxtBrush);
                Draw->DrawText(DownTxt, 12, bigText, DownR, TxtBrush);
                Draw->EndDraw();
            }
        }
        Sleep(1500);
    }
}
///////////////////////////////////////////////////////////////

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    bIns = hInstance;
    if (!bIns)
    {
        LogError(L"Windows hInstance error");
        ErrExit(eClass);
    }

    CreateResources();

    while (bMsg.message != WM_QUIT)
    {
        if ((bRet = PeekMessage(&bMsg, bHwnd, NULL, NULL, PM_REMOVE)) != 0)
        {
            if (bRet == -1)ErrExit(eMsg);
            TranslateMessage(&bMsg);
            DispatchMessageW(&bMsg);
        }

        if (pause)
        {
            if (show_help)continue;
            
            if (Draw && bigText && TxtBrush)
            {
                Draw->BeginDraw();
                Draw->Clear(D2D1::ColorF(D2D1::ColorF::Indigo));
                Draw->DrawText(L"ПАУЗА", 6, bigText, D2D1::RectF(scr_width / 2 - 100.0f, scr_height / 2 - 100.0f, 
                    scr_width, scr_height),TxtBrush);
                Draw->EndDraw();
            }
            continue;
        }

        ///////////////////////////////////////////////////////////

        if (Sonic)
        {
            if (Sonic->NowJumping())Sonic->Jump();
        }

        if (!vFields.empty())
        {
            for (std::vector<engine::FieldItem>::iterator field = vFields.begin(); field < vFields.end(); field++)
            {
                bool ended = false;
                if (!Sonic)break;
                else
                {
                    switch (Sonic->dir)
                    {
                    case dirs::right:
                        (*field)->dir = dirs::left;
                        (*field)->Move((float)(game_speed));
                        if ((*field)->x <= -scr_width)
                        {
                            (*field)->Release();
                            vFields.erase(field);
                            need_right_field = true;
                            ended = true;
                        }
                        break;

                    case dirs::left:
                        (*field)->dir = dirs::right;
                        (*field)->Move((float)(game_speed));
                        if ((*field)->ex >= 2 * scr_width)
                        {
                            (*field)->Release();
                            vFields.erase(field);
                            need_left_field = true;
                            ended = true;
                        }
                        break;
                    }
                }
                if (ended)break;
            }
        }

        if (need_right_field)
        {
            need_right_field = false;
            if (!vFields.empty())vFields.push_back(engine::CreateFieldFactory(field_type::field,
                vFields.back()->ex, vFields.back()->y));
        }

        if (need_left_field)
        {
            need_left_field = false;
            
            if (!vFields.empty())
            {
                float start_x = (*vFields.begin())->x - scr_width;
                float start_y = (*vFields.begin())->y;
                vFields.push_back(engine::CreateFieldFactory(field_type::field, start_x, start_y));
            }
        }

        
        //DRAW THINGS ************************************

        if (Draw && nrmText && bigText && TxtBrush && HgltBrush && InactBrush)
        {
            Draw->BeginDraw();
            Draw->FillRectangle(D2D1::RectF(0, 0, scr_width, 50.0f), BckgBrush);
            if (name_set)
                Draw->DrawText(L"ИМЕ НА ИГРАЧ", 13, nrmText, b1Rect, InactBrush);
            else
            {
                if (b1Hglt)Draw->DrawText(L"ИМЕ НА ИГРАЧ", 13, nrmText, b1Rect, HgltBrush);
                else Draw->DrawText(L"ИМЕ НА ИГРАЧ", 13, nrmText, b1Rect, TxtBrush);
            }
            if (b2Hglt)Draw->DrawText(L"ЗВУЦИ ON / OFF", 15, nrmText, b2Rect, HgltBrush);
            else Draw->DrawText(L"ЗВУЦИ ON / OFF", 15, nrmText, b2Rect, TxtBrush);
            if (b3Hglt)Draw->DrawText(L"ПОМОЩ ЗА ИГРАТА", 16, nrmText, b3Rect, HgltBrush);
            else Draw->DrawText(L"ПОМОЩ ЗА ИГРАТА", 16, nrmText, b3Rect, TxtBrush);
            Draw->DrawBitmap(bmpSky, D2D1::RectF(0, 50, scr_width, 655.0f));
            
            if (!vFields.empty())
                for (int i = 0; i < vFields.size(); i++)
                    Draw->DrawBitmap(bmpField, D2D1::RectF(vFields[i]->x, vFields[i]->y, vFields[i]->ex, vFields[i]->ey));
        }

        if (Sonic)
        {
            switch (Sonic->dir)
            {
            case dirs::stop:
                Draw->DrawBitmap(bmpSonicR[0], D2D1::RectF(Sonic->x, Sonic->y, Sonic->ex, Sonic->ey));
                break;

            case dirs::right:
                Draw->DrawBitmap(bmpSonicR[Sonic->GetFrame()], D2D1::RectF(Sonic->x, Sonic->y, Sonic->ex, Sonic->ey));
                break;

            case dirs::left:
                Draw->DrawBitmap(bmpSonicL[Sonic->GetFrame()], D2D1::RectF(Sonic->x, Sonic->y, Sonic->ex, Sonic->ey));
                break;
            }
        }

        //////////////////////////////////////////////////
        Draw->EndDraw();
    }

    std::remove(tmp_file);
    ReleaseMem();
    return (int) bMsg.wParam;
}