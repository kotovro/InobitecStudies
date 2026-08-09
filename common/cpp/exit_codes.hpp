#ifndef KV_EXIT_CODES_HPP
#define KV_EXIT_CODES_HPP

// Семантика sysexits: машинный канал для скриптов-обёрток.
// Двухканальность: exit-код — категория, stderr — детали человеку.

enum class ExitCode : int {
    kOk = 0,
    kUsage = 64,   // битые аргументы / usage
    kData = 65,    // битый формат / невалидные данные
    kNoInput = 66, // пустой ввод / нет данных
    kIOErr = 74,   // сбой ввода-вывода
};

#endif