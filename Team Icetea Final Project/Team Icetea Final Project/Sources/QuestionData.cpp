// QuestionData.cpp
#include "QuestionData.h"
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

// 전역 문제 데이터 벡터
vector<Question> PracticeQuestions;
vector<Question> ExamQuestions;

// ExamQuestionMake()에서 호출 - 현재는 비워둠 (MakeQuestion()이 CSV로 직접 저장)
void AddExamQuestion(const Question& newQuestion)
{
    (void)newQuestion;
}

// ===================================================
// 시작 시 ./QuestionData/ 폴더의 모든 CSV를 스캔하여
// PracticeQuestions에 로드합니다.
// (ExamQuestions는 시험모드 진입 시 직접 로드)
// ===================================================
void LoadAllQuestionData()
{
    cout << "데이터를 로딩 중입니다..." << endl;

    // --- CSV-based: scan all .csv files in ./QuestionData/ ---
    PracticeQuestions.clear();
    string folderPath = "./QuestionData";

    if (fs::exists(folderPath))
    {
        vector<fs::path> csvFiles;
        for (const auto& entry : fs::directory_iterator(folderPath))
        {
            if (entry.path().extension() == ".csv")
                csvFiles.push_back(entry.path());
        }

        // Sort by leading number in filename (e.g. "1. Test1.csv" -> 1)
        sort(csvFiles.begin(), csvFiles.end(), [](const fs::path& a, const fs::path& b)
        {
            int na = 0, nb = 0;
            try { na = stoi(a.stem().string()); } catch (...) {}
            try { nb = stoi(b.stem().string()); } catch (...) {}
            return na < nb;
        });

        for (const auto& csvPath : csvFiles)
        {
            ifstream csvFile(csvPath);
            if (!csvFile.is_open()) continue;

            string line;
            getline(csvFile, line); // skip header row (Q.DataID, Q.Desc, ...)

            while (getline(csvFile, line))
            {
                if (line.empty()) continue;
                if (!line.empty() && line.back() == '\r') line.pop_back();

                // Simple CSV parse with quoted-field support
                vector<string> fields;
                string cell;
                bool inQ = false;
                for (size_t ci = 0; ci < line.length(); ci++)
                {
                    char c = line[ci];
                    if (inQ)
                    {
                        if (c == '"' && ci + 1 < line.length() && line[ci + 1] == '"')
                        { cell += '"'; ci++; }
                        else if (c == '"') inQ = false;
                        else cell += c;
                    }
                    else
                    {
                        if (c == '"' && cell.empty()) inQ = true;
                        else if (c == ',') { fields.push_back(cell); cell.clear(); }
                        else cell += c;
                    }
                }
                fields.push_back(cell);

                // columns: DataID, Desc, Answer, W1, W2, W3, Difficulty, Explanation, Keyword
                if (fields.size() >= 6)
                {
                    Question q;
                    q.desc          = fields[1];
                    q.nameKr        = fields[2]; // correct answer
                    q.nameEn        = fields[3]; // wrong answer 1
                    q.character     = fields[4]; // wrong answer 2
                    q.keyword       = fields[5]; // wrong answer 3
                    q.level         = 0; try { if (fields.size() > 6) q.level = stoi(fields[6]); } catch (...) {}
                    q.commentary    = (fields.size() > 7) ? fields[7] : "";
                    q.searchKeyword = (fields.size() > 8) ? fields[8] : "";
                    PracticeQuestions.push_back(q);
                }
            }
            csvFile.close();
        }
    }

    // ExamQuestions: loaded on demand inside ExamQuestionSolve() via SelectSubjectMenu() + CSV

    cout << "로딩 완료! (총 " << PracticeQuestions.size() << "개 문제)" << endl;
}
