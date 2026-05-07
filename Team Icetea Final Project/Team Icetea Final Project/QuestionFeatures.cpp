#include "QuestionFeatures.h"
#include "QuestionData.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // min, max 사용
#include <ctime> // NEW  time() 사용
#include <cstdlib> // NEW  rand(), srand() 사용

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


// NEW

// PracticeQuestionSolve와 ExamQuestionSolve에서 공통으로 사용하는
// 4지선다 보기 구성 함수 (중복 코드 제거 목적)
void BuildChoices(const vector<Question>& pool, int questionIndex,
    vector<string>& choices, int& answerSlot)
{
    const Question& cur = pool[questionIndex];

    // 오답 3개 수집
    // wrongChoices에 값이 있으면 사용, 없으면 다른 문제 정답으로 임시 대체
    vector<string> wrongPool;
    for (int i = 0; i < 3; i++)
    {
        if (i < (int)cur.wrongChoices.size() && !cur.wrongChoices[i].empty())
        {
            wrongPool.push_back(cur.wrongChoices[i]);
        }
        else
        {
            // 자기 자신이 오답으로 뽑히지 않도록 do-while로 걸러냄
            int fallback;
            do { fallback = rand() % (int)pool.size(); } while (fallback == questionIndex);
            wrongPool.push_back(pool[fallback].nameKr);
        }
    }

    // 정답이 항상 같은 위치에 오지 않도록 위치를 랜덤으로 결정
    answerSlot = rand() % 4;
    choices.clear();
    int wrongUsed = 0;
    for (int i = 0; i < 4; i++)
        choices.push_back(i == answerSlot ? cur.nameKr : wrongPool[wrongUsed++]);
}

// ─────────────────────────────────────────
// 연습문제 풀기
// ─────────────────────────────────────────
void PracticeQuestionSolve()
{
    // 4지선다 구성을 위해 최소 4개 필요
    if ((int)PracticeQuestions.size() < 4)
    {
        screenClear();
        cout << "문제가 4개 이상 필요합니다. (현재: " << PracticeQuestions.size() << "개)" << endl;
        cout << "아무 키나 누르면 돌아갑니다.";
        _getch();
        return;
    }

    // 매 실행마다 다른 순서로 출제되도록 랜덤 시드 설정
    srand((unsigned int)time(nullptr));

    // 피셔-예이츠 셔플: 문제 인덱스 배열을 랜덤하게 섞음
    vector<int> order;
    for (int i = 0; i < (int)PracticeQuestions.size(); i++) order.push_back(i);
    for (int i = (int)order.size() - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        swap(order[i], order[j]);
    }

    int correct = 0;
    int total = (int)order.size();

    for (int q = 0; q < total; q++)
    {
        const Question& cur = PracticeQuestions[order[q]];

        // 보기 4개와 정답 위치 구성
        vector<string> choices;
        int answerSlot;
        BuildChoices(PracticeQuestions, order[q], choices, answerSlot);

        int  cursorPos = 0;
        bool answered = false; // 답을 선택했는지 여부
        bool isCorrect = false;

        while (true)
        {
            screenClear();
            cout << "=========== 연습문제 풀기 ===========" << endl;
            cout << "(" << (q + 1) << " / " << total << ")  맞은 개수: " << correct << endl;
            cout << "=====================================" << endl;
            cout << " Q. " << cur.nameEn << endl;
            cout << "-------------------------------------" << endl;

            for (int i = 0; i < 4; i++)
            {
                if (answered)
                {
                    // 답 선택 후: 정답/오답 위치 표시
                    if (i == answerSlot)
                        cout << "  O " << (i + 1) << ". " << choices[i] << "  <- 정답" << endl;
                    else if (i == cursorPos && !isCorrect)
                        cout << "  X " << (i + 1) << ". " << choices[i] << "  <- 내 선택" << endl;
                    else
                        cout << "    " << (i + 1) << ". " << choices[i] << endl;
                }
                else
                {
                    // 답 선택 전: 커서 위치만 표시
                    cout << (i == cursorPos ? "  > " : "    ");
                    cout << (i + 1) << ". " << choices[i] << endl;
                }
            }

            cout << "--------------------------------" << endl;

            if (answered)
            {
                // 연습모드: 답 선택 즉시 풀이 표시
                cout << (isCorrect ? " O 정답입니다!" : " X 오답입니다!") << endl;
                cout << endl;
                cout << " [정답] " << cur.nameKr << endl;
                cout << " [해설] " << (cur.desc.empty() ? "(해설 없음)" : cur.desc) << endl;
                cout << "--------------------------------" << endl;
                cout << "Enter: 다음 문제  ESC: 메인으로" << endl;

                int key = _getch();
                if (key == KEY_ENTER)    break;
                else if (key == KEY_ESC) goto EndPractice; // 이중 루프 탈출
            }
            else
            {
                cout << "↑↓: 이동  Enter: 선택  ESC: 메인으로" << endl;

                int key = _getch();
                if (key == 224)
                {
                    key = _getch();
                    if (key == KEY_UP) { cursorPos--; if (cursorPos < 0) cursorPos = 3; }
                    if (key == KEY_DOWN) { cursorPos++; if (cursorPos > 3) cursorPos = 0; }
                }
                else if (key == KEY_ENTER)
                {
                    answered = true;
                    isCorrect = (cursorPos == answerSlot);
                    if (isCorrect) correct++;
                }
                else if (key == KEY_ESC) goto EndPractice;
            }
        }
    }

    EndPractice:
    screenClear();
    cout << "=========== 풀이 완료 ===========" << endl;
    cout << "---------------------------------" << endl;
    cout << " 맞은 개수 : " << correct << " / " << total << endl;
    cout << " 정답률    : " << (total > 0 ? correct * 100 / total : 0) << "%" << endl;
    cout << "---------------------------------" << endl;
    cout << "아무 키나 누르면 메인으로 돌아갑니다.";
    _getch();
}

// ─────────────────────────────────────────
// 시험모드
// ─────────────────────────────────────────
void ExamQuestionSolve()
{
    if ((int)ExamQuestions.size() < 4)
    {
        screenClear();
        cout << "문제가 4개 이상 필요합니다. (현재: " << ExamQuestions.size() << "개)" << endl;
        cout << "아무 키나 누르면 돌아갑니다.";
        _getch();
        return;
    }

    // 1. 문제 수 입력 (4 ~ 전체 문제 수 범위 검증)
    int examCount = 0;
    while (true)
    {
        screenClear();
        cout << "=========== 시험모드 ===========" << endl;
        cout << "--------------------------------" << endl;
        cout << "총 문제 수: " << ExamQuestions.size() << "개" << endl;
        cout << "풀 문제 수를 입력하세요 (4 ~ " << ExamQuestions.size() << "): ";

        string input;
        getline(cin, input);

        // 숫자인지 검증
        bool isNumber = !input.empty();
        for (char c : input) if (!isdigit(c)) { isNumber = false; break; }

        if (isNumber)
        {
            examCount = stoi(input);
            if (examCount >= 4 && examCount <= (int)ExamQuestions.size()) break;
        }
        cout << "올바른 숫자를 입력하세요." << endl;
        _getch();
    }

    // 2. 셔플 후 설정한 문제 수만큼만 추출
    srand((unsigned int)time(nullptr));
    vector<int> order;
    for (int i = 0; i < (int)ExamQuestions.size(); i++) order.push_back(i);
    for (int i = (int)order.size() - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        swap(order[i], order[j]);
    }
    order.resize(examCount);

    int correct = 0;
    int total = examCount;

    // 오답 기록: {문제 인덱스, 내가 선택한 보기 텍스트}
    vector<pair<int, string>> wrongRecord;

    // 3. 시험 시작 시각 기록 (종료 후 소요 시간 계산에 사용)
    time_t startTime = time(nullptr);

    // 4. 문제 풀이 루프 (시험모드: 답 선택 후 풀이 즉시 표시 X)
    for (int q = 0; q < total; q++)
    {
        const Question& cur = ExamQuestions[order[q]];

        vector<string> choices;
        int answerSlot;
        BuildChoices(ExamQuestions, order[q], choices, answerSlot);

        int  cursorPos = 0;
        bool selected = false;

        while (!selected)
        {
            screenClear();
            cout << "=========== 시험모드 ===========" << endl;
            cout << "(" << (q + 1) << " / " << total << ")" << endl;
            cout << "================================" << endl;
            cout << " Q. " << cur.nameEn << endl;
            cout << "--------------------------------" << endl;

            for (int i = 0; i < 4; i++)
            {
                cout << (i == cursorPos ? "  > " : "    ");
                cout << (i + 1) << ". " << choices[i] << endl;
            }

            cout << "--------------------------------" << endl;
            cout << "↑↓: 이동  Enter: 선택  ESC: 시험 종료" << endl;

            int key = _getch();
            if (key == 224)
            {
                key = _getch();
                if (key == KEY_UP) { cursorPos--; if (cursorPos < 0) cursorPos = 3; }
                if (key == KEY_DOWN) { cursorPos++; if (cursorPos > 3) cursorPos = 0; }
            }
            else if (key == KEY_ENTER)
            {
                if (cursorPos == answerSlot)
                    correct++;
                else
                    // 오답이면 문제 인덱스와 선택한 보기를 기록
                    wrongRecord.push_back({ order[q], choices[cursorPos] });
                selected = true;
            }
            else if (key == KEY_ESC)
            {
                // 실수로 누르는 경우를 대비해 종료 확인
                screenClear();
                cout << "시험을 종료하시겠습니까? (y/n): ";
                char confirm = _getch();
                if (confirm == 'y' || confirm == 'Y') goto EndExam;
            }
        }
    }

    EndExam:
    // 5. 소요 시간 계산
    time_t endTime = time(nullptr);
    int elapsed = (int)(endTime - startTime);
    int elapsedMin = elapsed / 60;
    int elapsedSec = elapsed % 60;

    // 6. 시험 결과 화면
    screenClear();
    cout << "=========== 시험 완료 ===========" << endl;
    cout << "---------------------------------" << endl;
    cout << " 맞은 개수 : " << correct << " / " << total << endl;
    cout << " 정답률    : " << (total > 0 ? correct * 100 / total : 0) << "%" << endl;
    cout << " 소요 시간 : " << elapsedMin << "분 " << elapsedSec << "초" << endl;
    cout << "---------------------------------" << endl;

    if (wrongRecord.empty())
    {
        cout << "\n오답이 없습니다. 완벽합니다!" << endl;
        cout << "아무 키나 누르면 메인으로 돌아갑니다.";
        _getch();
        return;
    }

    cout << "\n오답 문제 수 : " << wrongRecord.size() << "개" << endl;
    cout << "\nEnter: 오답 복습 시작  ESC: 종료" << endl;

    {
        int resultKey = _getch();
        if (resultKey == KEY_ESC) return;
    }

    // 7. 오답 복습 루프 (연습모드처럼 즉시 풀이 표시)
    int reviewCorrect = 0;
    int reviewTotal = (int)wrongRecord.size();

    for (int r = 0; r < reviewTotal; r++)
    {
        const Question& cur = ExamQuestions[wrongRecord[r].first];
        const string& myWrong = wrongRecord[r].second; // 시험 때 내가 선택했던 오답

        vector<string> choices;
        int answerSlot;
        BuildChoices(ExamQuestions, wrongRecord[r].first, choices, answerSlot);

        int  cursorPos = 0;
        bool answered = false;
        bool isCorrect = false;

        while (true)
        {
            screenClear();
            cout << "=========== 오답 복습 ===========" << endl;
            cout << "(" << (r + 1) << " / " << reviewTotal << ")  맞은 개수: " << reviewCorrect << endl;
            // 시험 때 틀린 답을 상단에 표시해 비교할 수 있게 함
            cout << " [시험 때 내 선택]: " << myWrong << endl;
            cout << "=================================" << endl;
            cout << " Q. " << cur.nameEn << endl;
            cout << "---------------------------------" << endl;

            for (int i = 0; i < 4; i++)
            {
                if (answered)
                {
                    if (i == answerSlot)
                        cout << "  O " << (i + 1) << ". " << choices[i] << "  <- 정답" << endl;
                    else if (i == cursorPos && !isCorrect)
                        cout << "  X " << (i + 1) << ". " << choices[i] << "  <- 내 선택" << endl;
                    else
                        cout << "    " << (i + 1) << ". " << choices[i] << endl;
                }
                else
                {
                    cout << (i == cursorPos ? "  > " : "    ");
                    cout << (i + 1) << ". " << choices[i] << endl;
                }
            }

            cout << "--------------------------------" << endl;

            if (answered)
            {
                cout << (isCorrect ? " O 정답입니다!" : " X 오답입니다!") << endl;
                cout << endl;
                cout << " [정답] " << cur.nameKr << endl;
                cout << " [해설] "
                    << (cur.desc.empty() ? "(해설 없음)" : cur.desc) << endl;
                cout << "--------------------------------" << endl;
                cout << "Enter: 다음 문제  ESC: 종료" << endl;

                int key = _getch();
                if (key == KEY_ENTER)    break;
                else if (key == KEY_ESC) goto EndReview;
            }
            else
            {
                cout << "↑↓: 이동  Enter: 선택  ESC: 종료" << endl;

                int key = _getch();
                if (key == 224)
                {
                    key = _getch();
                    if (key == KEY_UP) { cursorPos--; if (cursorPos < 0) cursorPos = 3; }
                    if (key == KEY_DOWN) { cursorPos++; if (cursorPos > 3) cursorPos = 0; }
                }
                else if (key == KEY_ENTER)
                {
                    answered = true;
                    isCorrect = (cursorPos == answerSlot);
                    if (isCorrect) reviewCorrect++;
                }
                else if (key == KEY_ESC) goto EndReview;
            }
        }
    }

    EndReview:
    // 8. 시험 + 복습 결과를 함께 표시
    screenClear();
    cout << "=========== 복습 완료 ===========" << endl;
    cout << "---------------------------------" << endl;
    cout << " [시험 결과]" << endl;
    cout << "  맞은 개수 : " << correct << " / " << total << endl;
    cout << "  정답률    : " << (total > 0 ? correct * 100 / total : 0) << "%" << endl;
    cout << "  소요 시간 : " << elapsedMin << "분 " << elapsedSec << "초" << endl;
    cout << "---------------------------------" << endl;
    cout << " [복습 결과]" << endl;
    cout << "  맞은 개수 : " << reviewCorrect << " / " << reviewTotal << endl;
    cout << "  정답률    : "
        << (reviewTotal > 0 ? reviewCorrect * 100 / reviewTotal : 0) << "%" << endl;
    cout << "---------------------------------" << endl;
    cout << "아무 키나 누르면 메인으로 돌아갑니다.";
    _getch();
}