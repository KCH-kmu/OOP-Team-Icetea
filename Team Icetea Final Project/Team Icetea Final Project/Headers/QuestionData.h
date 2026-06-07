// QuestionData.h
#pragma once
#include <string>
#include <vector>

using namespace std;

// 문제 하나에 대한 정보를 담는 구조체
struct Question {
    string answer;        // 정답
    string wrongAnswer1;  // 오답 1번
    string wrongAnswer2;  // 오답 2번
    string wrongAnswer3;  // 오답 3번
    string desc;          // 문제 설명
    string commentary;    // 해설 (정답 풀린 후 표시)
    string searchKeyword; // 검색어 키워드
    vector<string> wrongChoices; // 추가 오답 후보 풀
    int level;            // 난이도
};

// ===== 문제 데이터 보관/로딩 클래스 (싱글턴) =====
// 기존 전역 벡터(PracticeQuestions / ExamQuestions)를 캡슐화한다.
class QuestionBank {
public:
    // 싱글턴 인스턴스 접근자
    static QuestionBank& Instance();

    // 시작 시 ./QuestionData/ 폴더의 모든 CSV를 연습문제로 로드
    void LoadAll();
    // 시험문제 추가 (현재는 CSV 직접 저장 방식이라 비워둠)
    void AddExam(const Question& newQuestion);

    // 연습문제 벡터 접근
    vector<Question>& Practice();
    // 시험문제 벡터 접근
    vector<Question>& Exam();

private:
    QuestionBank() = default;   // 외부 생성 금지 (싱글턴)
    vector<Question> practice_; // 연습문제 데이터
    vector<Question> exam_;     // 시험문제 데이터
};
