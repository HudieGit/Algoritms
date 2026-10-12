// search.h — алгоритмы поиска фрагмента Y в тексте X
// A — алгоритм Рабина–Карпа с полиномиальным хэшем
// B — алгоритм Кнута–Морриса–Пратта
//
// Обе функции возвращают позиции j (нумерация с 1), в которых f[j] = n,
// то есть позиции текста, где кончается полное вхождение шаблона
#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Счетчики операций — нужны для анализа
// Счетчики времени отключаются (Count = false), чтобы не влиять на результат
struct SearchStats {
    long long charCmp = 0;     // посимвольные сравнения
    long long hashHits = 0;    // РК: совпадения хэшей окна и шаблона
    long long collisions = 0;  // РК: хэши совпали, а строки — нет
    long long fallbacks = 0;   // КМП: «откаты» q = pi[q-1]
};

// ---------------------------------------------------------------------
// Алгоритм Рабина–Карпа
// h(w) = (w1*B^(n-1) + w2*B^(n-2) + ... + wn*B^0) mod P
// ---------------------------------------------------------------------
constexpr uint64_t RK_BASE = 257;            // основание
constexpr uint64_t RK_MOD  = 1000000007ULL;  // большое простое число

template <bool Count = false>
std::vector<int> rabinKarp(const std::string& X, const std::string& Y, SearchStats* st = nullptr) {
    //получили размерности X, Y
    const int m = (int)X.size(), n = (int)Y.size();
    //вектор результатов вхождений
    std::vector<int> res;
    if (n == 0 || n > m) return res;

    //считаем хэши, начальный и для Y
    uint64_t hY = 0, hW = 0, Bn1 = 1;  // Bn1 = B^(n-1) mod P
    for (int i = 0; i < n; ++i) {
        //Каждый последующий хэш = предыдущий * основание + текущий хэш
        //основание степенями полинома, для уменьшений колизий
        hY = (hY * RK_BASE + (unsigned char)Y[i]) % RK_MOD;
        hW = (hW * RK_BASE + (unsigned char)X[i]) % RK_MOD;
        if (i > 0) Bn1 = Bn1 * RK_BASE % RK_MOD;
    }

    for (int s = 0;; ++s) {  // s — начало окна (0-based), окно X[s .. s+n-1]
        if (hW == hY) {
            if constexpr (Count) st->hashHits++;
            int i = 0;  // посимвольная проверка — только при совпадении хэшей
            while (i < n) {
                if constexpr (Count) st->charCmp++;
                if (X[s + i] != Y[i]) break;
                ++i;
            }
            //Если прошли все итерации и i = n => колизий не было, можем записывать совпадение
            if (i == n) res.push_back(s + n);  // f[s+n] = n
            else if constexpr (Count) st->collisions++;
        }
        //Дошли до конца, выход из for
        if (s + n >= m) break;
        // Пересчёт хэша за O(1): убираем X[s], добавляем X[s+n]
        hW = (hW + RK_MOD - (unsigned char)X[s] * Bn1 % RK_MOD) % RK_MOD;
        hW = (hW * RK_BASE + (unsigned char)X[s + n]) % RK_MOD;
    }
    return res;
}

// ---------------------------------------------------------------------
// Алгоритм Кнута–Морриса–Пратта
// pi[i] — длина наибольшего собственного суффикса Y[0..i],
//         являющегося его префиксом (префикс-функция)
// ---------------------------------------------------------------------
template <bool Count = false>
std::vector<int> prefixFunction(const std::string& Y, SearchStats* st = nullptr) {
    const int n = (int)Y.size();
    std::vector<int> pi(n, 0);
    for (int i = 1; i < n; ++i) {
        int q = pi[i - 1];
        while (q > 0 && Y[i] != Y[q]) {
            if constexpr (Count) { st->charCmp++; st->fallbacks++; }
            q = pi[q - 1];
        }
        if constexpr (Count) st->charCmp++;
        if (Y[i] == Y[q]) ++q;
        pi[i] = q;
    }
    return pi;
}

template <bool Count = false>
std::vector<int> kmp(const std::string& X, const std::string& Y,
                     SearchStats* st = nullptr) {
    const int m = (int)X.size(), n = (int)Y.size();
    std::vector<int> res;
    if (n == 0 || n > m) return res;

    const std::vector<int> pi = prefixFunction<Count>(Y, st);
    int q = 0;  // число совпавших символов шаблона = f[j]
    for (int j = 0; j < m; ++j) {  // указатель по тексту только растёт
        while (q > 0 && X[j] != Y[q]) {
            if constexpr (Count) { st->charCmp++; st->fallbacks++; }
            q = pi[q - 1];  // «откат» по префикс-функции
        }
        if constexpr (Count) st->charCmp++;
        if (X[j] == Y[q]) ++q;
        if (q == n) {  // f[j+1] = n — полное вхождение
            res.push_back(j + 1);
            q = pi[q - 1];
        }
    }
    return res;
}
