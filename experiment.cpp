// experiment.cpp — программа проведения экспериментов (исследовательская)
//
// Режимы:
//   ./experiment                         — интерактивное меню
//   ./experiment series 3.1 out.csv [R]  — серия 3.1: Y=(ab)^k, X=(ab)^(1000k)
//   ./experiment series 3.2 out.csv [R]  — серия 3.2: X случайное в {a,b}, |X|=10^6+1, Y=a^n
//   ./experiment series 3.3 out.csv [R]  — серия 3.3: Y=aaaaa, X=(aaaaab)^h
//   R — число повторов замера в точке (берётся медиана), по умолчанию 5.
//
// Выход серии — CSV: значение параметра, T_A и T_B (секунды) и счётчики операций.
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "search.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using Clock = std::chrono::steady_clock;
static std::mt19937 rng(20261009);  // фиксированное зерно — воспроизводимость

// ---------------------------- генерация слов ----------------------------

// алфавит мощности q: первые q строчных латинских букв (q <= 26)
std::string alphabet(int q) {
    std::string a;
    for (int i = 0; i < q; ++i) a += char('a' + i);
    return a;
}

// Случайное слово длины len, буквы равновероятны
std::string randomWord(int len, const std::string& letters) {
    std::uniform_int_distribution<int> d(0, (int)letters.size() - 1);
    std::string w(len, ' ');
    for (char& c : w) c = letters[d(rng)];
    return w;
}

// Слово вида (B1 B2 ... Bs)^k
std::string repeatWord(const std::string& base, long long k) {
    std::string w;
    w.reserve(base.size() * k);
    for (long long i = 0; i < k; ++i) w += base;
    return w;
}

// ---------------------------- замер времени -----------------------------

static volatile long long sink = 0;  // чтобы компилятор не выбросил вызовы

template <class F>
double timeOnce(F&& f) {
    auto t0 = Clock::now();
    sink += f();
    auto t1 = Clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

// Медиана R замеров
template <class F>
double timeMedian(F&& f, int R) {
    std::vector<double> t(R);
    for (int i = 0; i < R; ++i) t[i] = timeOnce(f);
    std::sort(t.begin(), t.end());
    return t[R / 2];
}

struct Point {
    long long param;
    size_t m, n;
    double tA, tB;
    size_t occ;
    SearchStats sA, sB;
};

Point runPoint(long long param, const std::string& X, const std::string& Y, int R) {
    Point p{param, X.size(), Y.size(), 0, 0, 0, {}, {}};
    p.tA = timeMedian([&] { return (long long)rabinKarp(X, Y).size(); }, R);
    p.tB = timeMedian([&] { return (long long)kmp(X, Y).size(); }, R);
    // отдельный (не замеряемый) прогон со счётчиками операций
    auto ra = rabinKarp<true>(X, Y, &p.sA);
    auto rb = kmp<true>(X, Y, &p.sB);
    if (ra != rb) {
        std::cerr << "Error: результаты алгоритмов различаются при param=" << param << "\n";
        std::exit(2);
    }
    p.occ = ra.size();
    return p;
}

// ------------------------------- серии ----------------------------------

void runSeries(const std::string& id, const std::string& csvPath, int R) {
    std::ofstream csv(csvPath);
    std::string pname = id == "3.1" ? "k" : id == "3.2" ? "n" : "h";
    csv << pname << "_param,m,n,T_A_RK_s,T_B_KMP_s,occurrences,"
        << "rk_char_cmp,rk_hash_hits,rk_collisions,kmp_char_cmp,kmp_fallbacks\n";
    csv << std::setprecision(9);

    std::string X;
    if (id == "3.2") X = randomWord(1000001, "ab");  // текст фиксирован для всей серии

    for (long long v = 1;; v += (id == "3.1" ? 10 : 10000)) {
        if (id == "3.1" && v > 1001) break;
        if (id != "3.1" && v > 1000001) break;
        std::string Y;
        if (id == "3.1") { Y = repeatWord("ab", v); X = repeatWord("ab", 1000 * v); }
        else if (id == "3.2") { Y = std::string(v, 'a'); }
        else { Y = "aaaaa"; X = repeatWord("aaaaab", v); }

        Point p = runPoint(v, X, Y, R);
        csv << p.param << ',' << p.m << ',' << p.n << ',' << p.tA << ',' << p.tB << ','
            << p.occ << ',' << p.sA.charCmp << ',' << p.sA.hashHits << ','
            << p.sA.collisions << ',' << p.sB.charCmp << ',' << p.sB.fallbacks << '\n';
        csv.flush();
        std::cerr << id << ": " << pname << "=" << v << "  T_A=" << p.tA
                  << "  T_B=" << p.tB << "\n";
    }
}

// --------------------------- интерактивный режим ------------------------

std::string askWord(const char* name, const std::string& letters) {
    std::cout << "\n Способ задания слова " << name << ":\n"
              << "  1 - ввести вручную\n"
              << "  2 - случайное слово заданной длины (буквы алфавита равновероятны)\n"
              << "  3 - слово вида (B1...Bs)^k\n> ";
    int way;
    std::cin >> way;
    std::string w;
    if (way == 1) {
        std::cout << "Введите слово: ";
        std::cin >> w;
    } else if (way == 2) {
        long long len;
        std::cout << "Длина: ";
        std::cin >> len;
        w = randomWord((int)len, letters);
    } else {
        std::string base;
        long long k;
        std::cout << "Слово B1...Bs: ";
        std::cin >> base;
        std::cout << "k: ";
        std::cin >> k;
        w = repeatWord(base, k);
    }
    for (char c : w)
        if (letters.find(c) == std::string::npos) {
            std::cout << "Внимание: символ '" << c << "' не входит в алфавит\n";
            break;
        }
    return w;
}

void customExperiment() {
    int q;
    std::cout << "Мощность алфавита A (1..26, буквы a, b, c, ...): ";
    std::cin >> q;
    q = std::max(1, std::min(26, q));
    const std::string letters = alphabet(q);

    std::string X = askWord("X (текст)", letters);
    std::string Y = askWord("Y (шаблон)", letters);
    if (Y.empty() || Y.size() > X.size()) {
        std::cout << "Нужно 1 <= n <= m\n";
        return;
    }
    Point p = runPoint(0, X, Y, 5);
    std::cout << std::fixed << std::setprecision(9)
              << "\nm = " << p.m << ", n = " << p.n << ", вхождений: " << p.occ << "\n"
              << "T_A (Рабин-Карп) = " << p.tA << " с\n"
              << "T_B (КМП)        = " << p.tB << " с\n"
              << "РК: совпадений хэшей " << p.sA.hashHits << ", коллизий " << p.sA.collisions
              << ", сравнений символов " << p.sA.charCmp << "\n"
              << "КМП: откатов " << p.sB.fallbacks << ", сравнений символов "
              << p.sB.charCmp << "\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc >= 4 && std::string(argv[1]) == "series") {
        int R = argc >= 5 ? std::atoi(argv[4]) : 5;
        runSeries(argv[2], argv[3], std::max(1, R));
        return 0;
    }
    std::cout << "1 - свой эксперимент\n2 - серия 3.1\n3 - серия 3.2\n4 - серия 3.3\n> ";
    int c;
    std::cin >> c;
    if (c == 1) customExperiment();
    else if (c >= 2 && c <= 4) {
        std::string id = c == 2 ? "3.1" : c == 3 ? "3.2" : "3.3";
        runSeries(id, "series_" + id + ".csv", 5);
        std::cout << "Готово: series_" << id << ".csv\n";
    }
    return 0;
}
