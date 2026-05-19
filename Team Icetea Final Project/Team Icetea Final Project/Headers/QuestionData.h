// QuestionData.h
#pragma once
#include <string>
#include <vector>

using namespace std;

// 퍽 하나에 대한 정보 구조체
struct Question {
    string nameKr;    // 한글 이름
    string nameEn;    // 영문 이름
    string character; // 전승 캐릭터 (공용 포함)
    string desc;      // 설명 (아직 미구현 시 빈칸)
    string keyword;   // 키워드 (아직 미구현 시 빈칸)
    string commentary; // 문제 해설을 저장할 변수
    vector<string> wrongChoices;
    int level;
};

// 퍽 데이터를 저장할 전역 벡터 (외부에서 접근 가능하게 extern 선언)
extern vector<Question> PracticeQuestions;
extern vector<Question> ExamQuestions;

// 데이터 로딩 함수 선언
void LoadAllQuestionData();

void AddPracticeQuestion(const Question& newQuestion);
void AddExamQuestion(const Question& newQuestion);
