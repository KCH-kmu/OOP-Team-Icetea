// QuestionData.cpp
#include "QuestionData.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

// 전역 벡터 정의
vector<Question> PracticeQuestions;
vector<Question> ExamQuestions;

// 내부에서만 쓰는 헬퍼 함수: 특정 속성(멤버)만 파일에서 읽어오기
void LoadFileToVector(vector<Question>& targetVector, string filename, int type)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cout << "Error: " << filename << " 파일을 찾을 수 없습니다." << endl;
        return;
    }

    int count;
    file >> count; // 1. 총 개수 읽기

    // 개수 뒤의 엔터 키 처리 등 잔여 버퍼 비우기
    string dummy;
    getline(file, dummy);

    if (type == 0) // 한글 이름 파일일 때만 벡터 크기 설정
    {
        targetVector.resize(count);
    }

    for (int i = 0; i < count; i++)
    {
        int index;
        // 2. 인덱스 번호 읽기
        if (!(file >> index)) break;

        // **[수정 로직 시작]**: 숫자 뒤에 오는 모든 공백/탭 문자를 건너뜁니다.
        char separator;
        // 다음 문자를 읽지만, 스트림 위치는 옮기지 않습니다.
        if (file.peek() == ' ' || file.peek() == '\t')
        {
             // 탭이나 공백이 있으면 읽어서 버립니다.
             file.get(separator);
             // 이후 공백/탭이 연속될 경우 모두 건너뜁니다.
             while (file.peek() == ' ' || file.peek() == '\t')
             {
                 file.get(separator);
             }
        }
        // **[수정 로직 끝]**

        string line;
        // 3. 나머지 문장(내용)을 읽음 (공백 포함)
        getline(file, line);

        // 줄바꿈 문자(\r)가 포함된 경우 제거 (윈도우 호환)
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // 타입에 따라 구조체의 적절한 변수에 저장
        switch (type)
        {
        case 0: targetVector[i].nameKr = line; break;
        case 1: targetVector[i].nameEn = line; break;
        case 2: targetVector[i].character = line; break;
        case 3: targetVector[i].desc = line; break;
        case 4: targetVector[i].keyword = line; break;
        }
    }
    file.close();
}

// [유지됨] 벡터의 내용을 파일로 저장하는 헬퍼 함수 (저장 시에는 공백 ' '을 사용)
void WriteVectorToFile(const vector<Question>& targetVector, string filename, int type)
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cout << "Error: " << filename << " 저장 실패!" << endl;
        return;
    }

    // 1. 맨 윗줄에 현재 개수 기록
    file << targetVector.size() << endl;

    // 2. 모든 문제 데이터를 순회하며 저장
    for (size_t i = 0; i < targetVector.size(); ++i)
    {
        // "인덱스(i+1) + 공백 + 데이터" 형식으로 저장
        file << (i + 1) << " ";

        switch (type)
        {
        case 0: file << targetVector[i].nameKr << endl; break;
        case 1: file << targetVector[i].nameEn << endl; break;
        case 2: file << targetVector[i].character << endl; break;
        case 3: file << targetVector[i].desc << endl; break;
        case 4: file << targetVector[i].keyword << endl; break;
        }
    }
    file.close();
}

// 아래 추가/로딩 함수들은 변경 사항 없음
void AddPracticeQuestion(const Question& newQuestion)
{
    PracticeQuestions.push_back(newQuestion);
    cout << "파일 저장 중..." << endl;
    WriteVectorToFile(PracticeQuestions, "PracticeName_kr.txt", 0);
    WriteVectorToFile(PracticeQuestions, "PracticeName_en.txt", 1);
    WriteVectorToFile(PracticeQuestions, "PracticeCharacter.txt", 2);
    WriteVectorToFile(PracticeQuestions, "PracticeDesc.txt", 3);
    WriteVectorToFile(PracticeQuestions, "PracticeKeyword.txt", 4);
    cout << "저장 완료!" << endl;
}

void AddExamQuestion(const Question& newQuestion)
{
    ExamQuestions.push_back(newQuestion);
    cout << "파일 저장 중..." << endl;
    WriteVectorToFile(ExamQuestions, "ExamName_kr.txt", 0);
    WriteVectorToFile(ExamQuestions, "ExamName_en.txt", 1);
    WriteVectorToFile(ExamQuestions, "ExamCharacter.txt", 2);
    WriteVectorToFile(ExamQuestions, "ExamDesc.txt", 3);
    WriteVectorToFile(ExamQuestions, "ExamKeyword.txt", 4);
    cout << "저장 완료!" << endl;
}

void LoadAllQuestionData()
{
    cout << "데이터를 로딩 중입니다..." << endl;

    // --- 연습문제 문제 로딩 ---
    LoadFileToVector(PracticeQuestions, "PracticeName_kr.txt", 0);
    LoadFileToVector(PracticeQuestions, "PracticeName_en.txt", 1);
    LoadFileToVector(PracticeQuestions, "PracticeCharacter.txt", 2);
    LoadFileToVector(PracticeQuestions, "PracticeDesc.txt", 3);
    LoadFileToVector(PracticeQuestions, "PracticeKeyword.txt", 4);

    // --- 시험모드 문제 로딩 ---
    LoadFileToVector(ExamQuestions, "ExamName_kr.txt", 0);
    LoadFileToVector(ExamQuestions, "ExamName_en.txt", 1);
    LoadFileToVector(ExamQuestions, "ExamCharacter.txt", 2);
    LoadFileToVector(ExamQuestions, "ExamDesc.txt", 3);
    LoadFileToVector(ExamQuestions, "ExamKeyword.txt", 4);

    cout << "로딩 완료! (연습문제: " << PracticeQuestions.size() << "개, 시험모드: " << ExamQuestions.size() << "개)" << endl;
}
