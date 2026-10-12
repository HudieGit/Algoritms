// verify.cpp — программа верификации (функциональная)
// Вход:  файл, первая строка — текст X, вторая строка — шаблон Y.
// Выход: два файла, по одному на алгоритм: позиции вхождений и время работы.
//
// Запуск:  ./verify input.txt [out_rabin_karp.txt] [out_kmp.txt]
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "search.h"

#ifdef _WIN32  
#define NOMINMAX  // иначе windows.h ломает std::min / std::max
#include <windows.h>
#endif

using Clock = std::chrono::steady_clock;

template <class F>
double measureSeconds(F&& f) {
    auto t0 = Clock::now();
    f();
    auto t1 = Clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

static void writeResult(const std::string& path, const std::string& algoName,
                        const std::string& X, const std::string& Y,
                        const std::vector<int>& ends, double seconds) {
    std::ofstream out(path);
    const int n = (int)Y.size();
    out << "Алгоритм: " << algoName << "\n";
    out << "Длина текста m = " << X.size() << ", длина шаблона n = " << n << "\n";
    out << "Число вхождений: " << ends.size() << "\n";
    out << "Позиции j, где f[j] = n (конец вхождения, нумерация с 1):\n";
    for (size_t i = 0; i < ends.size(); ++i) out << ends[i] << (i + 1 < ends.size() ? ' ' : '\n');
    if (ends.empty()) out << "-\n";
    out << "Позиции начала вхождений (j - n + 1):\n";
    for (size_t i = 0; i < ends.size(); ++i) out << ends[i] - n + 1 << (i + 1 < ends.size() ? ' ' : '\n');
    if (ends.empty()) out << "-\n";
    out << std::fixed << std::setprecision(9);
    out << "Время работы: " << seconds << " с\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0]
                  << " input.txt [out_rabin_karp.txt] [out_kmp.txt]\n";
        return 1;
    }
    const std::string outA = argc > 2 ? argv[2] : "out_rabin_karp.txt";
    const std::string outB = argc > 3 ? argv[3] : "out_kmp.txt";

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Не удалось открыть файл " << argv[1] << "\n";
        return 1;
    }
    std::string X, Y;
    std::getline(in, X);
    std::getline(in, Y);
    // убираем '\r' у файлов, сохранённых в Windows
    if (!X.empty() && X.back() == '\r') X.pop_back();
    if (!Y.empty() && Y.back() == '\r') Y.pop_back();

    if (Y.empty() || Y.size() > X.size()) {
        std::cerr << "Нужно 1 <= n <= m (длина шаблона не больше длины текста)\n";
        return 1;
    }

    std::vector<int> resA, resB;
    double tA = measureSeconds([&] { resA = rabinKarp(X, Y); });
    double tB = measureSeconds([&] { resB = kmp(X, Y); });

    writeResult(outA, "A - Рабин-Карп (полиномиальный хэш)", X, Y, resA, tA);
    writeResult(outB, "B - Кнут-Моррис-Пратт", X, Y, resB, tB);

    std::cout << "Вхождений: РК = " << resA.size() << ", КМП = " << resB.size()
              << (resA == resB ? "  (результаты совпадают)\n" : "  (РЕЗУЛЬТАТЫ РАЗЛИЧАЮТСЯ!)\n");
    std::cout << std::fixed << std::setprecision(9)
              << "T_A = " << tA << " с, T_B = " << tB << " с\n"
              << "Результаты записаны в " << outA << " и " << outB << "\n";
    return resA == resB ? 0 : 2;
}
