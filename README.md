# Программа подготовки к стажировке в Inobitec — трек C/C++

Репозиторий с задачами оценочного периода. Домен задачника — растровые
изображения в текстовом формате PPM (`P3` — RGB, тройки ASCII).

## Структура проекта

```
AGENTS.md          — журнал работы и память агента
common/
  c/               — общие модули (C)
    api.h          — макрос для сборки общего модуля в формате библиотеки (lib/import lib + dll)
    exit_codes.h   — именованные exit-коды
    version.h      — версия программ (`KV_VERSION`)
    ppm_io.h(.c)   — модуль ввода-вывода PPM (парсинг/запись, экспорт в DLL)
    luma.h         — яркость (luma) по каналам (header-only)
    int_token.h    — строгий разбор целых токенов (header-only; только C, в C++ — `std::from_chars`)
    strerror.h(.c) — сообщение системной ошибки, платформонезависимо
  cpp/             — общие модули (C++)
    exit_codes.hpp — именованные exit-коды
    version.hpp    — версия программ (`kVersion`)
    luma.hpp       — яркость (luma) по каналам (header-only)
    ppm_io.hpp(.cpp) — модуль ввода-вывода PPM (PIMPL, экспорт в DLL)
build/
  <вариант>/       — артефакты одного варианта сборки (см. «Сборка»)
    common/{c,cpp}/ — объекты, библиотеки, DLL общего модуля
    {task}/{c,cpp,ref}/ — объекты, exe, эталоны задачи
    test_data/     — сгенерированные тестовые PPM и эталоны (в .gitignore)
dialog_logs/       — сырые логи диалогов с DeepSeek
00-intro-hello/    — hello world
 01-image-gen/      — задача 1: генератор PPM
   c/               — реализация на C
   cpp/             — реализация на C++
   ref/             — генераторы эталонных изображений(реализованы на С, используются для acceptance тестов)
 02-image-passport/ — задача 2: паспорт изображения
   c/               — реализация на C
   cpp/             — реализация на C++
   ref/             — эталон для acceptance-тестов
 03-image-stats/    — задача 3: статистика изображения
   c/
   cpp/
   ref/
 04-image-filter/   — задача 4: фильтр изображения
   c/               — реализация на C
   cpp/             — реализация на C++
   ref/             — эталоны для acceptance-тестов(предполагается сравнение результатов запуска acceptance тестов с прогоном фильтра на эталонах из первой задачи) 
```

Все задачи решаются дважды: C (C17) и C++ (C++23).

---

## Требования к окружению

- **OS:** Windows (основная) и Linux (проверка переносимости); платформа задаётся `PLATFORM=windows|linux`.
- **Toolchain Windows:** MSVC (Visual Studio Build Tools, `vcvars64.bat`).
- **Toolchain Linux:** gcc/g++ с поддержкой `-std=c++23`; для версионированных компиляторов — `GCC_SUFFIX` (например, `GCC_SUFFIX=-15`).
- **GNU make ≥ 4.4** — единственный вход в сборку; на Windows ставится `winget install ezwinports.make`.
- **clang-format** — в PATH или через `CLANG_FORMAT=<путь>`; нужен целям `test`, `all`, `format`, `format-check`.
- **Консоль:** PowerShell / FAR Manager / cmd, кодировка UTF-8 (`chcp 65001`). `make` можно запускать из любой: на Windows его рецепты выполняются через `cmd.exe`.
- **Просмотр PPM:** IrfanView, GIMP, ImageMagick (`magick out.ppm out.png`).

`PLATFORM` обязателен: `make` без него завершается с подсказкой.

**Важно для Windows (ручные запуски программ):** перенаправление (`<`, `>`) выполняйте в **Command Prompt (cmd.exe)**, а не в PowerShell: PowerShell 5.1 перекодирует вывод при `>`, и эталонный файл может получиться в формате, отличном от ожидаемого. Внутри `make` это не проблема — рецепты идут через `cmd.exe`; но примеры вида `prog > file` ниже даны для cmd, а для PowerShell обёрнуты в `cmd /c "…"`.

---

## Выпуск релиза
Перед тем, как присвоить тег по semversion коду, реализуется следующая последовательность действий:
1. Полная проверка из чистого клона на каждой платформе: `make PLATFORM=windows test` и `make PLATFORM=linux test` (Linux — на отдельной машине или VM). Цель `test` включает `format-check`, `unit`, `acceptance`, `parity` и выполняет сборку целиком, без частичных или инкрементальных запусков. Каждый тестовый бинарник должен завершиться кодом 0 и напечатать финальную строку All tests PASSED. Ненулевой код возврата или отсутствие финальной строки хотя бы у одного бинарника останавливает выпуск: причина разбирается и исправляется, тег не ставится.
2. Проверка идентичности поведения при сбоях: `make parity` сравнивает stdout успешных кейсов; тексты stderr ошибочных кейсов и вывод `--help`/`--version` пока сверяются вручную по разделу «Паритет C и C++» (автоматизация — в бэклоге). Дополнительно прогоняются входы, провоцирующие ошибки (например, заголовок чрезмерного размера или потребитель, преждевременно закрывающий канал), и подтверждается, что C и C++ завершаются одинаково: совпадает класс exit-кода и текст диагностики, а не только результат при успешном выполнении.
3. Сверка CHANGELOG на основе diff: поочерёдный анализ коммитов в диапазоне git log <last-tag>..HEAD и сопоставление их с записями в CHANGELOG для подтверждения того, что каждое изменение отражено в файле.
4. Проверка согласованности версий: подтверждение того, что вывод команды --version совпадает с создаваемым тегом и версией, указанной в заголовке CHANGELOG.
5. Вычитка текстов: список .md-файлов, изменённых с прошлого тега, берётся командой git diff --name-only <last-tag>..HEAD "*.md". Каждый файл из списка открывается в Notepad++ с расширением DSpellCheck (русский словарь) и проходится поиском следующего слова с ошибкой до конца файла. Каждое подчёркнутое слово разбирается: опечатка исправляется, термин (коммит, бинарник, аллокатор и т. п.) добавляется в пользовательский словарь. После этого изменённые абзацы перечитываются по git diff целиком: словарь не ловит ошибки, при которых получается существующее слово («было напечататься»). Тег не ставится, пока в файлах остаются неразобранные подчёркивания.

---
## Сборка

Сборка и проверки — через GNU make. Команды выполняются из корня репозитория. На Windows предварительно нужно окружение MSVC (`vcvars64.bat`), на Linux — gcc/g++ и GNU make в PATH.

```
# Windows
make PLATFORM=windows          # всё: библиотеки, приложения, тесты
make PLATFORM=windows test     # формат + юнит + приёмка + паритет

# Linux
make PLATFORM=linux test
make PLATFORM=linux CONFIG=release RUNTIME=full
```

### Вариант сборки

Артефакты одного варианта лежат в `build/<вариант>/`. Вариант собирается из осей:

`<toolchain>-<config>-<runtime>-<link>-<sanitize>[-cov]`

| Ось | Значения | По умолчанию |
|---|---|---|
| `PLATFORM` | `windows`, `linux` | — (обязателен) |
| `TOOLCHAIN` | `msvc` (windows), `gcc` (linux) | из `PLATFORM` |
| `CONFIG` | `debug`, `release` | `debug` |
| `RUNTIME` | `dynamic`, `static`, `full` | `dynamic` |
| `LINK` | `obj`, `lib`, `dll` | `lib` |
| `SANITIZE` | `address`, `none` | `address` для debug, `none` для release |
| `COVERAGE` | `0`, `1` | `0` (задел: цель `coverage` пока не реализована) |
| `WERROR` | `0`, `1` | `0` (только GCC) |
| `GCC_SUFFIX` | например `-15` | пусто |
| `CLANG_FORMAT` | путь к `clang-format` | `clang-format` из PATH |

Примеры: `build/msvc-debug-dynamic-lib-address/`, `build/msvc-release-dynamic-lib-none/`, `build/gcc-release-full-lib-none/`, `build/msvc-debug-dynamic-dll-address/`.

На Linux варианты начинаются с `gcc-` (например, `build/gcc-debug-dynamic-lib-address/`), исполняемые файлы без расширения, а сравнение файлов в примерах ниже — `cmp` вместо `fc`.

Текущие оси, каталог варианта и список целей печатает `make PLATFORM=windows help` (на Linux — `make PLATFORM=linux help`).

### Цели

| Цель | Что делает |
|---|---|
| `all` (по умолчанию) | `lib exes test` |
| `lib`, `common` | статические библиотеки общего модуля (и DLL при `LINK=dll`) |
| `exes` | приложения, тестовые бинарники и эталоны |
| `<задача>` (`01-image-gen`, …) | приложения одной задачи (C и C++) |
| `unit` | юнит-тесты (включая тесты общего модуля) |
| `acceptance` | приёмочные тесты: генерация эталонов и сравнение |
| `parity` | побайтовое сравнение stdout C- и C++-сборок |
| `massdata` | 6 случайных PPM для замеров в `test_data/` |
| `test` | `format-check unit acceptance parity` |
| `format`, `format-check` | `clang-format -i` / `--dry-run -Werror` по всем исходникам |
| `clean` | удалить каталог текущего варианта |
| `clean-all` | удалить `build/` целиком |
| `help` | оси, каталог варианта, цели, задачи |

### Флаги

Ниже — фактические флаги, которые получаются из осей.

**MSVC (`cl`, `link`):**

| Ось | Флаги |
|---|---|
| стандарт | `/std:c17` (C), `/std:c++latest /EHsc` (C++) |
| предупреждения | `/W4 /permissive- /utf-8 /nologo` |
| `CONFIG=debug` | `/Od` |
| `CONFIG=release` | `/O2 /DNDEBUG` |
| `RUNTIME=dynamic` | `/MDd` (debug) / `/MD` (release) |
| `RUNTIME=static` | `/MTd` (debug) / `/MT` (release) |
| `SANITIZE=address` | `/fsanitize=address` |
| отладочная информация | `/Zi /FS`, отдельный `compile.pdb` на каталог; линковка с `/DEBUG` (pdb есть и в Release) |
| линковка debug / release | `/INCREMENTAL:NO` / `-OPT:REF -OPT:ICF` |

**GCC (`gcc`/`g++`):**

| Ось | Флаги |
|---|---|
| стандарт | `-std=c17` (C), `-std=c++23` (C++) |
| предупреждения | `-Wall -Wextra -Wpedantic` (+ `-Werror` при `WERROR=1`) |
| `CONFIG=debug` | `-O0` |
| `CONFIG=release` | `-O2 -DNDEBUG` |
| `RUNTIME=dynamic` | — |
| `RUNTIME=static` | `-static-libstdc++ -static-libgcc` |
| `RUNTIME=full` | `-static` |
| `SANITIZE=address` | `-fsanitize=address -fno-omit-frame-pointer` |
| отладочная информация | `-g` (в самом бинаре; отдельный `.debug` не создаётся) |

### Полное встраивание рантайма (Linux, `RUNTIME=full`)

`RUNTIME=full` передаёт линкеру `-static`: в бинарь встраиваются libstdc++, libgcc и glibc, поэтому он не зависит от системных библиотек и запускается на машине без установленного рантайма:

```
make PLATFORM=linux CONFIG=release RUNTIME=full
```

Не путать с `RUNTIME=static`: там встраиваются только libstdc++ и libgcc (`-static-libstdc++ -static-libgcc`), а glibc остаётся внешней зависимостью.

Ограничения:

- только GCC/Linux — на MSVC `RUNTIME=full` отклоняется;
- несовместимо с `SANITIZE=address` и `LINK=dll`;
- статическая glibc имеет известные ограничения (NSS, `dlopen`): для утилиты без сети и плагинов это приемлемо, но об этом нужно помнить при поставке.

Проверка автономности: `ldd <exe>` не должен показывать зависимостей.

### Недопустимые сочетания

`make` завершается ошибкой до начала сборки для:

- `RUNTIME=full` в MSVC — только GCC;
- `LINK=dll` при `RUNTIME` не `dynamic` — DLL со встроенным рантаймом несовместима (через границу идут `FILE*` и `std::istream`);
- `LINK=dll` на GCC — `.so` в курсе не вводится;
- `RUNTIME=full` + `SANITIZE=address` — ASan с `-static` не работает;
- `COVERAGE=1` — требует `CONFIG=debug`, `SANITIZE=none`, `LINK=obj` (и цель `coverage` пока не реализована).

### Что делает make под капотом

Пример: задача 1, C, Debug. Make выполняет те же раздельные этапы, что и ручная сборка:

```
cl /std:c17 /W4 /permissive- /utf-8 /Od /Zi -FS /MDd /fsanitize=address /c /Fo:build/msvc-debug-dynamic-lib-address/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl ... 01-image-gen/c/patterns.c
cl ... 01-image-gen/c/hsv_to_rgb.c
cl ... 01-image-gen/c/main.c
cl ... /Fo:build/msvc-debug-dynamic-lib-address/common/c/ common/c/ppm_io.c
cl ... /Fo:build/msvc-debug-dynamic-lib-address/common/c/ common/c/strerror.c
lib -nologo -OUT:build/msvc-debug-dynamic-lib-address/common/c/ppm_io.lib build/msvc-debug-dynamic-lib-address/common/c/ppm_io.obj build/msvc-debug-dynamic-lib-address/common/c/strerror.obj
link -nologo -DEBUG -INCREMENTAL:NO -OUT:build/msvc-debug-dynamic-lib-address/01-image-gen/c/gen_image.exe build/msvc-debug-dynamic-lib-address/01-image-gen/c/parse_args.obj build/msvc-debug-dynamic-lib-address/01-image-gen/c/patterns.obj build/msvc-debug-dynamic-lib-address/01-image-gen/c/hsv_to_rgb.obj build/msvc-debug-dynamic-lib-address/01-image-gen/c/main.obj build/msvc-debug-dynamic-lib-address/common/c/ppm_io.lib
```

У каждой компиляции make дополнительно передаёт `-Fd<каталог>compile.pdb`: общий pdb на каталог, чтобы параллельная сборка не конфликтовала за один файл. На Linux те же этапы выполняют `gcc`/`g++` с флагами из таблицы GCC: `-c` даёт объектные файлы, затем линковка (`-o`).

Для остальных задач и режимов эта цепочка в README не дублируется — её строит make.

## Тесты

Все проверки запускаются через make: `make PLATFORM=windows test` (или `unit`, `acceptance`, `parity` по отдельности). Цель `test` включает `format-check`, `unit`, `acceptance`, `parity`; `format-check` требует clang-format в PATH или `CLANG_FORMAT=<путь>`.

Критерий успеха прогона: каждый тестовый бинарник завершился кодом 0 и напечатал финальную строку All tests PASSED. Отсутствие финальной строки означает, что процесс был убит до конца набора.
Любой ненулевой код возврата означает дефект - в программе или в тестах; ненулевой код означает, что прогон упал, независимо от того, сколько строк PASS было напечатано. Тег на такой ревизии не навешивается.
Debug сборка включает AddressSanitizer: он обнаруживает обращения за границы буферов, использование освобождённой памяти и утечки. При обнаружении процесс завершается ненулевым кодом. Это всегда подлежит разбору.


### Эталонные программы

Эталонные программы представляют из себя независимые реализации для acceptance-тестов


| Программа | Файл | Назначение |
|---|---|---|
| `ref_gradient` | `01-image-gen/ref/ref_gradient.c` | PPM gradient 3x3 |
| `ref_checker` | `01-image-gen/ref/ref_checker.c` | PPM checker 3x3 |
| `ref_radial` | `01-image-gen/ref/ref_radial.c` | PPM radial 3x3 |
| `ref_gradient_3_3_grayscale` | `04-image-filter/ref/ref_gradient_3_3_grayscale.c` | PPM gradient 3×3, `--grayscale` |
| `ref_gradient_3_3_threshold_128` | `04-image-filter/ref/ref_gradient_3_3_threshold_128.c` | PPM gradient 3×3, `--threshold 128` |
| `ref_radial_3_3_grayscale` | `04-image-filter/ref/ref_radial_3_3_grayscale.c` | PPM radial 3×3, `--grayscale` |
| `ref_probe_2_2` | `04-image-filter/ref/ref_probe_2_2.c` | вход probe 2×2 |
| `ref_probe_2_2_grayscale` | `04-image-filter/ref/ref_probe_2_2_grayscale.c` | PPM probe 2×2, `--grayscale` |
| `ref_probe_2_2_threshold_100` | `04-image-filter/ref/ref_probe_2_2_threshold_100.c` | PPM probe 2×2, `--threshold 100` |
| `ref_probe_3_3` | `04-image-filter/ref/ref_probe_3_3.c` | вход probe 3×3 |
| `ref_probe_3_3_grayscale` | `04-image-filter/ref/ref_probe_3_3_grayscale.c` | PPM probe 3×3, `--grayscale` |
| `ref_probe_3_3_threshold_100` | `04-image-filter/ref/ref_probe_3_3_threshold_100.c` | PPM probe 3×3, `--threshold 100` |
| `ref_stats` | `03-image-stats/ref/ref_stats.c` | Статистика PPM из stdin |
| `ref_passport <case>` | `02-image-passport/ref/ref_passport.c` | Паспорт: эталонный вывод по кейсу |
| `ref_passport_input <case>` | `02-image-passport/ref/ref_passport_input.c` | Паспорт: входной поток по кейсу |

Эталонные программы собираются как зависимости целей `exes` и `acceptance`; бинарники лежат в `build/<вариант>/<задача>/ref/`.

---

## Приёмочное тестирование
Автоматически выполняется целью `make PLATFORM=windows acceptance` (для Release — с `CONFIG=release`). Вручную действуют правила ниже.
Каталог `test_data/` варианта создаётся целями `acceptance` и `massdata`; ручные примеры ниже исходят из того, что он уже существует.
Для каждой программы, создающей файл, который используется в приемочном тестировании:

Программа должна завершаться с кодом выхода 0.
Созданный файл должен сравниваться с эталонным файлом с помощью утилиты fc (в Windows) или аналогичной утилиты для побайтового сравнения.
Если созданный файл впоследствии используется другой программой, эта программа также должна завершиться успешно.

Для того чтобы приемочный тест считался успешным, должны быть пройдены как проверка кода выхода, так и сравнение файлов.

---

## Задача 1 — Генератор изображений

Генерирует квадратное PPM `P3` `N * N` с паттерном.

### Старый режим (позиционные аргументы)
```
gen_image <N> [pattern]
```
`N` — сторона (1–512). `pattern`: `gradient` (по умолчанию), `checker`, `radial`.

```
gen_image 5 gradient        -> stdout
gen_image 3 radial > out.ppm -> в файл
```

### Режим массовой генерации
```
gen_image --size N [--seed S]
```
`N` — сторона (без ограничения 512). `--seed` — seed ГПСЧ (по умолчанию 42).
Паттерн — `random` (детерминированная случайная заливка).

```
gen_image --size 1024 --seed 42 > big_random.ppm
```

### Справка и версия
```
gen_image --help       -> usage в stdout, exit 0
gen_image --version    -> "gen_image 0.1.7", exit 0
```

### Поведение при ошибках
- Нет аргументов -> exit 64, stderr
- `N` не число / вне диапазона -> exit 64, stderr
- Неизвестный паттерн -> exit 64, stderr
- `--help` / `--version` -> exit 0, текст в stdout
- Успех -> exit 0, PPM в stdout

### Воспроизводимость случайной заливки

Паттерн `random` детерминирован: одна и та же программа с одними и теми же
`--size` и `--seed` всегда порождает побитово одинаковый файл.

Гарантия действует **в пределах одной реализации**. C- и C++-версии генератора
на одном seed дают **разные** изображения, и это ожидаемое поведение, а не регрессия:

- C++-версия использует `std::mt19937`, для которого стандарт строго задаёт
  порождаемую последовательность;
- в C стандарт не предъявляет требований к алгоритму `rand`/`srand`:
  реализация зависит от платформы и версии среды выполнения, поэтому один и тот
  же seed на другой машине дал бы другую картинку и сделал бы эталоны
  невоспроизводимыми. Чтобы этого избежать, C-версия использует собственный
  генератор.

Полная побитовая совместимость двух генераторов не реализована сознательно:
она избыточна. Сравнение реализаций строится не на сопоставлении их генераторов, а на обработке **одного и того же** входного файла обеими
реализациями фильтра.

Практические следствия:

- сравнивая C и C++, подавайте обеим один входной файл; не генерируйте вход
  для каждой реализации её собственным генератором;
- `fc` (Windows) или `cmp` (Linux) между C- и C++-бинарниками на одном seed покажет
  различия — это норма;
- массовые тестовые данные в примерах ниже генерируются C-версией; при замене
  её на C++-версию эталоны, посчитанные для этих данных, перестанут совпадать.

### Сборка

```
make PLATFORM=windows 01-image-gen
make PLATFORM=linux 01-image-gen
```

Собирает обе реализации (C и C++) в `build/msvc-debug-dynamic-lib-address/01-image-gen/{c,cpp}/gen_image.exe`. Release:

```
make PLATFORM=windows CONFIG=release 01-image-gen
make PLATFORM=linux CONFIG=release 01-image-gen
```

Артефакты Release — в `build/msvc-release-dynamic-lib-none/01-image-gen/…`. Полный набор executables и эталонов собирает `make PLATFORM=windows exes`; эталоны также собираются как зависимости `acceptance`.

### Тесты

```
make PLATFORM=windows unit
make PLATFORM=linux unit
make PLATFORM=windows CONFIG=release unit
make PLATFORM=linux CONFIG=release unit
```

`unit` собирает и запускает `ppm_test` обеих реализаций. Release-прогон проверяет поведение без ASan и отладочных проверок.

### Примеры приёмки

Debug-бинарники; в начале блока задаётся каталог варианта.

**PowerShell:**
```powershell
$b = 'build\msvc-debug-dynamic-lib-address'

# gradient 3x3
cmd /c "$b\01-image-gen\ref\ref_gradient.exe > $b\test_data\gradient_3x3.ppm"
cmd /c "$b\01-image-gen\c\gen_image.exe 3 gradient > $b\actual.ppm"
cmd /c fc $b\actual.ppm $b\test_data\gradient_3x3.ppm
$LASTEXITCODE                                        # -> 0

# error-кейсы (только exit-код; stderr — для человека)
& $b\01-image-gen\c\gen_image.exe; $LASTEXITCODE        # -> 64
& $b\01-image-gen\c\gen_image.exe abc; $LASTEXITCODE    # -> 64
& $b\01-image-gen\c\gen_image.exe 0; $LASTEXITCODE      # -> 64

# справка и версия
& $b\01-image-gen\c\gen_image.exe --help; $LASTEXITCODE     # -> 0, usage в stdout
& $b\01-image-gen\c\gen_image.exe --version; $LASTEXITCODE  # -> 0, "gen_image 0.1.7"
```

**cmd:**
```bat
chcp 65001 >nul
set "b=build\msvc-debug-dynamic-lib-address"

rem gradient 3x3
%b%\01-image-gen\ref\ref_gradient.exe > %b%\test_data\gradient_3x3.ppm
%b%\01-image-gen\c\gen_image.exe 3 gradient > %b%\actual.ppm
fc %b%\actual.ppm %b%\test_data\gradient_3x3.ppm
echo %errorlevel%                                        & rem expected 0

rem error-кейсы (только exit-код; stderr — для человека)
%b%\01-image-gen\c\gen_image.exe
echo %errorlevel%                                        & rem expected 64
%b%\01-image-gen\c\gen_image.exe abc
echo %errorlevel%                                        & rem expected 64
%b%\01-image-gen\c\gen_image.exe 0
echo %errorlevel%                                        & rem expected 64

rem справка и версия
%b%\01-image-gen\c\gen_image.exe --help
echo %errorlevel%                                        & rem expected 0, usage в stdout
%b%\01-image-gen\c\gen_image.exe --version
echo %errorlevel%                                        & rem expected 0, "gen_image 0.1.7"
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address

# gradient 3x3
"$b/01-image-gen/ref/ref_gradient" > "$b/test_data/gradient_3x3.ppm"
"$b/01-image-gen/c/gen_image" 3 gradient > "$b/actual.ppm"
cmp "$b/actual.ppm" "$b/test_data/gradient_3x3.ppm"
echo $?                                             # -> 0

# error-кейсы (только exit-код; stderr — для человека)
"$b/01-image-gen/c/gen_image"; echo $?               # -> 64
"$b/01-image-gen/c/gen_image" abc; echo $?           # -> 64
"$b/01-image-gen/c/gen_image" 0; echo $?             # -> 64

# справка и версия
"$b/01-image-gen/c/gen_image" --help; echo $?        # -> 0, usage в stdout
"$b/01-image-gen/c/gen_image" --version; echo $?     # -> 0, "gen_image 0.1.7"
```

Release — тот же сценарий с `$b = 'build\msvc-release-dynamic-lib-none'` / `set "b=build\msvc-release-dynamic-lib-none"` / `b=build/gcc-release-dynamic-lib-none`.

Приёмка целиком автоматизирована: `make PLATFORM=windows acceptance` — сравнение с эталонами, проверка exit-кодов и непустого stderr.

## Задача 2 — Паспорт изображения

Запрашивает название и число пикселей, выводит фразу с правильным склонением
«пиксель/пикселя/пикселей».

```
read_passport
> Введите название изображения: морской закат
> Введите количество пикселей: 1920
> Изображение «морской закат»: 1920 пикселей.
```

### Классы ошибок
- Пустой ввод / EOF -> exit 66
- Пустое имя / не-число / отрицательное -> exit 65
- IO-сбой -> exit 74

### Сборка

```
make PLATFORM=windows 02-image-passport
make PLATFORM=linux 02-image-passport
make PLATFORM=windows CONFIG=release 02-image-passport
make PLATFORM=linux CONFIG=release 02-image-passport
```

Артефакты: `build/msvc-debug-dynamic-lib-address/02-image-passport/{c,cpp}/passport.exe` и Release-вариант в `build/msvc-release-dynamic-lib-none/…`.

### Тесты

```
make PLATFORM=windows unit
make PLATFORM=linux unit
make PLATFORM=windows CONFIG=release unit
make PLATFORM=linux CONFIG=release unit
```

`unit` собирает и запускает `read_passport_test` обеих реализаций.

### Примеры приёмки

Debug-бинарники; вход пишет `ref_passport_input` (не зависит от `echo` и кодировки консоли).

**PowerShell:**
```powershell
$b = 'build\msvc-debug-dynamic-lib-address'

# success-кейс: два слова + 1920
cmd /c "$b\02-image-passport\ref\ref_passport_input.exe basic > $b\test_data\passport_basic_in.txt"
cmd /c "$b\02-image-passport\ref\ref_passport.exe basic > $b\expected.txt"
cmd /c "$b\02-image-passport\cpp\passport.exe < $b\test_data\passport_basic_in.txt > $b\actual.txt"
cmd /c fc $b\actual.txt $b\expected.txt
$LASTEXITCODE                                        # -> 0

# error: пустое имя
cmd /c "$b\02-image-passport\ref\ref_passport_input.exe empty_name > $b\test_data\passport_empty_name_in.txt"
cmd /c "$b\02-image-passport\cpp\passport.exe < $b\test_data\passport_empty_name_in.txt > $b\actual.txt"
$LASTEXITCODE                                        # -> 65
```

**cmd:**
```bat
chcp 65001 >nul
set "b=build\msvc-debug-dynamic-lib-address"

rem success-кейс: два слова + 1920
%b%\02-image-passport\ref\ref_passport_input.exe basic > %b%\test_data\passport_basic_in.txt
%b%\02-image-passport\ref\ref_passport.exe basic > %b%\expected.txt
%b%\02-image-passport\cpp\passport.exe < %b%\test_data\passport_basic_in.txt > %b%\actual.txt
fc %b%\actual.txt %b%\expected.txt
echo %errorlevel%                                    & rem expected 0

rem error: пустое имя
%b%\02-image-passport\ref\ref_passport_input.exe empty_name > %b%\test_data\passport_empty_name_in.txt
%b%\02-image-passport\cpp\passport.exe < %b%\test_data\passport_empty_name_in.txt > %b%\actual.txt
echo %errorlevel%                                    & rem expected 65
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address

# success-кейс: два слова + 1920
"$b/02-image-passport/ref/ref_passport_input" basic > "$b/test_data/passport_basic_in.txt"
"$b/02-image-passport/ref/ref_passport" basic > "$b/expected.txt"
"$b/02-image-passport/cpp/passport" < "$b/test_data/passport_basic_in.txt" > "$b/actual.txt"
cmp "$b/actual.txt" "$b/expected.txt"
echo $?                                             # -> 0

# error: пустое имя
"$b/02-image-passport/ref/ref_passport_input" empty_name > "$b/test_data/passport_empty_name_in.txt"
"$b/02-image-passport/cpp/passport" < "$b/test_data/passport_empty_name_in.txt" > "$b/actual.txt"
echo $?                                             # -> 65
```

Release — тот же сценарий с `$b = 'build\msvc-release-dynamic-lib-none'` / `set "b=build\msvc-release-dynamic-lib-none"` / `b=build/gcc-release-dynamic-lib-none`.

Доступные кейсы (те же имена принимает `ref_passport_input`): `basic`, `single_1`, `plural_2`–`101`–`111`, `empty_name`, `no_input`, `bad_count`, `negative`, `zero`. Полностью автоматизировано: `make PLATFORM=windows acceptance`.

## Задача 3 — Статистика изображения

Читает PPM `P3` из stdin до EOF, выводит:
- размеры (`W*H`)
- число пикселей
- средний цвет (округлённый)
- мин./макс. яркость (luma)
- гистограмму яркости (8 корзин)

Работает в конвейере (Debug-бинарники; каталог варианта — см. «Сборка»):
```bat
set "b=build\msvc-debug-dynamic-lib-address"
%b%\01-image-gen\c\gen_image.exe 64 gradient | %b%\03-image-stats\c\image_stats.exe
%b%\01-image-gen\c\gen_image.exe --size 1024 --seed 42 | %b%\03-image-stats\c\image_stats.exe > report.txt
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address
"$b/01-image-gen/c/gen_image" 64 gradient | "$b/03-image-stats/c/image_stats"
"$b/01-image-gen/c/gen_image" --size 1024 --seed 42 | "$b/03-image-stats/c/image_stats" > report.txt
```

### Классы ошибок
- Пустой ввод -> exit 66
- IO-сбой -> exit 74
- Битый формат, не-число, `#` в данных, лишние/недостающие пиксели -> exit 65

### Сборка

```
make PLATFORM=windows 03-image-stats
make PLATFORM=linux 03-image-stats
make PLATFORM=windows CONFIG=release 03-image-stats
make PLATFORM=linux CONFIG=release 03-image-stats
```

Артефакты: `build/<вариант>/03-image-stats/{c,cpp}/image_stats.exe`.

### Тесты

```
make PLATFORM=windows unit
make PLATFORM=linux unit
make PLATFORM=windows CONFIG=release unit
make PLATFORM=linux CONFIG=release unit
```

### Примеры приёмки

Конвейер: `gen_image | image_stats`; эталон — `ref_gradient | ref_stats`.

**PowerShell:**
```powershell
$b = 'build\msvc-debug-dynamic-lib-address'

# эталон
cmd /c "$b\01-image-gen\ref\ref_gradient.exe | $b\03-image-stats\ref\ref_stats.exe > $b\test_data\stats_gradient_3x3.txt"
# прогон
cmd /c "$b\01-image-gen\c\gen_image.exe 3 gradient | $b\03-image-stats\c\image_stats.exe > $b\actual.txt"
# сравнение
cmd /c fc $b\actual.txt $b\test_data\stats_gradient_3x3.txt
$LASTEXITCODE                                        # -> 0

# error-кейсы (только exit-код)
cmd /c "$b\03-image-stats\c\image_stats.exe < nul"
$LASTEXITCODE                                        # -> 66
cmd /c "echo P5 | $b\03-image-stats\c\image_stats.exe"
$LASTEXITCODE                                        # -> 65
```

**cmd:**
```bat
chcp 65001 >nul
set "b=build\msvc-debug-dynamic-lib-address"

rem эталон
%b%\01-image-gen\ref\ref_gradient.exe | %b%\03-image-stats\ref\ref_stats.exe > %b%\test_data\stats_gradient_3x3.txt
rem прогон
%b%\01-image-gen\c\gen_image.exe 3 gradient | %b%\03-image-stats\c\image_stats.exe > %b%\actual.txt
rem сравнение
fc %b%\actual.txt %b%\test_data\stats_gradient_3x3.txt
echo %errorlevel%                                    & rem expected 0

rem error-кейсы (только exit-код)
%b%\03-image-stats\c\image_stats.exe < nul
echo %errorlevel%                                    & rem expected 66
echo P5 | %b%\03-image-stats\c\image_stats.exe
echo %errorlevel%                                    & rem expected 65
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address

# эталон
"$b/01-image-gen/ref/ref_gradient" | "$b/03-image-stats/ref/ref_stats" > "$b/test_data/stats_gradient_3x3.txt"
# прогон
"$b/01-image-gen/c/gen_image" 3 gradient | "$b/03-image-stats/c/image_stats" > "$b/actual.txt"
# сравнение
cmp "$b/actual.txt" "$b/test_data/stats_gradient_3x3.txt"
echo $?                                             # -> 0

# error-кейсы (только exit-код)
"$b/03-image-stats/c/image_stats" < /dev/null
echo $?                                             # -> 66
echo P5 | "$b/03-image-stats/c/image_stats"
echo $?                                             # -> 65
```

Release — тот же сценарий с `$b = 'build\msvc-release-dynamic-lib-none'` / `b=build/gcc-release-dynamic-lib-none`. Автоматически: `make PLATFORM=windows acceptance` (на Linux — `PLATFORM=linux`).

## Задача 4 — Фильтр изображения

Применяет преобразование к PPM `P3`, читает из stdin, пишет валидный PPM
в stdout. Использует общий модуль `common/ppm_io` (парсинг заголовка,
буфер пикселей, потоковую запись через `PpmWriter`).

Работает в конвейере (Debug-бинарники; каталог варианта — см. «Сборка»):
```bat
set "b=build\msvc-debug-dynamic-lib-address"
%b%\01-image-gen\c\gen_image.exe 64 gradient | %b%\04-image-filter\c\filter.exe --grayscale
%b%\01-image-gen\c\gen_image.exe 4 gradient | %b%\04-image-filter\c\filter.exe --threshold 128 | %b%\03-image-stats\c\image_stats.exe
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address
"$b/01-image-gen/c/gen_image" 64 gradient | "$b/04-image-filter/c/filter" --grayscale
"$b/01-image-gen/c/gen_image" 4 gradient | "$b/04-image-filter/c/filter" --threshold 128 | "$b/03-image-stats/c/image_stats"
```

### Режимы

```
filter --grayscale      -> конверсия в оттенки серого по luma
filter --threshold T    -> бинаризация по порогу яркости (0 <= T <= 255)
```

- `--grayscale`: `y = round(0.299·R + 0.587·G + 0.114·B)`, пиксель -> `(y, y, y)`
- `--threshold T`: пиксель с `luma > T` -> белый `(255,255,255)`, иначе чёрный `(0,0,0)`

### Справка и версия
```
filter --help       -> usage в stdout, exit 0 (stdin не читается)
filter --version    -> "filter 0.1.7", exit 0
```

### Классы ошибок
- Без аргументов / неизвестный режим / `--threshold` без T / T не число или вне `[0; 255]` -> exit 64, stderr
- `--help` / `--version` -> exit 0, текст в stdout
- Пустой ввод -> exit 66
- Битый формат PPM (из `ppm_io`) -> exit 65
- IO-сбой -> exit 74


### Сборка

```
make PLATFORM=windows 04-image-filter
make PLATFORM=linux 04-image-filter
make PLATFORM=windows CONFIG=release 04-image-filter
make PLATFORM=linux CONFIG=release 04-image-filter
```

Артефакты: `build/<вариант>/04-image-filter/{c,cpp}/filter.exe`.

### Тесты

```
make PLATFORM=windows unit
make PLATFORM=linux unit
make PLATFORM=windows CONFIG=release unit
make PLATFORM=linux CONFIG=release unit
```

### Примеры приёмки

Эталоны и probe-входы собираются целью `exes` (и как зависимости `acceptance`); отдельной ручной сборки `cl`/`link` больше не требуется.

**Часть 1. Паттерн первой задачи.**

Debug — PowerShell:
```powershell
$b = 'build\msvc-debug-dynamic-lib-address'
cmd /c "$b\01-image-gen\ref\ref_radial.exe > $b\test_data\radial_3x3.ppm"
cmd /c "$b\04-image-filter\ref\ref_radial_3_3_grayscale.exe > $b\test_data\radial_3x3_grayscale.ppm"
cmd /c "$b\04-image-filter\cpp\filter.exe --grayscale < $b\test_data\radial_3x3.ppm > $b\actual_filter.ppm"
cmd /c fc $b\actual_filter.ppm $b\test_data\radial_3x3_grayscale.ppm
$LASTEXITCODE                                        # -> 0
```

Debug — cmd:
```bat
chcp 65001 >nul
set "b=build\msvc-debug-dynamic-lib-address"
%b%\01-image-gen\ref\ref_radial.exe > %b%\test_data\radial_3x3.ppm
%b%\04-image-filter\ref\ref_radial_3_3_grayscale.exe > %b%\test_data\radial_3x3_grayscale.ppm
%b%\04-image-filter\cpp\filter.exe --grayscale < %b%\test_data\radial_3x3.ppm > %b%\actual_filter.ppm
fc %b%\actual_filter.ppm %b%\test_data\radial_3x3_grayscale.ppm
echo %errorlevel%                                    & rem expected 0
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address
"$b/01-image-gen/ref/ref_radial" > "$b/test_data/radial_3x3.ppm"
"$b/04-image-filter/ref/ref_radial_3_3_grayscale" > "$b/test_data/radial_3x3_grayscale.ppm"
"$b/04-image-filter/cpp/filter" --grayscale < "$b/test_data/radial_3x3.ppm" > "$b/actual_filter.ppm"
cmp "$b/actual_filter.ppm" "$b/test_data/radial_3x3_grayscale.ppm"
echo $?                                             # -> 0
```

Release — тот же сценарий с `$b = 'build\msvc-release-dynamic-lib-none'` / `b=build/gcc-release-dynamic-lib-none`.

**Часть 2. Probe 3×3.**

Debug — PowerShell:
```powershell
$b = 'build\msvc-debug-dynamic-lib-address'
cmd /c "$b\04-image-filter\ref\ref_probe_3_3.exe > $b\test_data\probe_3x3.ppm"
cmd /c "$b\04-image-filter\ref\ref_probe_3_3_grayscale.exe > $b\test_data\probe_3x3_grayscale.ppm"
cmd /c "$b\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > $b\test_data\probe_3x3_threshold_100.ppm"

cmd /c "$b\04-image-filter\cpp\filter.exe --grayscale < $b\test_data\probe_3x3.ppm > $b\actual_filter.ppm"
cmd /c fc $b\actual_filter.ppm $b\test_data\probe_3x3_grayscale.ppm
$LASTEXITCODE                                        # -> 0
cmd /c "$b\04-image-filter\cpp\filter.exe --threshold 100 < $b\test_data\probe_3x3.ppm > $b\actual_filter.ppm"
cmd /c fc $b\actual_filter.ppm $b\test_data\probe_3x3_threshold_100.ppm
$LASTEXITCODE                                        # -> 0
```

Debug — cmd:
```bat
chcp 65001 >nul
set "b=build\msvc-debug-dynamic-lib-address"
%b%\04-image-filter\ref\ref_probe_3_3.exe > %b%\test_data\probe_3x3.ppm
%b%\04-image-filter\ref\ref_probe_3_3_grayscale.exe > %b%\test_data\probe_3x3_grayscale.ppm
%b%\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > %b%\test_data\probe_3x3_threshold_100.ppm

%b%\04-image-filter\cpp\filter.exe --grayscale < %b%\test_data\probe_3x3.ppm > %b%\actual_filter.ppm
fc %b%\actual_filter.ppm %b%\test_data\probe_3x3_grayscale.ppm
echo %errorlevel%                                    & rem expected 0
%b%\04-image-filter\cpp\filter.exe --threshold 100 < %b%\test_data\probe_3x3.ppm > %b%\actual_filter.ppm
fc %b%\actual_filter.ppm %b%\test_data\probe_3x3_threshold_100.ppm
echo %errorlevel%                                    & rem expected 0
```

**Linux (bash):**
```bash
b=build/gcc-debug-dynamic-lib-address
"$b/04-image-filter/ref/ref_probe_3_3" > "$b/test_data/probe_3x3.ppm"
"$b/04-image-filter/ref/ref_probe_3_3_grayscale" > "$b/test_data/probe_3x3_grayscale.ppm"
"$b/04-image-filter/ref/ref_probe_3_3_threshold_100" > "$b/test_data/probe_3x3_threshold_100.ppm"

"$b/04-image-filter/cpp/filter" --grayscale < "$b/test_data/probe_3x3.ppm" > "$b/actual_filter.ppm"
cmp "$b/actual_filter.ppm" "$b/test_data/probe_3x3_grayscale.ppm"
echo $?                                             # -> 0
"$b/04-image-filter/cpp/filter" --threshold 100 < "$b/test_data/probe_3x3.ppm" > "$b/actual_filter.ppm"
cmp "$b/actual_filter.ppm" "$b/test_data/probe_3x3_threshold_100.ppm"
echo $?                                             # -> 0
```

Release — тот же сценарий с `$b = 'build\msvc-release-dynamic-lib-none'` / `b=build/gcc-release-dynamic-lib-none`. Полностью автоматизировано: `make PLATFORM=windows acceptance` (7 кейсов фильтра; на Linux — `PLATFORM=linux`).

## Управление выделением памяти

По умолчанию библиотека выделяет память под изображение стандартными средствами языка.
`ppm_read` в C и `Image::read(is)` в C++ работают так же, как раньше, и менять существующий
код не требуется.

Дополнительно можно указать собственный источник памяти. Это нужно, когда стандартная куча
не подходит: для арены или монотонного ресурса на время обработки, для пула фиксированных
блоков, для ресурса поверх отображённого в память файла, для ограничения суммарного объёма
сверху. Библиотека при этом не меняется — меняется только то, что ей передают.

### C

```c

#include "ppm_io_alloc.h"

struct PpmAllocator {
    void* (*alloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void  (*free)(void* ptr);
};

struct PpmResult r = ppm_read_with(f, &my_allocator);
if (r.error == PRE_OK) {
    /* ... */
    ppm_image_free_with(&r.image, &my_allocator);
}
```

Контракт:

- указатель на аллокатор не может быть нулевым, все три функции должны быть заданы;
- библиотека обращается только к ним и не вызывает `malloc`, `realloc` и `free` напрямую;
- любая из функций может вернуть `NULL`; в этом случае чтение освобождает всё уже
  захваченное тем же аллокатором и возвращает `PRE_ALLOC_ERROR`;
- освобождение результата — обязанность вызывающего, и оно должно идти **тем же**
  аллокатором: изображение, прочитанное через `ppm_read_with`, освобождается
  `ppm_image_free_with`, а не `ppm_image_free`.

### C++

```cpp
MyMemoryResource mr;
auto r = raster::common::Image::read(is, &mr);
```

Контракт:

- `mr` должен оставаться живым всё время жизни возвращённого изображения: контейнер внутри
  хранит на него указатель и возвращает ему память в деструкторе;
- парная функция освобождения не нужна — изображение освобождает себя само;
- при отказе выделения `read` не бросает исключение, а возвращает ошибку `kAllocError`,
  как и остальные ошибки чтения.

### Отличие двух реализаций

В C владение памятью остаётся за вызывающим, поэтому пара «читать своим аллокатором —
освобождать своим аллокатором» обязательна. В C++ владение закрыто временем жизни объекта,
поэтому вместо парной функции в контракт вынесено требование к времени жизни ресурса.


## Модуль `common/ppm_io` и сборка DLL

Задачи 1/3/4 используют общий модуль ввода-вывода PPM. Способ сборки задаётся осью `LINK`:

- `LINK=obj` — объектные файлы линкуются напрямую;
- `LINK=lib` — статическая библиотека (по умолчанию);
- `LINK=dll` — динамическая библиотека + import-lib; make копирует DLL рядом с приложениями, иначе при запуске будет ошибка разрешения символов.

### Сборка DLL (`LINK=dll`)

Общий модуль собирается как динамическая библиотека одной осью варианта:

```
make PLATFORM=windows LINK=dll exes
```

Make собирает `ppm_io.dll` и import-lib для C и C++, линкует с ними приложения-потребители и копирует DLL рядом с exe — ручное копирование не требуется. Release:

```
make PLATFORM=windows LINK=dll CONFIG=release exes
```

Артефакты: `build/msvc-debug-dynamic-dll-address/common/{c,cpp}/dll/ppm_io.{dll,lib}` и копии рядом с приложениями.

Ограничения: `LINK=dll` требует `RUNTIME=dynamic` (через границу идут `FILE*` и `std::istream`); на GCC `LINK=dll` не поддерживается.

Макрос экспорта: `KV_API` = `__declspec(dllexport)` на Windows при `KV_DYNAMIC_LINK`, иначе пусто. `dllimport` не используется: потребитель, линкующий import-lib, работает без него.

В C++ `Image` реализован как PIMPL-класс (move-only): поля скрыты за `std::unique_ptr<Impl>`, публичный API — через аксессоры (`width()`, `height()`, `pixel_count()`, `pixels()`). Это даёт стабильный layout объекта и снимает зависимость от `<vector>` в заголовке.

### Номер строки ошибки

Результат чтения (`struct PpmResult` в C, `struct PpmResult` в C++)
содержит номер строки, на которой возникла ошибка парсинга: поле
`error_line` (C) / `line` (C++). Диагностическое сообщение (`diagnostic`)
по-прежнему содержит и текст вида «строка N: …» — поле добавлено для
программного доступа к номеру строки без разбора текста.

---

## Тесты общих модулей

Логика общих модулей (`ppm_io`, `luma`, `int_token`) тестируется на уровне модуля; юнит-тесты задач покрывают только специфичную для задачи логику.

Запуск — в составе общей цели:

```
make PLATFORM=windows unit
make PLATFORM=linux unit
make PLATFORM=windows CONFIG=release unit
make PLATFORM=linux CONFIG=release unit
```

`unit` собирает и прогоняет `ppm_io_test` для C и C++: тесты `int_token` есть только в C-версии (в C++ эту роль выполняет `std::from_chars`), magic проверяется в обеих.

---

## Массовые тестовые данные

Данные для замеров и ручных прогонов лежат в `build/<вариант>/test_data/` (в `.gitignore`), в репозиторий не попадают. Каталог создаётся make; `make clean` удаляет его вместе с вариантом.

### Случайные входы (random)

```
make PLATFORM=windows massdata
make PLATFORM=linux massdata
```

Цель создаёт 6 PPM `random_<N>x<N>_seed<S>.ppm` для `N = {2, 64, 1024}` и `S = {42, 9999}` — C-версией генератора. Повторный запуск без изменений ничего не пересобирает; удалённый файл генерируется заново. Для замеров удобнее Release-вариант:

```
make PLATFORM=windows CONFIG=release massdata
make PLATFORM=linux CONFIG=release massdata
```

### Эталонные файлы

Независимые эталоны и входы генерируются как зависимости приёмки:

```
make PLATFORM=windows acceptance
make PLATFORM=linux acceptance
```

Цель создаёт эталоны задач 1–4 и probe-входы в `build/<вариант>/test_data/ref/` (имена — по таблице «Эталонные программы»).

---

## Паритет C и C++

C++ — эталон поведения, C приводится к нему. Совпадение результатов и диагностики — часть контракта: `make PLATFORM=windows parity` побайтово сравнивает stdout успешных кейсов. Тексты stderr ошибочных кейсов и вывод `--help`/`--version` пока сверяются вручную — их автоматическое сравнение ближайшее в бэклоге.

### Разбор чисел

Общее правило для `N`, `--size`, `--seed`, количества пикселей и порога: `-?[0-9]+`.

| Вход | Результат |
|---|---|
| `5`, `0005`, `-5` | лексически корректно; отрицательное значение затем отвергается проверкой диапазона |
| `+5`, `" 5"`, `"\t5"`, `""`, `5.0`, `5e3`, `0x10`, `5-` | отказ (не число) |
| `2147483648`, `4294967296` | отказ по диапазону целевого типа |

В C правило реализует `common/c/int_token.h` (`kv_parse_i32`, `kv_parse_u32`, статусы `KV_INT_OK` / `KV_INT_INVALID` / `KV_INT_RANGE`); в C++ — `std::from_chars`. Потребители: задача 1 (`N`, `--size`, `--seed`), задача 2 (количество пикселей), задача 4 (`--threshold`).

### Заголовок PPM

- `P3` принимается, если за ним идёт разделитель: пробел, TAB, CR, LF, `#` или конец файла. `P3\n` и `P3\r\n` валидны.
- `\v` и `\f` разделителями не являются.
- `P3#comment` валиден; `P31`, `P3x`, `P 3` — ошибка.
- В диагностике эхо токена ограничено 127 символами; непечатаемые байты выводятся как `\xNN`; сообщение — до 512 байт.
- Номер строки увеличивает только `\n`.

### Диагностика

Тексты сообщений и exit-коды одинаковы у C и C++ (exit-коды — в разделе «Exit-коды»). Политика закреплена тестами: magic — в `ppm_io_test` обеих реализаций, `int_token` — в C-версии, разбор аргументов — в тестах задач 1/2/4.

### Зависимости сборки

`common/c/int_token.h` общий для трёх задач, но зависимость в Makefile ведётся по конкретным объектам (`INT_TOKEN_HDR`), а не через список заголовков модуля: правка заголовка пересобирает только `parse_args`, `read_passport`, `filter` и `ppm_io_test`, не задевая задачу 3 и саму библиотеку `ppm_io`.

### Известный долг

- текст отказа выделения памяти различается: C сообщает заказанную ёмкость, C++ — размещённое количество;
- тексты сбоя записи: у C одна формулировка на `finish`, у C++ — три (заголовок/пиксель/сброс);
- остаточные расхождения парсера PPM: `-2147483648` в поле, номер строки для комментария на EOF, `\v`/`\f` в хвосте, длина эха токена;
- формулировка «должно быть числом» для значения вне `int32` (оставлена сознательно);
- у `--size` нет потолка: при очень больших значениях — неопределённое поведение в арифметике генератора;
- строка count в задаче 2 ограничена буфером 64 байта (C) против `std::string` (C++).

Генератор `random` в паритет не входит сознательно — см. «Воспроизводимость случайной заливки».

---

## Exit-коды

Определены в `common/exit_codes.h` (.hpp для C++):

| Константа | Код | Назначение |
|---|---|---|
| `EC_OK` | 0 | Успех |
| `EC_USAGE` | 64 | Неверные аргументы |
| `EC_DATA` | 65 | Битый формат / невалидные данные |
| `EC_NOINPUT` | 66 | Пустой ввод / нет данных |
| `EC_IOERR` | 74 | Сбой ввода-вывода |

Код — машинный канал для скриптов-обёрток; stderr — детали для человека.