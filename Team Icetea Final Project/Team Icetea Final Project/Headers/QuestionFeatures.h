// QuestionFeatures.h
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "QuestionData.h"

using namespace std;

// ===== CSV 입출력 유틸리티 클래스 =====
class CSVUtil {
public:
    // CSV 필드 이스케이프 (RFC 4180)
    static string EscapeField(const string& s);
    // CSV 한 줄 파싱 (따옴표 내 줄바꿈 지원)
    static vector<string> ParseLine(const string& firstLine, ifstream& file);
    // CSV 파일에서 문제 목록 로드
    static vector<Question> LoadQuestions(const string& path);
};

// ===== 과목(CSV) 선택 메뉴 =====
class SubjectSelector {
public:
    // 과목 선택 메뉴 표시 → 선택된 CSV 경로 반환
    static string Select(string title = "학습할 과목을 선택하세요", bool allowCreate = false);
private:
    // 새 과목(CSV) 등록 화면
    static string PromptNewSubject();
};

// ===== 문제 검색 기능 =====
class QuestionSearch {
public:
    void RunPractice(); // 연습문제 검색
    void RunExam();     // 시험모드 문제 검색
private:
    void SearchLogic(const vector<Question>& targetQuestions, string typeName);
    void ShowQuestionDetail(const Question& q);
    static vector<int> BuildDisplayIndex(const vector<Question>& src, int sortMode, const string& searchLower);
    static void PromptSearchKeyword(string& currentKeyword);
};

// ===== 문제 제작 기능 =====
class QuestionMaker {
public:
    void Run();         // 통합 문제 제작
    void RunPractice(); // 연습문제 제작 (추후 수정 예정)
    void RunExam();     // 시험모드 문제 제작
};

// ===== 문제 풀이 기능 (연습/시험) =====
class QuizManager {
public:
    void SolvePractice(); // 연습문제 풀기
    void SolveExam();     // 시험모드 풀기
private:
    static void BuildChoices(const vector<Question>& pool, int questionIndex,
        vector<string>& choices, int& answerSlot);
};

// ===== 게임모드: 추상 베이스 클래스 (다형성) =====
class GameMode {
public:
    virtual ~GameMode() {}
    virtual void Run() = 0; // 순수 가상 함수 - 파생 클래스가 구현
};

// 보스 몬스터 처치 모드
class BossMonsterMode : public GameMode {
public:
    void Run() override; // 게임 실행
private:
    void PrintBoss();
    void PrintBossDamaged();
    void PrintAttackEffect(int damage);
};

// 무한의 탑 모드
class InfiniteTowerMode : public GameMode {
public:
    void Run() override; // 게임 실행
private:
    void PrintTower(int floor, int lives, int combo, int score);
    void BuildTowerChoices(const vector<Question>& pool, int qIdx,
        vector<string>& choices, int& answerSlot);
    string GetDifficultyLabel(int floor);
    bool IsBossFloor(int floor);
    string GetGrade(int floor);
    void PrintQuizScreen(int floor, int lives, int combo, int score,
        bool isBoss, int bossCorrect, int bossRequired,
        bool itemFloor, bool shieldUsed,
        const string& desc, const vector<string>& choices,
        int focusChoice);
};
