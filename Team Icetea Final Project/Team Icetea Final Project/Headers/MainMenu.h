// MainMenu.h
#pragma once
#include <string>
using namespace std;

// ===== 메인 메뉴 클래스 =====
// 로고 출력과 메뉴 네비게이션(연습/시험/게임/검색/제작)을 담당한다.
class MainMenu
{
public:
    void Show(); // 메인 메뉴 표시 및 입력 처리 루프
private:
    // 자연어(한글) 출력 시 시각적 너비 계산 (한글=2, 영문=1)
    static int GetVisualLength(const string& str);
    // 텍스트를 지정한 너비에 맞춰 가운데 정렬 출력
    static void PrintCentered(const string& text, int logoWidth);
    // 로고 파일을 읽어오고 최대 너비를 반환
    string LoadLogoFromFile(string filename, int& outMaxWidth);
    // 게임모드 하위 메뉴 표시
    void ShowGameModeMenu(int logoWidth);
};
