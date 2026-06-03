// QuestionData.h
#pragma once
#include <string>
#include <vector>

using namespace std;

// 문제 하나에 대한 정보를 담는 구조체
struct Question {
    string answer;    // 정답
    string wrongAnswer1;   // 오답 1번
    string wrongAnswer2;   // 오답 2번
    string wrongAnswer3;   // 오답 3번 (아직 미구현 시 빈칸)
    string desc;            // 문제 설명 (아직 미구현 시 빈칸)
    string commentary; // 해설 (정답 선택 후 표시되는 내용)
    string searchKeyword; // 검색용 키워드 필드
    vector<string> wrongChoices;
    int level;
};

// 문제 데이터를 저장할 전역 벡터 (외부에서 접근 가능하게 extern 선언)
extern vector<Question> PracticeQuestions;
extern vector<Question> ExamQuestions;

// 데이터 로딩 함수 선언
void LoadAllQuestionData();

void AddExamQuestion(const Question& newQuestion);
