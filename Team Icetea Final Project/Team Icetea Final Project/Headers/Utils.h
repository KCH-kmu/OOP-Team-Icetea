#pragma once // 헤더 파일 중복 포함 방지
#include <iostream>
#include <conio.h>
#include <string>

// 키보드 값 상수 정의
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75   // 좌측 화살표 키
#define KEY_RIGHT 77  // 우측 화살표 키
#define KEY_ENTER 13
#define KEY_ESC 27
#define KEY_TAB 9         // 탭 키 (정렬 모드 순환 이동)
#define KEY_BACKSPACE 8   // 백스페이스 (검색 입력 시 글자 삭제)

// ===== 콘솔 출력 유틸리티 클래스 =====
// 화면 지우기 등 콘솔 표시 관련 기능을 정적 메서드로 제공한다.
class ConsoleUI
{
public:
    // 화면 지우기 (ANSI 이스케이프 시퀀스 사용)
    static void Clear()
    {
        std::cout << "\033[2J\033[H";
        std::cout.flush();
    }
};
