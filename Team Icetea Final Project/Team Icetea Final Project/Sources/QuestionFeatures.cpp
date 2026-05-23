#include "QuestionFeatures.h"
#include "QuestionData.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // min, max 사용
#include <ctime> // time() 사용
#include <cstdlib> // rand(), srand() 사용
#include <filesystem> // 파일 목록 탐색
#include <random>
#include <fstream>
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

// CSV의 한 줄을 파싱하는 함수 (사용자 정의 규칙 적용)
vector<string> ParseCSVLine(const string& firstLine, ifstream& file) {
    vector<string> result;
    string cell;
    bool inQuotes = false;
    string currentLine = firstLine;

    while (true) {
        for (size_t i = 0; i < currentLine.length(); ++i) {
            char c = currentLine[i];

            if (inQuotes) {
                if (c == '"') {
                    if (i + 1 < currentLine.length() && currentLine[i + 1] == '"') {
                        cell += '"';
                        i++;
                    }
                    else {
                        inQuotes = false; // 닫는 따옴표
                    }
                }
                else {
                    cell += c; // 큰따옴표 안의 문자는 줄바꿈 포함 그대로 저장
                }
            }
            else {
                if (c == '"' && cell.empty()) inQuotes = true;
                else if (c == ',') {
                    result.push_back(cell);
                    cell.clear();
                }
                else {
                    cell += c;
                }
            }
        }

        // 따옴표가 닫히지 않았는데 줄이 끝났다면, 파일에서 다음 줄을 더 읽어옴
        if (inQuotes && !file.eof()) {
            string nextLine;
            if (getline(file, nextLine)) {
                cell += "\n"; // 줄바꿈 문자 유지
                currentLine = nextLine;
                continue;
            }
        }
        break;
    }
    result.push_back(cell);
    return result;
}

// 폴더 내의 CSV 파일 목록을 탐색하여 과목 선택 메뉴 출력
// title 파라미터로 헤더 문구 변경 가능 (기본값: "학습할 과목을 선택하세요")
string SelectSubjectMenu(string title = "학습할 과목을 선택하세요") {
    vector<pair<string, string>> subjects; // 과목코드.이름, 전체경로

    // 현재 디렉토리에서 .csv 확장자 파일만 수집
    string folderPath = "./QuestionData";

    // 폴더가 없는 경우 알림
    if (!fs::exists(folderPath)) {
        screenClear();
        cout << "[오류] 폴더를 찾을 수 없습니다: " << folderPath << endl;
        cout << "상대 경로 확인이 필요합니다." << endl;
        _getch();
        return "";
    }

    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            // .csv 확장자 파일만 필터링
            if (entry.path().extension() == ".csv") {
                subjects.push_back({ entry.path().stem().string(), entry.path().string() });
            }
        }
    }
    catch (...) {
        cout << "폴더 탐색 중 오류 발생!" << endl;
        _getch();
        return "";
    }

    if (subjects.empty()) {
        cout << "사용 가능한 CSV 문제 파일이 없습니다." << endl;
        _getch();
        return "";
    }

    // 파일명 앞의 숫자를 기준으로 오름차순 정렬 (예: "1. Test1" → 1, "100. Test100" → 100)
    sort(subjects.begin(), subjects.end(), [](const pair<string, string>& a, const pair<string, string>& b) {
        int numA = 0, numB = 0;
        try { numA = stoi(a.first); } catch (...) {}
        try { numB = stoi(b.first); } catch (...) {}
        return numA < numB;
    });

    int focus = 0;
    int itemsPerPage = 10;
    int total = (int)subjects.size();

    while (true) {
        // 현재 포커스 위치에 따라 페이지 자동 계산
        int totalPages = (total + itemsPerPage - 1) / itemsPerPage;
        int currentPage = focus / itemsPerPage; // 0-indexed
        int startIdx = currentPage * itemsPerPage;
        int endIdx = min(startIdx + itemsPerPage, total);

        screenClear();
        cout << "=== " << title << " ===" << endl;
        cout << "--------------------------------" << endl;

        for (int i = startIdx; i < endIdx; i++) {
            if (i == focus) cout << "> " << subjects[i].first << endl;
            else            cout << "  " << subjects[i].first << endl;
        }

        cout << "--------------------------------" << endl;
        // 과목이 10개 초과일 때만 페이지 표시
        if (totalPages > 1) {
            cout << "        " << (currentPage + 1) << " / " << totalPages << " 페이지" << endl;
            cout << "--------------------------------" << endl;
            cout << "↑↓: 이동  ←→: 페이지  Enter: 선택  ESC: 취소" << endl;
        }
        else {
            cout << "↑↓: 이동, Enter: 선택, ESC: 취소" << endl;
        }

        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == KEY_UP) {
                // 현재 페이지 첫 항목보다 위로는 이동 안 함
                if (focus > startIdx) focus--;
            }
            else if (key == KEY_DOWN) {
                // 현재 페이지 마지막 항목보다 아래로는 이동 안 함
                if (focus < endIdx - 1) focus++;
            }
            else if (key == KEY_LEFT) {
                if (currentPage > 0)
                    focus = (currentPage - 1) * itemsPerPage;
            }
            else if (key == KEY_RIGHT) {
                if (currentPage < totalPages - 1)
                    focus = (currentPage + 1) * itemsPerPage;
            }
        }
        else if (key == KEY_ENTER) {
            return subjects[focus].second; // 선택된 파일 경로 반환
        }
        else if (key == KEY_ESC) {
            return "";
        }
    }
}

// CSV 파일에서 데이터를 로드하여 퀴즈 리스트를 만드는 공통 함수
vector<Question> LoadQuestionsFromCSV(const string& path) {
    vector<Question> list;
    ifstream file(path);

    if (!file.is_open()) return list;

    string line;
    getline(file, line); // 첫 줄(헤더) 건너뛰기

    while (getline(file, line)) {
        if (line.empty()) continue;

        vector<string> fields = ParseCSVLine(line, file);

        if (fields.size() >= 6) {
            Question q;
            q.desc = fields[1];      // 문제 내용
            q.nameKr = fields[2];    // 정답
            q.nameEn = fields[3];    // 오답1
            q.character = fields[4]; // 오답2 (기존 변수 재활용)
            q.keyword = fields[5];   // 오답3 (기존 변수 재활용)
            q.level = (fields.size() > 6) ? stoi(fields[6]) : 0;
            q.commentary = (fields.size() > 7) ? fields[7] : "";
            q.searchKeyword = (fields.size() > 8) ? fields[8] : ""; // 검색용 키워드
            list.push_back(q);
        }
    }
    file.close();
    return list;
}

// [유지됨] 세미콜론 줄바꿈 기능 제거 (원래대로 복구)
void ShowQuestionDetail(const Question& Question)
{
    screenClear();
    cout << "=== 문제 정보 ===" << endl;
    cout << "문제: " << Question.desc << endl;
    cout << "정답: " << Question.nameKr << endl;
    _getch();
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
        string criteriaNames[2] = { "과목", "키워드" };

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
    // 과목 선택 화면 표시
    string selectedFile = SelectSubjectMenu("검색할 과목을 선택하세요");
    if (selectedFile.empty()) return;

    // 선택된 CSV 파일에서 문제 로드
    vector<Question> questions = LoadQuestionsFromCSV(selectedFile);
    if (questions.empty()) {
        screenClear();
        cout << "문제 데이터가 없습니다." << endl;
        cout << "아무 키나 눌러주세요..." << endl;
        _getch();
        return;
    }

    // 과목명 추출 (파일 경로에서 파일명만)
    string subjectName = fs::path(selectedFile).stem().string();

    int focus = 0;
    int itemsPerPage = 10;
    int total = (int)questions.size();

    while (true) {
        int totalPages = (total + itemsPerPage - 1) / itemsPerPage;
        int currentPage = focus / itemsPerPage; // 0-indexed
        int startIdx = currentPage * itemsPerPage;
        int endIdx = min(startIdx + itemsPerPage, total);

        screenClear();
        cout << "=== " << subjectName << " 문제 목록 ===" << endl;
        cout << "  번호  난이도  키워드" << endl;
        cout << "--------------------------------" << endl;

        for (int i = startIdx; i < endIdx; i++) {
            string kw = questions[i].searchKeyword.empty() ? "-" : questions[i].searchKeyword;
            string line = to_string(i + 1) + ".  Lv." + to_string(questions[i].level) + "  " + kw;
            if (i == focus) cout << "> " << line << endl;
            else            cout << "  " << line << endl;
        }

        cout << "--------------------------------" << endl;
        if (totalPages > 1) {
            cout << "        " << (currentPage + 1) << " / " << totalPages << " 페이지" << endl;
            cout << "--------------------------------" << endl;
            cout << "↑↓: 이동  ←→: 페이지  Enter: 선택  ESC: 뒤로" << endl;
        }
        else {
            cout << "↑↓: 이동  Enter: 선택  ESC: 뒤로" << endl;
        }

        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == KEY_UP) {
                if (focus > startIdx) focus--;
            }
            else if (key == KEY_DOWN) {
                if (focus < endIdx - 1) focus++;
            }
            else if (key == KEY_LEFT) {
                if (currentPage > 0) focus = (currentPage - 1) * itemsPerPage;
            }
            else if (key == KEY_RIGHT) {
                if (currentPage < totalPages - 1) focus = (currentPage + 1) * itemsPerPage;
            }
        }
        else if (key == KEY_ENTER) {
            // 추후 수정 예정 - 문제 상세 보기
            screenClear();
            cout << "추후 수정 예정입니다." << endl;
            cout << "아무 키나 눌러주세요..." << endl;
            _getch();
        }
        else if (key == KEY_ESC) {
            return;
        }
    }
}

void ExamQuestionSearch()
{
    SearchLogic(ExamQuestions, "시험모드");
}

// 추후 수정 예정
void PracticeQuestionMake()
{
    screenClear();
    cout << "추후 수정 예정입니다." << endl;
    cout << "아무 키나 눌러주세요..." << endl;
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
    }
    else {
        cout << "\n취소되었습니다." << endl;
    }
    _getch();
}

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
    string selectedFile = SelectSubjectMenu();
    if (selectedFile == "") return;

    ifstream file(selectedFile);
    if (!file.is_open()) return;

    vector<Question> quizList;
    string line;
    getline(file, line); // 헤더 스킵

    while (getline(file, line)) {
        if (line.empty()) continue;
        vector<string> fields = ParseCSVLine(line, file);

        if (fields.size() >= 7) { // 최소 난이도 칸까지는 있어야 함
            Question q;
            q.desc = fields[1];
            q.nameKr = fields[2];    // 정답 (OX일 때 'O' 또는 'X')
            q.nameEn = fields[3];    // 오답 (OX일 때 'X' 또는 'O')
            q.character = (fields.size() > 4) ? fields[4] : "";
            q.keyword = (fields.size() > 5) ? fields[5] : "";

            // 난이도 저장 (문자열 "1"을 정수 1로 변환)
            q.level = stoi(fields[6]);

            if (fields.size() >= 8) {
                q.commentary = fields[7];
            }
            else {
                // 해설이 없으면 기본 문구 출력
                q.commentary = "[ 등록된 해설이 없습니다. ]";
            }
            q.searchKeyword = (fields.size() > 8) ? fields[8] : ""; // 검색용 키워드
            quizList.push_back(q);
        }
    }
    file.close();

    if (quizList.empty()) return;

    random_device rd;
    mt19937 g(rd());
    shuffle(quizList.begin(), quizList.end(), g);

    int score = 0;
    for (int i = 0; i < (int)quizList.size(); i++) {
        // 보기를 생성하고 섞음
        vector<string> options;
        int maxChoices = 0;

        if (quizList[i].level == 1) {
            // O가 위로, X가 아래로 가게 고정
            options = { "O", "X" };
            maxChoices = 2;
        }
        else {
            options = { quizList[i].nameKr, quizList[i].nameEn, quizList[i].character, quizList[i].keyword };
            random_device rd;
            mt19937 g(rd());
            shuffle(options.begin(), options.end(), g);
            maxChoices = 4;
        }

        int focus = 0; // 현재 어떤 보기를 가리키고 있는지
        bool answered = false;

        while (!answered) {
            screenClear();
            cout << "=== 문제 [" << i + 1 << " / " << quizList.size() << "] ===" << endl;
            cout << "----------------------------------------" << endl;
            cout << quizList[i].desc << endl;
            cout << "----------------------------------------" << endl;

            // 보기를 출력하며 현재 포커스 된 항목 앞에 '>' 표시
            for (int k = 0; k < maxChoices; k++) {
                if (k == focus) cout << "> " << k + 1 << ". " << options[k] << endl;
                else {
                    cout << "  " << k + 1 << ". " << options[k] << endl;
                }
            }
            
            cout << "----------------------------------------" << endl;
            cout << "↑↓: 이동  Enter: 결정  ESC: 학습 중단" << endl;

            int input = _getch();
            if (input == 224) { // 방향키 입력 처리
                input = _getch();
                if (input == KEY_UP) {
                    focus = (focus - 1 + maxChoices) % maxChoices;
                }
                else if (input == KEY_DOWN) {
                    focus = (focus + 1) % maxChoices;
                }
            }
            else if (input == KEY_ENTER) {
                answered = true;
                // Enter를 누르면 현재 focus에 있는 답이 정답인지 확인
                if (options[focus] == quizList[i].nameKr) {
                    cout << "\n[ 정답 ]" << endl;
                    score++;
                }
                else {
                    cout << "\n[ 오답 ]" << endl;
                    cout << "정답: " << quizList[i].nameKr << endl;
                }

                cout << "\n [ 문제 해설 ]" << endl;
                if (!quizList[i].commentary.empty()) {
                    cout << " " << quizList[i].commentary << endl;
                }
                else {
                    cout << "[ 등록된 해설이 없습니다. ]" << endl;
                }
                cout << "----------------------------------------" << endl;
                cout << "아무 키나 누르면 다음 문제로 이동합니다.";

                _getch(); // 결과 확인용 대기
                answered = true;
            } else if (input == KEY_ESC) {
                return; // 학습 종료
            }
        }
    }
    
FinalEnd:
    screenClear();
    cout << "==========================================" << endl;
    cout << "                                          " << endl;
    cout << "             연습을 완료했습니다!         " << endl;
    cout << "                                          " << endl;
    cout << "==========================================" << endl;
    cout << endl;
    cout << "      ESC를 누르면 메뉴로 돌아갑니다.     " << endl;
    cout << endl;
    cout << "==========================================" << endl;

    while (true) {
        int finalKey = _getch();
        if (finalKey == 27) break;
    }
}

// ─────────────────────────────────────────
// 시험모드
// ─────────────────────────────────────────
void ExamQuestionSolve()
{
    // 과목 선택
    string selectedFile = SelectSubjectMenu();
    if (selectedFile == "") return;

    ExamQuestions.clear();
    ifstream file(selectedFile);
    if (!file.is_open()) return;

    string line;
    getline(file, line); // 헤더 스킵

    while (getline(file, line)) {
        if (line.empty()) continue;
        vector<string> fields = ParseCSVLine(line, file);

        if (fields.size() >= 7) { // 최소 난이도 칸까지는 있어야 함
            Question q;
            q.desc = fields[1];
            q.nameKr = fields[2];    // 정답 (OX일 때 'O' 또는 'X')
            q.nameEn = fields[3];    // 오답 (OX일 때 'X' 또는 'O')
            q.character = (fields.size() > 4) ? fields[4] : "";
            q.keyword = (fields.size() > 5) ? fields[5] : "";

            // 난이도 저장 (문자열 "1"을 정수 1로 변환)
            q.level = stoi(fields[6]);

            if (fields.size() >= 8) q.commentary = fields[7];
            else {
                // 해설이 없으면 기본 문구 출력
                q.commentary = "[ 등록된 해설이 없습니다. ]";
            }
            q.searchKeyword = (fields.size() > 8) ? fields[8] : ""; // 검색용 키워드

            ExamQuestions.push_back(q);
        }
    }
    file.close();

    if (ExamQuestions.size() < 4) {
        cout << "\n[ 문제가 부족합니다. ]";
        _getch();
        return;
    }

    // 문제 수 설정 및 셔플
    int examCount = 0;
    while (true) {
        screenClear();
        cout << "=== 시험 모드: " << fs::path(selectedFile).stem().string() << " ===" << endl;
        cout << "풀 문제 수 입력 (4 ~ " << ExamQuestions.size() << "): ";
        string input; getline(cin, input);
        try {
            examCount = stoi(input);
            if (examCount >= 4 && examCount <= (int)ExamQuestions.size()) break;
        } catch (...) {}
        cout << "범위 내 숫자를 입력하세요.";
        _getch();
    }

    random_device rd; mt19937 g(rd());
    shuffle(ExamQuestions.begin(), ExamQuestions.end(), g);
    ExamQuestions.resize(examCount);

    int score = 0;
    vector<pair<int, string>> wrongRecord;
    time_t startTime = time(nullptr);

    for (int i = 0; i < (int)ExamQuestions.size(); i++) {
        vector<string> options;
        int maxChoices = 0;

        if (ExamQuestions[i].level == 1) {
            options = { "O", "X" };
            maxChoices = 2;
        }
        else {
            options = { ExamQuestions[i].nameKr, ExamQuestions[i].nameEn, ExamQuestions[i].character, ExamQuestions[i].keyword };
            shuffle(options.begin(), options.end(), g);
            maxChoices = 4;
        }

        int focus = 0;
        bool answered = false;

        while (!answered) {
            screenClear();
            cout << "=== 시험 [" << i + 1 << " / " << examCount << "] ===" << endl;
            cout << "\n " << ExamQuestions[i].desc << "\n" << endl;

            for (int k = 0; k < maxChoices; k++) {
                cout << (k == focus ? "> " : "  ") << k + 1 << ". " << options[k] << endl;
            }
            cout << "\n↑↓: 이동  Enter: 제출  ESC: 중단" << endl;

            int key = _getch();
            if (key == 224) {
                key = _getch();
                if (key == KEY_UP) {
                    focus = (focus - 1 + maxChoices) % maxChoices;
                }
                else if (key == KEY_DOWN) {
                    focus = (focus + 1) % maxChoices;
                }
            }
            else if (key == KEY_ENTER) {
                if (options[focus] == ExamQuestions[i].nameKr) {
                    score++;
                }
                else {
                    wrongRecord.push_back({ i, options[focus] });
                }
                answered = true; // 시험모드는 즉시 해설 없이 다음 문제로
            }
            else if (key == KEY_ESC) {
                return;
            }
        }
    }

    // 결과 및 오답 복습
    time_t elapsed = time(nullptr) - startTime;
    screenClear();
    cout << "=== 시험 완료 ===" << endl;
    cout << " 성적: " << score << " / " << examCount << endl;
    cout << " 시간: " << elapsed / 60 << "분 " << elapsed % 60 << "초" << endl;
    
    if (wrongRecord.empty()) {
        cout << "\n 만점입니다.";
        _getch();
        return;
    }
    cout << "\nENTER: 오답 복습 시작  ESC: 종료" << endl;
    if (_getch() == 27) {
        return;
    }

    for (auto& rec : wrongRecord) {
        int idx = rec.first;
        vector<string> options;
        int maxChoices = 0;

        if (ExamQuestions[idx].level == 1) {
            options = { "O", "X" };
            maxChoices = 2;
        }
        else {
            options = { ExamQuestions[idx].nameKr, ExamQuestions[idx].nameEn, ExamQuestions[idx].character, ExamQuestions[idx].keyword };
            shuffle(options.begin(), options.end(), g);
            maxChoices = 4;
        }

        int focus = 0;
        bool answered = false;
        bool isCorrect = false;

        while (true) {
            screenClear();
            cout << "=== 오답 복습 (시험 때 선택: " << rec.second << ") ===" << endl;
            cout << "\n " << ExamQuestions[idx].desc << "\n" << endl;

            for (int k = 0; k < maxChoices; k++) {
                if (answered) {
                    if (options[k] == ExamQuestions[idx].nameKr) cout << "O ";
                    else if (k == focus && !isCorrect) cout << "X ";
                    else cout << "  ";
                } else cout << (k == focus ? "> " : "  ");
                cout << k + 1 << ". " << options[k] << endl;
            }

            if (answered) {
                cout << "\n---------------------------------" << endl;
                cout << " [해설]\n " << ExamQuestions[idx].commentary << endl;
                cout << "---------------------------------" << endl;
                cout << "Enter: 다음 문제  ESC: 중단" << endl;
                int k = _getch();
                if (k == 13) break; else if (k == 27) return;
            }
            else {
                int k = _getch();
                if (k == 224) {
                    k = _getch();
                    if (k == 72) {
                        focus = (focus - 1 + maxChoices) % maxChoices;
                    }
                    else if (k == 80) {
                        focus = (focus + 1) % maxChoices;
                    }
                } else if (k == 13) {
                    answered = true;
                    isCorrect = (options[focus] == ExamQuestions[idx].nameKr);
                }
            }
        }
    }
FinalScore:
    // G. 최종 종료 화면
    screenClear();
    cout << "==========================================" << endl;
    cout << "                                          " << endl;
    cout << "             복습을 완료했습니다!         " << endl;
    cout << "                                          " << endl;
    cout << "==========================================" << endl;
    cout << endl;
    cout << "      ESC를 누르면 메뉴로 돌아갑니다.     " << endl;
    cout << endl;
    cout << "==========================================" << endl;

    // ESC 키가 입력될 때까지 대기
    while (true) {
        int finalKey = _getch();
        if (finalKey == 27) break;
    }
}

void PrintBoss()
{
    cout << endl;

    cout << "            /\\_____/\\\\ " << endl;
    cout << "           /  o   o  \\\\" << endl;
    cout << "          ( ==  ^  == )" << endl;
    cout << "           )         ( " << endl;
    cout << "          (           )" << endl;
    cout << "         ( (  )   (  ) )" << endl;
    cout << "        (__(__)___(__)__)" << endl;

    cout << endl;
    cout << "         [ BOSS MONSTER ]" << endl;
    cout << endl;
}

void PrintBossDamaged()
{
    cout << endl;

    cout << "            /\\_____/\\\\ " << endl;
    cout << "           /  x   x  \\\\" << endl;
    cout << "          ( ==  ^  == )" << endl;
    cout << "           )   ---   ( " << endl;
    cout << "          (    !!!    )" << endl;
    cout << "         ( (  )   (  ) )" << endl;
    cout << "        (__(__)___(__)__)" << endl;

    cout << endl;
    cout << "       [ BOSS HIT !!! ]" << endl;
    cout << endl;
}

void PrintAttackEffect(int damage)
{
    cout << endl;

    cout << "       플레이어의 공격!" << endl;
    cout << "==================================" << endl;
    cout << "            >>> BOOM! >>>         " << endl;
    cout << "==================================" << endl;

    cout << endl;
    cout << "보스에게 " << damage << " 데미지!" << endl;
    cout << endl;
}

void BossMonsterMode()
{
    string selectedFile = SelectSubjectMenu("보스전에 사용할 과목을 선택하세요");
    if (selectedFile == "") return;

    vector<Question> quizList = LoadQuestionsFromCSV(selectedFile);

    if (quizList.empty())
    {
        screenClear();
        cout << "문제 데이터가 없습니다." << endl;
        cout << "아무 키나 눌러주세요..." << endl;
        _getch();
        return;
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(quizList.begin(), quizList.end(), g);

    int bossHp = 100;
    int maxBossHp = 100;
    int score = 0;
    int combo = 0;
    int timeLimit = 120;

    time_t startTime = time(nullptr);

    for (int i = 0; i < (int)quizList.size(); i++)
    {
        if (bossHp <= 0) break;

        int elapsed = (int)(time(nullptr) - startTime);
        int remainTime = timeLimit - elapsed;

        if (remainTime <= 0) break;

        string userAnswer;

        screenClear();

        cout << "========== 몬스터 처치 모드 ==========" << endl;
        cout << "남은 시간: " << remainTime << "초" << endl;
        cout << "점수: " << score << endl;
        cout << "콤보: " << combo << endl;

        cout << "Boss HP: [";
        int hpBar = bossHp / 5;

        for (int j = 0; j < hpBar; j++) cout << "#";
        for (int j = hpBar; j < 20; j++) cout << "-";

        cout << "] " << bossHp << " / " << maxBossHp << endl;
        
        PrintBoss();

        cout << "--------------------------------------" << endl;
        cout << "문제 [" << i + 1 << " / " << quizList.size() << "]" << endl;
        cout << quizList[i].desc << endl;
        cout << "--------------------------------------" << endl;

        cout << "정답 입력: ";
        getline(cin, userAnswer);

        if (userAnswer == quizList[i].nameKr)
        {
            combo++;

            int damage = 1;

            if (combo >= 5)
                damage = 5;
            else if (combo >= 3)
                damage = 3;

            bossHp -= damage;
            if (bossHp < 0) bossHp = 0;

            score += damage * 10;
            score += combo * 2;

            cout << endl;
            
            screenClear();

            cout << "========== 몬스터 처치 모드 ==========" << endl;
            cout << "[정답!] 공격 성공!" << endl;
            cout << "보스에게 " << damage << " 데미지!" << endl;

            if (combo >= 3)
            {
                cout << "콤보 보너스 발동!" << endl;
            }

            PrintBossDamaged();

            cout << endl;
            cout << "아무 키나 누르면 다음 문제로 이동합니다..." << endl;
            _getch();
        }
        else
        {
            combo = 0;

            cout << endl;
            cout << "[오답] 공격 실패!" << endl;
            cout << "정답: " << quizList[i].nameKr << endl;
        }

        cout << endl;
    }

    screenClear();

    cout << "========== 게임 결과 ==========" << endl;

    if (bossHp <= 0)
        cout << "보스 처치 성공!" << endl;
    else
        cout << "보스 처치 실패!" << endl;

    cout << "최종 점수: " << score << endl;
    cout << "남은 보스 HP: " << bossHp << " / " << maxBossHp << endl;
    cout << "===============================" << endl;
    cout << "아무 키나 누르면 메뉴로 돌아갑니다..." << endl;

    _getch();
}