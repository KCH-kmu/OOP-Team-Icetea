// QuestionData.h
#pragma once
#include <string>
#include <vector>

using namespace std;

// �� �ϳ��� ���� ���� ����ü
struct Question {
    string nameKr;    // �ѱ� �̸�
    string nameEn;    // ���� �̸�
    string character; // ���� ĳ���� (���� ����)
    string desc;      // ���� (���� �̱��� �� ��ĭ)
    string keyword;   // Ű���� (���� �̱��� �� ��ĭ)
    string commentary; // ���� �ؼ��� ������ ����
    string searchKeyword; // ���� ���� Ű����
    vector<string> wrongChoices;
    int level;
};

// �� �����͸� ������ ���� ���� (�ܺο��� ���� �����ϰ� extern ����)
extern vector<Question> PracticeQuestions;
extern vector<Question> ExamQuestions;

// ������ �ε� �Լ� ����
void LoadAllQuestionData();

void AddPracticeQuestion(const Question& newQuestion);
void AddExamQuestion(const Question& newQuestion);
