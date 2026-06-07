#include "MainMenu.h"
#include "QuestionData.h"


int main()
{
    // 데이터 로드
    QuestionBank::Instance().LoadAll();

    // 메뉴 실행
    MainMenu menu;
    menu.Show();

    return 0;
}
