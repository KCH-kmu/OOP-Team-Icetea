#pragma once // 헤더 파일 중복 포함 방지
#include <iostream>
#include <conio.h>

// 키보드 값 상수 정의
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75   // 좌측 화살표 추가
#define KEY_RIGHT 77  // 우측 화살표 추가
#define KEY_ENTER 13
#define KEY_ESC 27

// 화면 지우기 함수
inline void screenClear()
{
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}
