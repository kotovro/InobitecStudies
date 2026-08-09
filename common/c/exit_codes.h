#ifndef KV_EXIT_CODES_H
#define KV_EXIT_CODES_H

// Семантика sysexits: машинный канал для скриптов-обёрток.
// Двухканальность: exit-код — категория, stderr — детали человеку.

#define EC_OK 0
#define EC_USAGE 64   // битые аргументы / usage
#define EC_DATA 65    // битый формат / невалидные данные
#define EC_NOINPUT 66 // пустой ввод / нет данных
#define EC_IOERR 74   // сбой ввода-вывода

#endif