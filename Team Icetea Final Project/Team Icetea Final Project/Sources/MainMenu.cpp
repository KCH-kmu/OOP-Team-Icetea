#include "MainMenu.h"
#include "Utils.h"
#include "QuestionFeatures.h"
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <conio.h>

using namespace std;

// 자연어(한글) 콘솔 출력 시 시각적 너비를 계산하는 함수 (한글=2, 영문=1)
int GetVisualLength(const string& str)
{
    int length = 0;
    for (size_t i = 0; i < str.length(); ++i)
    {
        // 최상위 비트가 1인지 확인 (멀티바이트 문자, 주로 한글)
        if ((str[i] & 0x80) != 0)
        {
            length += 2; // 한글은 2칸 차지
            
            int charLen = 1;
            unsigned char c = (unsigned char)str[i];
            if (c >= 0xF0) charLen = 4;
            else if (c >= 0xE0) charLen = 3;
            else if (c >= 0xC0) charLen = 2;

            i += (charLen - 1);
        }
        else
        {
            length += 1; // 영문/숫자는 1칸 차지
        }
    }
    return length;
}

// 텍스트를 로고의 너비에 맞춰 가운데 정렬하여 출력하는 함수
void PrintCentered(const string& text, int logoWidth)
{
    int textLen = GetVisualLength(text);
    int padding = (logoWidth - textLen) / 2;
    if (padding < 0) padding = 0;

    for (int i = 0; i < padding; ++i) cout << " ";
    cout << text << endl;
}

// 로고를 파일에서 읽어오고, 로고의 최대 너비를 반환하는 함수
string LoadLogoFromFile(string filename, int& outMaxWidth)
{
    ifstream file(filename);
    string logo = "";
    outMaxWidth = 0;

    if (!file.is_open())
    {
        string errorMsg = "로고 파일을 찾을 수 없습니다.";
        outMaxWidth = GetVisualLength(errorMsg);
        return errorMsg;
    }

    int count;
    file >> count; // 첫 줄에 줄 개수 읽기

    string dummy;
    getline(file, dummy); // 개행 처리

    for (int i = 0; i < count; i++)
    {
        string line;
        getline(file, line);
        // 줄바꿈 문자 처리 (윈도우/리눅스 호환)
        if (!line.empty() && line.back() == '\r') line.pop_back();

        int currentWidth = GetVisualLength(line);
        if (currentWidth > outMaxWidth) outMaxWidth = currentWidth;

        logo += line + "\n";
    }
    file.close();
    return logo;
}

void ShowGameModeMenu(int logoWidth)
{
    string chooseList[2] = { "1. 몬스터 처치", "2. 무한모드" };
    int focus = 1;
    int getkey;

    do
    {
        screenClear();

        PrintCentered("=== 게임모드 ===", logoWidth);
        cout << endl;

        int maxMenuLen = 0;
        for (const string& s : chooseList) {
            int len = GetVisualLength(s) + 2; 
            if (len > maxMenuLen) maxMenuLen = len;
        }

        int menuPadding = (logoWidth - maxMenuLen) / 2;
        if (menuPadding < 0) menuPadding = 0;

        int listSize = sizeof(chooseList) / sizeof(string);
        for (int i = 0; i < listSize; i++)
        {
            for(int k=0; k<menuPadding; k++) cout << " ";

            if (focus == i + 1)
                cout << "> " << chooseList[i] << endl;
            else
                cout << "  " << chooseList[i] << endl;
        }
        cout << endl;

        string footer = "↑↓:이동  Enter:선택  ESC:뒤로 가기";
        string separator = "----------------------------------------------------------------------";

        PrintCentered(separator, logoWidth);
        PrintCentered(footer, logoWidth);
        PrintCentered(separator, logoWidth);

        getkey = _getch();

        if (getkey == 224) {
            getkey = _getch();
        }

        if (getkey == KEY_UP)
        {
            focus--;
            if (focus < 1) focus = listSize;
        }
        else if (getkey == KEY_DOWN)
        {
            focus++;
            if (focus > listSize) focus = 1;
        }
        else if (getkey == KEY_ENTER)
        {
            screenClear();

            if (focus == 1)
            {
                BossMonsterMode();
            }
            else if (focus == 2)
            {
                InfiniteTowerMode();
            }
        }
        else if (getkey == KEY_ESC)
        {
            return;
        }
    } while (1);
}

void ShowMainMenu()
{
    int logoWidth = 0;
    string projectLogo = LoadLogoFromFile("ProjectLogo.txt", logoWidth);

    if (logoWidth < 40) logoWidth = 80;

    string chooseList[5] = { "1. 연습문제 풀기", "2. 시험모드", "3. 게임모드", "4. 문제 검색", "5. 문제 제작" };
    int focus = 1;
    int getkey;

    do
    {
        screenClear();

        cout << projectLogo << endl;

        PrintCentered("문제은행 alpha 0.01", logoWidth);
        cout << endl;

        int maxMenuLen = 0;
        for (const string& s : chooseList) {
            int len = GetVisualLength(s) + 2; // "> " 부분 포함
            if (len > maxMenuLen) maxMenuLen = len;
        }

        int menuPadding = (logoWidth - maxMenuLen) / 2;
        if (menuPadding < 0) menuPadding = 0;

        for (int i = 0; i < 5; i++)
        {
            for(int k=0; k<menuPadding; k++) cout << " ";

            if (focus == i + 1)
                cout << "> " << chooseList[i] << endl;
            else
                cout << "  " << chooseList[i] << endl;
        }
        cout << endl;

        string footer = "↑↓:이동  Enter:선택  ESC:종료";
        string separator = "----------------------------------------------------------------------";

        PrintCentered(separator, logoWidth);
        PrintCentered(footer, logoWidth);
        PrintCentered(separator, logoWidth);

        getkey = _getch();

        if (getkey == 224) {
            getkey = _getch();
        }

        if (getkey == KEY_UP)
        {
            focus--;
            if (focus < 1) focus = 5;
        }
        else if (getkey == KEY_DOWN)
        {
            focus++;
            if (focus > 5) focus = 1;
        }
        else if (getkey == KEY_ENTER)
        {
            screenClear();
            switch (focus)
            {
            case 1:
                // NEW
                PracticeQuestionSolve();
                break;
            case 2:
                ExamQuestionSolve();
                break;
            case 3:
                ShowGameModeMenu(logoWidth);
                break;
            case 4:
                PracticeQuestionSearch();
                break;
            case 5:
                MakeQuestion(); // 통합 문제 제작 함수
                break;
            }
        }
        else if (getkey == KEY_ESC)
        {
            return;
        }
    } while (1);
}



