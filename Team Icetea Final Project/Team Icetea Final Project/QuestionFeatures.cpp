#include "QuestionFeatures.h"
#include "QuestionData.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // min, max 사용

using namespace std;

// [유지됨] 세미콜론 줄바꿈 기능 제거 (원래대로 복구)
void ShowQuestionDetail(const Question& Question)
{
    screenClear();
    cout << "=== 문제 상세 정보 ===" << endl;
    cout << "--------------------------------" << endl;
    cout << " [이름] : " << Question.nameKr << endl;
    cout << " [Name] : " << Question.nameEn << endl;
    cout << "--------------------------------" << endl;
    cout << " [캐릭터] : " << Question.character << endl;
    cout << " [키워드] : " << (Question.keyword.empty() ? "(없음)" : Question.keyword) << endl;
    cout << "--------------------------------" << endl;
    cout << " [설명] " << endl;

    // 단순 출력으로 변경
    cout << (Question.desc.empty() ? "(설명 데이터가 없습니다)" : Question.desc) << endl;

    cout << "--------------------------------" << endl;
    cout << "\nESC 또는 아무 키나 누르면 목록으로 돌아갑니다.";
    _getch(); // 키 입력 대기
}

// [수정됨] 검색 기준 선택을 상하 방향키 메뉴 방식으로 변경
void SearchLogic(const vector<Question>& targetQuestions, string typeName)
{
    while (true)
    {
        // 1. 검색 기준 선택 메뉴 (상하 이동)
        int focus = 1; // 1: 과목, 2: 키워드
        int criteria = 0; // 선택된 검색 기준

        // 검색 기준 텍스트 배열 (출력용)
        string criteriaNames[2] = {"과목", "키워드"};

        while (true)
        {
            screenClear();
            cout << "=== " << typeName << " 문제 검색 ===" << endl;
            cout << "검색 기준을 선택하세요." << endl;
            cout << "--------------------------------" << endl;

            if (focus == 1) cout << "> 1. 과목" << endl;
            else            cout << "  1. 과목" << endl;

            if (focus == 2) cout << "> 2. 키워드" << endl;
            else            cout << "  2. 키워드" << endl;

            cout << "--------------------------------" << endl;
            cout << "↑↓: 이동, Enter: 선택, ESC: 뒤로 가기" << endl;

            int key = _getch();
            if (key == 224) // 화살표 키 처리
            {
                key = _getch();
                if (key == KEY_UP)
                {
                    focus--;
                    if (focus < 1) focus = 2;
                }
                else if (key == KEY_DOWN)
                {
                    focus++;
                    if (focus > 2) focus = 1;
                }
            }
            else if (key == KEY_ENTER)
            {
                criteria = focus;
                break; // 선택 완료
            }
            else if (key == KEY_ESC)
            {
                return; // 함수 종료 (뒤로 가기)
            }
        }

        // 2. 검색어 입력
        string query;
        screenClear();
        cout << "=== " << typeName << " 문제 검색 ===" << endl;
        // 선택한 기준을 화면에 표시
        cout << "[검색 기준: " << criteriaNames[criteria - 1] << "]" << endl;

        cout << "\n검색어를 입력하세요: ";
        getline(cin, query);

        if (query.empty()) continue;

        // 3. 검색 결과 수집 (results 벡터는 이전과 동일)
        vector<Question> results;
        for (const auto& Question : targetQuestions)
        {
            bool isFound = false;
            // criteria 변수(1:이름, 2:캐릭터, 3:키워드)를 사용하여 검색
            if (criteria == 1 && Question.nameKr.find(query) != string::npos) isFound = true;
            else if (criteria == 2 && Question.keyword.find(query) != string::npos) isFound = true;

            if (isFound) results.push_back(Question);
        }

        if (results.empty())
        {
            cout << "\n검색 결과가 없습니다." << endl;
            cout << "아무 키나 누르면 돌아갑니다.";
            _getch();
            continue;
        }

        // 4. 결과 화면 루프 (페이지 및 커서 관리)
        int currentPage = 1;
        int itemsPerPage = 10;
        int totalPages = (results.size() + itemsPerPage - 1) / itemsPerPage;

        int cursorIndex = 0;

        while (true)
        {
            screenClear();

            // [수정됨] 헤더에 검색 기준 포함
            // 예: 단어 : 문제 이름 [눈속임]
            cout << "단어 : " << criteriaNames[criteria - 1] << " [" << query << "]" << endl;
            cout << "--------------------------------" << endl;

            int startIndex = (currentPage - 1) * itemsPerPage;
            int currentCount = min(itemsPerPage, (int)results.size() - startIndex);

            for (int i = 0; i < itemsPerPage; i++)
            {
                if (i < currentCount)
                {
                    if (i == cursorIndex)
                        cout << "> ";
                    else
                        cout << "  ";

                    int realIndex = startIndex + i;
                    cout << (realIndex + 1) << ". " << results[realIndex].nameKr
                         << " (" << results[realIndex].character << ")" << endl;
                }
                else
                {
                    cout << endl;
                }
            }

            cout << "--------------------------------" << endl;
            cout << "      " << currentPage << " / " << totalPages << " 페이지" << endl;
            cout << "--------------------------------" << endl;
            cout << "↑↓:이동  Enter:상세정보  ←→:페이지  ESC:종료" << endl;

            int navKey = _getch();

            if (navKey == 224)
            {
                navKey = _getch();
                if (navKey == KEY_UP)
                {
                    if (cursorIndex > 0)
                        cursorIndex--;
                    else
                        cursorIndex = currentCount - 1;
                }
                else if (navKey == KEY_DOWN)
                {
                    if (cursorIndex < currentCount - 1)
                        cursorIndex++;
                    else
                        cursorIndex = 0;
                }
                else if (navKey == KEY_LEFT)
                {
                    if (currentPage > 1)
                    {
                        currentPage--;
                        cursorIndex = 0;
                    }
                }
                else if (navKey == KEY_RIGHT)
                {
                    if (currentPage < totalPages)
                    {
                        currentPage++;
                        cursorIndex = 0;
                    }
                }
            }
            else if (navKey == KEY_ENTER)
            {
                int selectedDataIndex = startIndex + cursorIndex;
                ShowQuestionDetail(results[selectedDataIndex]);
            }
            else if (navKey == KEY_ESC)
            {
                break;
            }
        }
    }
}

void PracticeQuestionSearch()
{
    SearchLogic(PracticeQuestions, "연습문제");
}

void ExamQuestionSearch()
{
    SearchLogic(ExamQuestions, "시험모드");
}

// [유지됨] 입력 코드 줄바꿈 적용
void PracticeQuestionMake()
{
    screenClear();
    cout << "=== 연습문제 문제 제작 ===" << endl;
    Question newQuestion;

    cout << "문제 이름(한글) 입력: ";
    getline(cin, newQuestion.nameKr);

    cout << "문제 이름(영문) 입력: ";
    getline(cin, newQuestion.nameEn);

    cout << "전승 캐릭터 입력: ";
    getline(cin, newQuestion.character);

    cout << "문제 설명 입력: ";
    getline(cin, newQuestion.desc);

    cout << "키워드 입력: ";
    getline(cin, newQuestion.keyword);

    cout << "\n저장하시겠습니까? (y/n): ";
    char confirm = _getch();
    if (confirm == 'y' || confirm == 'Y') {
        AddPracticeQuestion(newQuestion);
        cout << "\n성공적으로 추가되었습니다! 아무 키나 누르면 돌아갑니다." << endl;
    } else {
        cout << "\n취소되었습니다." << endl;
    }
    _getch();
}

// [유지됨] 입력 코드 줄바꿈 적용
void ExamQuestionMake()
{
    screenClear();
    cout << "=== 시험모드 문제 제작 ===" << endl;
    Question newQuestion;

    cout << "문제 이름(한글) 입력: ";
    getline(cin, newQuestion.nameKr);

    cout << "문제 이름(영문) 입력: ";
    getline(cin, newQuestion.nameEn);

    cout << "전승 캐릭터 입력: ";
    getline(cin, newQuestion.character);

    cout << "문제 설명 입력: ";
    getline(cin, newQuestion.desc);

    cout << "키워드 입력: ";
    getline(cin, newQuestion.keyword);

    cout << "\n저장하시겠습니까? (y/n): ";
    char confirm = _getch();
    if (confirm == 'y' || confirm == 'Y') {
        AddExamQuestion(newQuestion);
        cout << "\n성공적으로 추가되었습니다! 아무 키나 누르면 돌아갑니다." << endl;
    } else {
        cout << "\n취소되었습니다." << endl;
    }
    _getch();
}

