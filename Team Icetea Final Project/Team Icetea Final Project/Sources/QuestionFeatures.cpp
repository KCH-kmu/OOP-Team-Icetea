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
#include <set>
#include <conio.h>
#include <thread> // 타이밍 루프를 위해 추가
#include <chrono>
#include <iomanip> // setw 등 출력 정렬을 위해 추가

using namespace std;
namespace fs = std::filesystem;

// ===== CSV 이스케이프 헬퍼 (RFC 4180) =====
// 콤마, 큰따옴표, 줄바꿈이 포함된 필드를 "..."로 감싸는 함수
static string EscapeCSVField(const string& s) {
    bool needsQuote = (s.find(',')  != string::npos ||
                       s.find('"') != string::npos ||
                       s.find('\n') != string::npos ||
                       s.find('\r') != string::npos);
    if (!needsQuote) return s;
    string result = "\"";
    for (char c : s) {
        if (c == '"') result += "\"\"";
        else result += c;
    }
    result += "\"";
    return result;
}

// ===== 정렬/검색 보조 함수들 =====
// 영문 대문자만 소문자로 변환 (한글 2바이트 시퀀스는 건드리지 않음)
static string ToLowerCopy(const string& s) {
    string r = s;
    for (size_t i = 0; i < r.size(); i++) {
        unsigned char c = (unsigned char)r[i];
        if (c < 0x80) {
            r[i] = (char)tolower(c);
        }
    }
    return r;
}

// 대소문자 무시 부분일치 검사
static bool ContainsCI(const string& haystack, const string& needleLower) {
    if (needleLower.empty()) return true;
    string lowHay = ToLowerCopy(haystack);
    return lowHay.find(needleLower) != string::npos;
}

// 정렬 모드 enum 대용 상수
// 0: 번호 오름차순, 1: 번호 내림차순, 2: 레벨 오름차순, 3: 레벨 내림차순
// 정렬+검색 결과로 화면에 표시할 원본 인덱스 목록을 생성
static vector<int> BuildDisplayIndex(const vector<Question>& src, int sortMode, const string& searchLower) {
    vector<int> idx;
    idx.reserve(src.size());
    for (int i = 0; i < (int)src.size(); i++) {
        // 검색어가 없으면 모두 통과, 있으면 searchKeyword 필드에서 부분일치 확인
        if (searchLower.empty() || ContainsCI(src[i].searchKeyword, searchLower)) {
            idx.push_back(i);
        }
    }
    switch (sortMode) {
    case 0: // 번호 오름차순 (원본 인덱스 순)
        sort(idx.begin(), idx.end());
        break;
    case 1: // 번호 내림차순
        sort(idx.begin(), idx.end(), greater<int>());
        break;
    case 2: // 레벨 오름차순 (같은 레벨은 원본 번호 순 유지)
        stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
            return src[a].level < src[b].level;
            });
        break;
    case 3: // 레벨 내림차순
        stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
            return src[a].level > src[b].level;
            });
        break;
    }
    return idx;
}

// 키워드 검색 입력 화면 - 사용자가 입력한 검색어로 currentKeyword를 갱신
// 빈 문자열로 Enter 시 검색 해제
static void PromptSearchKeyword(string& currentKeyword) {
    screenClear();
    cout << "=== 키워드 검색 ===" << endl;
    cout << "----------------------------------------" << endl;
    cout << "검색어를 입력하세요." << endl;
    cout << "(빈 채로 Enter 시 검색이 해제됩니다)" << endl;
    cout << "----------------------------------------" << endl;
    if (!currentKeyword.empty()) {
        cout << "현재 적용된 검색어: \"" << currentKeyword << "\"" << endl;
    }
    cout << "검색어: ";

    string input;
    getline(cin, input);
    currentKeyword = input;
}

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
// ===== 새 과목 등록 화면 =====
// 사용 중인 번호를 스캔해 두 가지 추천 번호를 보여준 뒤 사용자가 직접 입력.
// 성공 시 새 CSV 파일 경로 반환, 취소 시 빈 문자열 반환.
static string PromptNewSubject() {
    string folderPath = "./QuestionData";

    // 사용 중인 과목 번호 수집
    set<int> usedNumbers;
    if (fs::exists(folderPath)) {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.path().extension() == ".csv") {
                try { usedNumbers.insert(stoi(entry.path().stem().string())); } catch (...) {}
            }
        }
    }

    // 추천 번호 계산
    int suggestSmall = 1;
    while (usedNumbers.count(suggestSmall)) suggestSmall++;
    int suggestNext = usedNumbers.empty() ? 1 : (*usedNumbers.rbegin() + 1);

    while (true) {
        screenClear();
        cout << "=== 새 과목 등록 ===" << endl;
        cout << "----------------------------------------" << endl;
        cout << "[추천 번호]" << endl;
        cout << "  - 가장 작은 빈 번호 : " << suggestSmall << endl;
        cout << "  - 마지막 번호 다음  : " << suggestNext  << endl;
        cout << "----------------------------------------" << endl;
        cout << "새 과목 번호 입력 (ESC=취소): ";

        // 숫자 직접 입력 (ESC/백스페이스 지원)
        string numStr;
        while (true) {
            int ch = _getch();
            if (ch == KEY_ESC) return "";
            if (ch == KEY_ENTER) break;
            if (ch == 8 && !numStr.empty()) {  // 백스페이스
                numStr.pop_back();
                cout << "\b \b";
            }
            else if (ch >= '0' && ch <= '9') {
                numStr += (char)ch;
                cout << (char)ch;
            }
        }
        cout << endl;

        if (numStr.empty()) continue;
        int newNum = 0;
        try { newNum = stoi(numStr); } catch (...) { continue; }
        if (newNum <= 0) continue;

        if (usedNumbers.count(newNum)) {
            cout << "[오류] " << newNum << "번은 이미 사용 중입니다. 다시 입력하세요." << endl;
            _getch();
            continue;
        }

        cout << "새 과목 이름 입력: ";
        string newName;
        getline(cin, newName);
        if (newName.empty()) continue;

        string newPath = folderPath + "/" + to_string(newNum) + ". " + newName + ".csv";

        screenClear();
        cout << "=== 새 과목 등록 ===" << endl;
        cout << "----------------------------------------" << endl;
        cout << "다음 파일을 생성합니다:" << endl;
        cout << "  " << newNum << ". " << newName << ".csv" << endl;
        cout << "----------------------------------------" << endl;
        cout << "생성하시겠습니까? (y/n): ";
        int confirm = _getch();
        cout << endl;

        if (confirm == 'y' || confirm == 'Y') {
            ofstream newFile(newPath);
            if (!newFile.is_open()) {
                cout << "[오류] 파일 생성 실패: " << newPath << endl;
                _getch();
                return "";
            }
            newFile << "Q.DataID,Q.Desc,R.Answer,W.Answer1,W.Answer2,W.Answer3,Difficulty,Explanation,Keyword" << endl;
            newFile.close();
            return newPath;
        }
        // n이면 다시 처음으로
    }
}

string SelectSubjectMenu(string title = "학습할 과목을 선택하세요", bool allowCreate = false) {
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
        }
        // allowCreate 여부에 따라 안내 문구 다르게 표시
        if (allowCreate) {
            cout << "↑↓:이동  ←→:페이지  Enter:선택  F:새 과목 등록  ESC:취소" << endl;
        }
        else if (totalPages > 1) {
            cout << "↑↓:이동  ←→:페이지  Enter:선택  ESC:취소" << endl;
        }
        else {
            cout << "↑↓:이동  Enter:선택  ESC:취소" << endl;
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
        else if ((key == 'f' || key == 'F') && allowCreate) {
            // 새 과목 등록 화면 진입
            string newPath = PromptNewSubject();
            if (!newPath.empty()) return newPath;
            // 취소 시 목록 재스캔 후 복귀
            subjects.clear();
            if (fs::exists(folderPath)) {
                for (const auto& entry : fs::directory_iterator(folderPath)) {
                    if (entry.path().extension() == ".csv") {
                        subjects.push_back({ entry.path().stem().string(), entry.path().string() });
                    }
                }
                sort(subjects.begin(), subjects.end(),
                    [](const pair<string,string>& a, const pair<string,string>& b) {
                        int na=0, nb=0;
                        try { na=stoi(a.first); } catch(...) {}
                        try { nb=stoi(b.first); } catch(...) {}
                        return na < nb;
                    });
            }
            total = (int)subjects.size();
            focus = 0;
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
            q.level = 0; try { if (fields.size() > 6) q.level = stoi(fields[6]); } catch (...) {}
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
            cout << "↑↓:이동  Enter:선택  ESC:뒤로 가기" << endl;

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

    // ===== 정렬/검색 상태 변수 =====
    int sortMode = 0;          // 0:번호↑, 1:번호↓, 2:레벨↑, 3:레벨↓
    string searchKeyword = ""; // 빈 문자열이면 검색 적용 안 함
    int focus = 0;             // 현재 커서가 가리키는 표시 인덱스 (displayIdx 기준)
    int itemsPerPage = 10;

    // 정렬 모드 라벨 (탭 표시에 사용)
    const string sortLabels[4] = { "번호↑", "번호↓", "난이도↑", "난이도↓" };

    while (true) {
        // 매 프레임 정렬/검색 결과 재계산
        vector<int> displayIdx = BuildDisplayIndex(questions, sortMode, ToLowerCopy(searchKeyword));
        int total = (int)displayIdx.size();

        // focus 범위 보정 (검색 후 결과가 줄어든 경우 등)
        if (focus >= total) focus = max(0, total - 1);
        if (focus < 0) focus = 0;

        int totalPages = max(1, (total + itemsPerPage - 1) / itemsPerPage);
        int currentPage = (total == 0) ? 0 : focus / itemsPerPage;
        int startIdx = currentPage * itemsPerPage;
        int endIdx = min(startIdx + itemsPerPage, total);

        screenClear();
        cout << "=== " << subjectName << " 문제 목록 ===" << endl;

        // 헤더: "번호"(4컬럼) + 4공백 + "난이도"(6컬럼) + 4공백 + "키워드"(6컬럼)
        // 데이터 컬럼과 위치를 맞추기 위해 한글 폭(2컬럼)을 고려해 배치
        cout << "  번호    난이도    키워드" << endl;
        cout << "----------------------------------------" << endl;

        if (total == 0) {
            // 검색 결과가 0개일 때 안내 (목록 영역 채워주기)
            cout << "  (검색 결과가 없습니다)" << endl;
            for (int blank = 1; blank < itemsPerPage; blank++) cout << endl;
        }
        else {
            for (int i = startIdx; i < endIdx; i++) {
                int origIdx = displayIdx[i];           // 원본 문제 인덱스
                const Question& q = questions[origIdx];
                string kw = q.searchKeyword.empty() ? "-" : q.searchKeyword;
                char focusChar = (i == focus) ? '>' : ' ';
                // 번호는 원본 문제 번호(원본 인덱스 + 1)를 표시
                // → 정렬 방향에 따라 번호 자체가 자연스럽게 1→N 또는 N→1로 나옴
                // setw(2)로 '.' 위치를 동일 컬럼에 맞춤 (예: " 1.", "10.")
                cout << focusChar << ' '
                    << setw(2) << (origIdx + 1) << ".     "
                    << "Lv." << q.level << "      "
                    << kw << endl;
            }
            // 마지막 페이지가 itemsPerPage보다 적으면 빈 줄로 채워 레이아웃 유지
            for (int blank = endIdx - startIdx; blank < itemsPerPage; blank++) cout << endl;
        }

        cout << "----------------------------------------" << endl;
        cout << "        " << (currentPage + 1) << " / " << totalPages << " 페이지" << endl;
        cout << "----------------------------------------" << endl;

        // ===== 정렬 탭(토글바) 출력 - 페이지 표시 아래로 이동 =====
        // 활성 탭은 [ ]로, 비활성 탭은 공백 2칸으로 감싸 동일 폭 유지
        for (int t = 0; t < 4; t++) {
            if (sortMode == t) cout << "[" << sortLabels[t] << "]";
            else               cout << " " << sortLabels[t] << " ";
            if (t < 3) cout << " ";
        }
        // 우측: 현재 키워드 검색 상태 표시
        cout << "    키워드 검색: ";
        if (searchKeyword.empty()) cout << "-";
        else                       cout << "\"" << searchKeyword << "\"";
        cout << endl;

        cout << "----------------------------------------" << endl;
        cout << "↑↓:이동  ←→:페이지  Tab:정렬 변경  F:키워드 검색  Enter:선택  ESC:뒤로" << endl;

        int key = _getch();
        if (key == 224) {
            // 화살표 키 (확장키)
            key = _getch();
            if (key == KEY_UP) {
                if (total > 0 && focus > startIdx) focus--;
            }
            else if (key == KEY_DOWN) {
                if (total > 0 && focus < endIdx - 1) focus++;
            }
            else if (key == KEY_LEFT) {
                if (currentPage > 0) focus = (currentPage - 1) * itemsPerPage;
            }
            else if (key == KEY_RIGHT) {
                if (currentPage < totalPages - 1) focus = (currentPage + 1) * itemsPerPage;
            }
        }
        else if (key == KEY_TAB) {
            // 정렬 모드 순환: 0 -> 1 -> 2 -> 3 -> 0
            sortMode = (sortMode + 1) % 4;
            focus = 0; // 정렬 바뀌면 첫 항목으로
        }
        else if (key == 'f' || key == 'F') {
            // 검색 화면 진입
            PromptSearchKeyword(searchKeyword);
            focus = 0; // 검색 적용 후 첫 항목으로
        }
        else if (key == KEY_ENTER) {
            if (total == 0) continue; // 결과 없으면 무시
            // 추후 수정 예정 - 문제 상세 보기
            // 현재는 선택된 원본 인덱스 정보만 알림 (디버깅용)
            screenClear();
            const Question& selected = questions[displayIdx[focus]];
            cout << "선택된 문제: Lv." << selected.level
                << "  키워드: " << (selected.searchKeyword.empty() ? "-" : selected.searchKeyword) << endl;
            cout << "(상세 보기 기능은 추후 수정 예정)" << endl;
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

// ===== 통합 문제 제작 함수 =====
// 연습/시험 공용 CSV에 새 문제를 추가한다.
// 난이도 1=OX, 2·3=4지선다. 연습·시험 모드는 풀이 화면에서 해설 표시 여부만 다름.
void MakeQuestion()
{
    // 1. 과목 선택 (F 키로 새 과목 등록 가능)
    string selectedFile = SelectSubjectMenu("문제를 추가할 과목을 선택하세요", true);
    if (selectedFile.empty()) return;

    string subjectName = fs::path(selectedFile).stem().string();

    // 2. 기존 CSV에서 최대 문제 번호 파악 → nextID = 최대값 + 1
    int nextID = 1;
    {
        ifstream f(selectedFile);
        string line;
        getline(f, line); // 헤더 스킵
        while (getline(f, line)) {
            if (line.empty()) continue;
            vector<string> fields = ParseCSVLine(line, f);
            if (!fields.empty()) {
                try { int id = stoi(fields[0]); if (id >= nextID) nextID = id + 1; }
                catch (...) {}
            }
        }
    }

    while (true) {
        // 3. 난이도 선택
        int difficulty = 0;
        {
            screenClear();
            cout << "=== 새 문제 입력 ===" << endl;
            cout << "과목      : " << subjectName << ".csv" << endl;
            cout << "문제 번호 : " << nextID << endl;
            cout << "----------------------------------------" << endl;
            string diffInput;
            cout << "난이도 선택 (1=OX  2=보통  3=어려움  Enter=취소): ";
            while (true) {
                getline(cin, diffInput);
                if (diffInput.empty())    return;
                if (diffInput == "1") { difficulty = 1; break; }
                if (diffInput == "2") { difficulty = 2; break; }
                if (diffInput == "3") { difficulty = 3; break; }
                cout << "  1, 2, 3 중 하나를 입력하세요: ";
            }
        }

        // 4. 내용 입력
        string desc, rAnswer, wAnswer1, wAnswer2, wAnswer3, keyword, explanation;

        screenClear();
        cout << "=== 새 문제 입력 ===" << endl;
        cout << "저장: " << subjectName << ".csv  (DataID: " << nextID << ")" << endl;
        cout << "난이도: Lv." << difficulty;
        if      (difficulty == 1) cout << " (OX)";
        else if (difficulty == 2) cout << " (4지선다 보통)";
        else                      cout << " (4지선다 어려움)";
        cout << endl;
        cout << "----------------------------------------" << endl;

        cout << "1) 문제 설명: ";
        getline(cin, desc);
        if (desc.empty()) {
            cout << "[취소] 설명이 비어 있습니다. Enter 키를 누르세요." << endl;
            { string dummy; getline(cin, dummy); } continue;
        }

        if (difficulty == 1) {
            // OX 입력 (Enter로 확정)
            string oxInput;
            cout << "2) 정답 (O 또는 X, Enter로 확정): ";
            while (true) {
                getline(cin, oxInput);
                if (oxInput.empty()) { rAnswer = ""; break; }  // 빈 입력 → 취소
                if (oxInput == "O" || oxInput == "o") { rAnswer = "O"; wAnswer1 = "X"; break; }
                if (oxInput == "X" || oxInput == "x") { rAnswer = "X"; wAnswer1 = "O"; break; }
                cout << "  O 또는 X만 입력하세요 (Enter로 확정): ";
            }
            if (rAnswer.empty()) continue;
            wAnswer2 = "";
            wAnswer3 = "";
        }
        else {
            // 4지선다 입력
            cout << "2) 정답  : "; getline(cin, rAnswer);
            if (rAnswer.empty()) { cout << "[취소] Enter 키를 누르세요." << endl; { string dummy; getline(cin, dummy); } continue; }
            cout << "3) 오답1 : "; getline(cin, wAnswer1);
            cout << "4) 오답2 : "; getline(cin, wAnswer2);
            cout << "5) 오답3 : "; getline(cin, wAnswer3);
        }

        int kwNum = (difficulty == 1) ? 3 : 6;
        cout << kwNum << ") 검색 키워드 (Enter=건너뜀): "; getline(cin, keyword);
        cout << kwNum + 1 << ") 해설        (Enter=건너뜀): "; getline(cin, explanation);

        // 5. 확인 화면
        screenClear();
        cout << "------------ 입력 확인 ------------" << endl;
        cout << "과목     : " << subjectName << ".csv  (DataID: " << nextID << ")" << endl;
        cout << "난이도   : Lv." << difficulty;
        if      (difficulty == 1) cout << " (OX)";
        else if (difficulty == 2) cout << " (보통)";
        else                      cout << " (어려움)";
        cout << endl;
        cout << "문제     : " << desc     << endl;
        cout << "정답     : " << rAnswer  << endl;
        if (difficulty == 1) {
            cout << "오답     : " << wAnswer1 << endl;
        } else {
            cout << "오답 1   : " << wAnswer1 << endl;
            cout << "오답 2   : " << wAnswer2 << endl;
            cout << "오답 3   : " << wAnswer3 << endl;
        }
        cout << "키워드   : " << (keyword.empty()     ? "-" : keyword)     << endl;
        cout << "해설     : " << (explanation.empty() ? "-" : explanation) << endl;
        cout << "-----------------------------------" << endl;
        string confirmInput;
        cout << "저장하시겠습니까? (y=저장  n=재입력): ";
        getline(cin, confirmInput);

        if (confirmInput.empty())                              return;
        if (confirmInput != "y" && confirmInput != "Y") continue; // n → 재입력

        // 6. CSV 파일 끝에 한 줄 추가
        {
            // 마지막 줄에 줄바꿈이 없으면 먼저 추가
            {
                fstream chk(selectedFile, ios::in | ios::out | ios::binary);
                if (chk.is_open()) {
                    chk.seekg(-1, ios::end);
                    char last; chk.get(last);
                    chk.close();
                    if (last != '\n') {
                        ofstream nl(selectedFile, ios::app | ios::binary);
                        nl << "\r\n";
                    }
                }
            }
            ofstream out(selectedFile, ios::app);
            if (!out.is_open()) {
                cout << "[오류] 파일 저장 실패! Enter 키를 누르세요." << endl;
                { string dummy; getline(cin, dummy); } continue;
            }
            out << EscapeCSVField(to_string(nextID)) << ","
                << EscapeCSVField(desc)        << ","
                << EscapeCSVField(rAnswer)     << ","
                << EscapeCSVField(wAnswer1)    << ","
                << EscapeCSVField(wAnswer2)    << ","
                << EscapeCSVField(wAnswer3)    << ","
                << difficulty                  << ","
                << EscapeCSVField(explanation) << ","
                << EscapeCSVField(keyword)     << "\r\n";
            out.close();
        }
        nextID++;

        // 7. 저장 완료 → 계속 입력 or 메뉴로
        screenClear();
        cout << "=== 저장 완료 ===" << endl;
        cout << "문제 번호 " << (nextID - 1) << " 문제를 " << subjectName << ".csv 에 저장했습니다." << endl;
        string contInput;
        cout << "계속 입력하시겠습니까? (y=계속  n/Enter=메뉴로): ";
        getline(cin, contInput);
        if (contInput != "y" && contInput != "Y") return;
    }
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
            q.level = 0; try { if (fields.size() > 6) q.level = stoi(fields[6]); } catch (...) {}

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
            cout << "↑↓:이동  Enter:결정  ESC:학습 중단" << endl;

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
    cout << "ESC:메뉴로 돌아가기" << endl;
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
            q.level = 0; try { if (fields.size() > 6) q.level = stoi(fields[6]); } catch (...) {}

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
            cout << "\n↑↓:이동  Enter:제출  ESC:중단" << endl;

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
    cout << "\nEnter:오답 복습 시작  ESC:종료" << endl;
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
                cout << "Enter:다음 문제  ESC:중단" << endl;
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
    cout << "ESC:메뉴로 돌아가기" << endl;
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
    screenClear();

    cout << "========== 몬스터 처치 모드 ==========" << endl;
    cout << endl;
    cout << "[게임 룰]" << endl;
    cout << "1. 문제를 맞히면 보스에게 데미지를 줍니다." << endl;
    cout << "2. 기본 데미지는 1입니다." << endl;
    cout << "3. 3콤보부터 데미지가 3으로 증가합니다." << endl;
    cout << "4. 5콤보부터 데미지가 5로 증가합니다." << endl;
    cout << "5. OX 문제, 객관식 문제, 주관식 문제가 나올 수 있습니다." << endl;
    cout << "6. 한 챕터의 문제를 모두 풀어도 보스 HP가 남아 있으면 다른 챕터를 선택해 계속 공격합니다." << endl;
    cout << "7. 제한 시간 안에 보스 HP를 0으로 만들면 승리합니다." << endl;
    cout << endl;
    cout << "보스 HP: 100" << endl;
    cout << "제한 시간: 120초" << endl;
    cout << endl;
    cout << "아무 키나 누르면 챕터 선택 화면으로 이동합니다..." << endl;

    _getch();

    random_device rd;
    mt19937 g(rd());

    int bossHp = 100;
    int maxBossHp = 100;
    int score = 0;
    int combo = 0;
    int timeLimit = 120;

    time_t startTime = time(nullptr);

    while (bossHp > 0)
    {
        int elapsed = (int)(time(nullptr) - startTime);
        int remainTime = timeLimit - elapsed;

        if (remainTime <= 0)
            break;

        string selectedFile = SelectSubjectMenu("공격할 챕터를 선택하세요");
        if (selectedFile == "")
            return;

        vector<Question> quizList = LoadQuestionsFromCSV(selectedFile);

        if (quizList.empty())
        {
            screenClear();
            cout << "문제 데이터가 없습니다." << endl;
            cout << "아무 키나 눌러주세요..." << endl;
            _getch();
            continue;
        }

        shuffle(quizList.begin(), quizList.end(), g);

        for (int i = 0; i < (int)quizList.size(); i++)
        {
            if (bossHp <= 0) break;

            elapsed = (int)(time(nullptr) - startTime);
            remainTime = timeLimit - elapsed;

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

            string correctAnswer = quizList[i].nameKr;

            correctAnswer.erase(remove(correctAnswer.begin(), correctAnswer.end(), ' '), correctAnswer.end());
            correctAnswer.erase(remove(correctAnswer.begin(), correctAnswer.end(), '\r'), correctAnswer.end());
            correctAnswer.erase(remove(correctAnswer.begin(), correctAnswer.end(), '\n'), correctAnswer.end());

            bool isCorrect = false;

            if (correctAnswer == "O" || correctAnswer == "X")
            {
                cout << "[ O / X 문제 ]" << endl;
                cout << "정답 입력(O/X): ";
                getline(cin, userAnswer);

                transform(userAnswer.begin(), userAnswer.end(), userAnswer.begin(), ::toupper);
                transform(correctAnswer.begin(), correctAnswer.end(), correctAnswer.begin(), ::toupper);

                isCorrect = (userAnswer == correctAnswer);
            }
            else if (!quizList[i].nameEn.empty() &&
                !quizList[i].character.empty() &&
                !quizList[i].keyword.empty())
            {
                cout << "[ 객관식 문제 ]" << endl;

                vector<string> options =
                {
                    quizList[i].nameKr,
                    quizList[i].nameEn,
                    quizList[i].character,
                    quizList[i].keyword
                };

                shuffle(options.begin(), options.end(), g);

                for (int k = 0; k < 4; k++)
                {
                    cout << k + 1 << ". " << options[k] << endl;
                }

                cout << endl;
                cout << "번호 입력: ";
                getline(cin, userAnswer);

                try
                {
                    int selected = stoi(userAnswer);

                    if (selected >= 1 && selected <= 4)
                    {
                        isCorrect = (options[selected - 1] == quizList[i].nameKr);
                    }
                }
                catch (...)
                {
                    isCorrect = false;
                }
            }
            else
            {
                cout << "[ 주관식 문제 ]" << endl;
                cout << "정답 입력: ";
                getline(cin, userAnswer);

                isCorrect = (userAnswer == quizList[i].nameKr);
            }

            if (isCorrect)
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
                cout << "아무 키나 누르면 다음 문제로 이동합니다..." << endl;
                _getch();
            }
        }

        if (bossHp > 0)
        {
            screenClear();
            cout << "이 챕터의 문제를 모두 풀었습니다." << endl;
            cout << "현재 보스 HP: " << bossHp << " / " << maxBossHp << endl;
            cout << "다른 챕터를 선택해서 계속 공격하세요." << endl;
            cout << "아무 키나 누르면 챕터 선택으로 돌아갑니다..." << endl;
            _getch();
        }
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

static void PrintTower(int floor, int lives, int combo, int score)
{
    string livesStr = "";
    for (int i = 0; i < lives; i++)  livesStr += " [O]";
    for (int i = lives; i < 3; i++)  livesStr += " [ ]";

    cout << "\n";
    for (int f = floor + 2; f >= floor - 2; f--) {
        if (f < 1) { cout << "  |             |\n"; continue; }
        if (f == floor)
            cout << "  |==[" << f << "F HERE]==|\n";
        else
            cout << "  |     " << f << "F      |\n";
    }
    cout << "  |_____________|\n";
    cout << "\n";
    cout << "  목숨:" << livesStr << "   콤보: x" << combo
        << "   점수: " << score << "\n";
    cout << "-----------------------------------------\n";
}

// -- 내부 헬퍼: 4지선다 보기 만들기 ------------------------------
static void BuildTowerChoices(const vector<Question>& pool, int qIdx,
    vector<string>& choices, int& answerSlot)
{
    const Question& cur = pool[qIdx];
    vector<string> wrongs;

    if (!cur.nameEn.empty())    wrongs.push_back(cur.nameEn);
    if (!cur.character.empty()) wrongs.push_back(cur.character);
    if (!cur.keyword.empty())   wrongs.push_back(cur.keyword);

    while ((int)wrongs.size() < 3) {
        int fallback = rand() % (int)pool.size();
        if (fallback != qIdx && pool[fallback].nameKr != cur.nameKr)
            wrongs.push_back(pool[fallback].nameKr);
    }

    answerSlot = rand() % 4;
    choices.clear();
    int wi = 0;
    for (int i = 0; i < 4; i++)
        choices.push_back((i == answerSlot) ? cur.nameKr : wrongs[wi++]);
}

// -- 난이도 단계 텍스트 -------------------------------------------
static string GetDifficultyLabel(int floor)
{
    if (floor < 10)  return "[1] 입문";
    if (floor < 20)  return "[2] 초급";
    if (floor < 30)  return "[3] 중급";
    if (floor < 40)  return "[4] 고급";
    return "[5] 전문가";
}

// -- 보스층 여부 --------------------------------------------------
static bool IsBossFloor(int floor)
{
    return floor > 0 && floor % 10 == 0;
}

// -- 최종 등급 텍스트 ---------------------------------------------
static string GetGrade(int floor)
{
    if (floor >= 50) return "  [ LEGEND  ] 당신은 탑의 정복자입니다!";
    if (floor >= 40) return "  [ MASTER  ] 놀라운 실력입니다!";
    if (floor >= 30) return "  [ GOLD    ] 뛰어난 도전자입니다!";
    if (floor >= 20) return "  [ SILVER  ] 훌륭한 도전이었습니다!";
    if (floor >= 10) return "  [ BRONZE  ] 좋은 시작이었습니다!";
    return "  [ BEGINNER] 다음엔 더 높이 올라가 보세요!";
}

// -- 공통 퀴즈 화면 출력 ------------------------------------------
static void PrintQuizScreen(int floor, int lives, int combo, int score,
    bool isBoss, int bossCorrect, int bossRequired,
    bool itemFloor, bool shieldUsed,
    const string& desc, const vector<string>& choices,
    int focusChoice)
{
    screenClear();

    if (isBoss) {
        string livesStr = "";
        for (int i = 0; i < lives; i++)  livesStr += "[O]";
        for (int i = lives; i < 3; i++)  livesStr += "[ ]";
        cout << "=========== BOSS " << floor << "층 ["
            << bossCorrect << "/" << bossRequired << "] ===========\n";
        cout << " 목숨: " << livesStr
            << "   콤보: x" << combo
            << "   점수: " << score << "\n";
    }
    else {
        PrintTower(floor, lives, combo, score);
    }

    cout << " 난이도: " << GetDifficultyLabel(floor) << "\n";
    if (itemFloor && !shieldUsed)
        cout << " [SHIELD] 실드 보유 중\n";
    cout << "-----------------------------------------\n";

    if (floor >= 30 && !isBoss)
        cout << " 문제: " << desc << "\n (힌트 없음 - 고난이도 구간)\n";
    else
        cout << " 문제: " << desc << "\n";

    cout << "-----------------------------------------\n";
    for (int k = 0; k < 4; k++) {
        cout << (k == focusChoice ? " > " : "   ")
            << k + 1 << ". " << choices[k] << "\n";
    }
    cout << "-----------------------------------------\n";
    cout << " ↑↓:이동  Enter:선택  ESC:게임 종료\n";
}

// =================================================================
// * 메인 함수 *
// =================================================================
void InfiniteTowerMode() {
    // 1. [순서 변경] 게임 시작 전 룰 설명부터 출력
    screenClear();
    cout << "\n";
    cout << "  +----------------------------------+\n";
    cout << "  |        ** 무한의 탑  ** |\n";
    cout << "  |                                  |\n";
    cout << "  |  * 목숨 3개로 시작합니다         |\n";
    cout << "  |  * 정답을 맞추면 층이 올라가요   |\n";
    cout << "  |  * 오답 -> 목숨 -1               |\n";
    cout << "  |  * 5콤보마다 목숨 +1 회복        |\n";
    cout << "  |  * 10층마다 보스전 등장!         |\n";
    cout << "  |  * 목숨이 다하면 게임 종료       |\n";
    cout << "  +----------------------------------+\n";
    cout << "\n  아무 키나 누르면 과목 선택 화면으로 이동합니다...\n";
    _getch();

    // 2. [그다음] 과목 선택 진행
    string selectedFile = SelectSubjectMenu("무한의 탑: 오를 과목을 선택하세요");
    if (selectedFile.empty()) return;

    vector<Question> quizList = LoadQuestionsFromCSV(selectedFile);
    if (quizList.size() < 4) {
        screenClear();
        cout << "  [오류] 문제가 4개 이상 필요합니다.\n";
        cout << "  아무 키나 눌러주세요...";
        _getch();
        return;
    }

    srand((unsigned)time(nullptr));

    // -- 게임 변수 초기화 -----------------------------------------
    int  lives = 3;
    int  maxLives = 5;
    int  floor = 0;
    int  combo = 0;
    int  score = 0;
    int  bestCombo = 0;
    int  correctCnt = 0;
    int  wrongCnt = 0;
    bool shieldUsed = false;

    // -- 메인 루프 ------------------------------------------------
    while (lives > 0) {
        floor++;
        bool boss = IsBossFloor(floor);

        // +-------------------------------------------------------+
        // | 보스층 진입 연출                                       |
        // +-------------------------------------------------------+
        if (boss) {
            screenClear();
            cout << "\n";
            cout << "  +======================================+\n";
            cout << "  |  !! " << floor << "층  BOSS FLOOR 등장!  !!    |\n";
            cout << "  |  4지선다 3문제 연속 정답으로 통과!  |\n";
            cout << "  |  틀리면 처음부터 + 목숨 -1          |\n";
            cout << "  +======================================+\n";
            cout << "\n";
            cout << "             ,~~.             " << endl;
            cout << "            /     \\           " << endl;
            cout << "           (  0 0  )          " << endl;
            cout << "          /   \\^/   \\         " << endl;
            cout << "         / |       | \\        " << endl;
            cout << "        /  |       |  \\       " << endl;
            cout << "       (   |       |   )      " << endl;
            cout << "        \\  \\_______/  /       " << endl;
            cout << "         `--_______--'        " << endl;
            cout << "\n        [ 심연의 군주 ]\n";
            cout << "\n   아무 키나 누르면 시작...\n";
            _getch();
        }

        // +-------------------------------------------------------+
        // | 보스층: 4지선다 3문제 연속 정답                        |
        // +-------------------------------------------------------+
        if (boss) {
            const int bossRequired = 3;
            int bossCorrect = 0;
            shieldUsed = false;

            while (bossCorrect < bossRequired && lives > 0) {
                int idx = rand() % (int)quizList.size();
                vector<string> choices;
                int answerSlot = 0;
                BuildTowerChoices(quizList, idx, choices, answerSlot);

                int  focusChoice = 0;
                bool answered = false;

                while (!answered) {
                    PrintQuizScreen(floor, lives, combo, score,
                        true, bossCorrect, bossRequired,
                        false, false,
                        quizList[idx].desc, choices, focusChoice);

                    int key = _getch();
                    if (key == 224) {
                        key = _getch();
                        if (key == KEY_UP)
                            focusChoice = (focusChoice - 1 + 4) % 4;
                        else if (key == KEY_DOWN)
                            focusChoice = (focusChoice + 1) % 4;
                    }
                    else if (key == KEY_ENTER) {
                        answered = true;
                        if (focusChoice == answerSlot) {
                            bossCorrect++;
                            combo++;
                            if (combo > bestCombo) bestCombo = combo;
                            score += 200 + (combo * 50);
                            correctCnt++;
                            screenClear();
                            cout << "\n  [O] 정답! BOSS ["
                                << bossCorrect << "/" << bossRequired << "]\n";
                            cout << "  아무 키나 누르면 계속...\n";
                            _getch();
                        }
                        else {
                            lives--;
                            combo = 0;
                            bossCorrect = 0;
                            wrongCnt++;
                            screenClear();
                            cout << "\n  [X] 오답! 정답: "
                                << quizList[idx].nameKr
                                << "\n  목숨 -1  보스전 처음부터!\n";
                            cout << "  아무 키나 누르면 계속...\n";
                            _getch();
                        }
                    }
                    else if (key == KEY_ESC) {
                        goto GameOver;
                    }
                }
            } // while bossCorrect

            // 보스 클리어 보상
            if (lives > 0) {
                int oldLives = lives;
                lives = min(lives + 1, maxLives);
                score += 500;
                screenClear();
                cout << "\n  [CLEAR!] BOSS 클리어!  목숨 +" << (lives - oldLives)
                    << " 보상!  +500점!\n";
                cout << "  현재 목숨: ";
                for (int i = 0; i < lives; i++) cout << "[O]";
                cout << "\n\n  아무 키나 누르면 계속...\n";
                _getch();
            }
            continue;
        } // if (boss)

        // +-------------------------------------------------------+
        // | 일반 층: 4지선다 1문제                                  |
        // +-------------------------------------------------------+
        {
            // 5의 배수 층 = 아이템 층
            bool itemFloor = (floor % 5 == 0);
            if (itemFloor) {
                screenClear();
                cout << "\n";
                cout << "  +==============================+\n";
                cout << "  |  [ITEM] " << floor << "층  아이템 층!     |\n";
                cout << "  |  이번 층은 실드가 지급됩니다 |\n";
                cout << "  |  (오답 1회 목숨 보호!)       |\n";
                cout << "  +==============================+\n";
                cout << "\n  아무 키나 누르면 계속...\n";
                shieldUsed = false;
                _getch();
            }

            int idx = rand() % (int)quizList.size();
            vector<string> choices;
            int  answerSlot = 0;
            int  focusChoice = 0;
            bool answered = false;
            BuildTowerChoices(quizList, idx, choices, answerSlot);

            while (!answered) {
                PrintQuizScreen(floor, lives, combo, score,
                    false, 0, 0,
                    itemFloor, shieldUsed,
                    quizList[idx].desc, choices, focusChoice);

                int key = _getch();
                if (key == 224) {
                    key = _getch();
                    if (key == KEY_UP)
                        focusChoice = (focusChoice - 1 + 4) % 4;
                    else if (key == KEY_DOWN)
                        focusChoice = (focusChoice + 1) % 4;
                }
                else if (key == KEY_ENTER) {
                    answered = true;
                    if (focusChoice == answerSlot) {
                        // -- 정답 처리 --------------------------
                        combo++;
                        if (combo > bestCombo) bestCombo = combo;
                        int gain = 100 + (combo * 20);
                        if (floor >= 30) gain = (int)(gain * 1.5);
                        if (floor >= 40) gain = (int)(gain * 2.0);
                        score += gain;
                        correctCnt++;

                        screenClear();
                        cout << "\n  [O] 정답!  +" << gain << "점";
                        if (combo >= 3)
                            cout << "  ** " << combo << " COMBO! **";
                        cout << "\n";

                        // 5콤보 목숨 회복
                        if (combo % 5 == 0) {
                            int oldLives = lives;
                            lives = min(lives + 1, maxLives);
                            if (lives > oldLives)
                                cout << "  [+1] " << combo << "콤보 달성! 목숨 +1!\n";
                        }
                        cout << "\n  아무 키나 누르면 계속...\n";
                        _getch();
                    }
                    else {
                        // -- 오답 처리 --------------------------
                        if (itemFloor && !shieldUsed) {
                            shieldUsed = true;
                            combo = 0;
                            wrongCnt++;
                            screenClear();
                            cout << "\n  [X] 오답!  [SHIELD] 실드 발동 -> 목숨 보호!\n";
                            cout << "  정답: " << quizList[idx].nameKr << "\n";
                        }
                        else {
                            lives--;
                            combo = 0;
                            wrongCnt++;
                            screenClear();
                            cout << "\n  [X] 오답! 정답: "
                                << quizList[idx].nameKr << "  목숨 -1\n";
                        }
                        cout << "\n  아무 키나 누르면 계속...\n";
                        _getch();
                    }
                }
                else if (key == KEY_ESC) {
                    goto GameOver;
                }
            }
        } // 일반층 블록
    } // while lives > 0

    // =============================================================
    // * GAME OVER 화면 *
    // =============================================================
GameOver:
    floor--;
    if (floor < 0) floor = 0;

    screenClear();
    cout << "\n";
    cout << "  +====================================+\n";
    cout << "  |           ** GAME OVER **          |\n";
    cout << "  +====================================+\n";
    cout << "  |  도달한 층  :  " << floor << " 층\n";
    cout << "  |  최종 점수  : " << score << " 점\n";
    cout << "  |  정답 수    : " << correctCnt << " 문제\n";
    cout << "  |  오답 수    : " << wrongCnt << " 문제\n";
    cout << "  |  최고 콤보  : x" << bestCombo << "\n";
    cout << "  +====================================+\n";
    cout << GetGrade(floor) << "\n";
    cout << "  +====================================+\n";

    int total = correctCnt + wrongCnt;
    if (total > 0) {
        int pct = (correctCnt * 100) / total;
        cout << "\n  정답률: " << pct << "%\n";
    }

    cout << "\n  아무 키나 누르면 메뉴로 돌아갑니다...\n";
    _getch();
}