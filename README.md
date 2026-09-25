# Программа подготовки к стажировке в Inobitec — трек C/C++

Репозиторий с задачами оценочного периода. Домен задачника — растровые
изображения в текстовом формате PPM (`P3` — RGB, тройки ASCII).

## Структура проекта

```
AGENTS.md          — журнал работы и память агента
common/
  c/               — общие модули (C)
    api.h          — макрос для сборки общего модуля в формате библиотеки(lib/imp lib + dll)
    exit_codes.h   — именованные exit-коды
    version.h      — версия программ (`KV_VERSION`)
    ppm_io.h(.c)   — модуль ввода-вывода PPM (парсинг/запись, экспорт в DLL)
    luma.h         — яркость (luma) по каналам (header-only)
    strerror.h(.c) — сообщение системной ошибки, платформонезависимо
  cpp/             — общие модули (C++)
    exit_codes.hpp — именованные exit-коды
    version.hpp    — версия программ (`kVersion`)
    luma.hpp       — яркость (luma) по каналам (header-only)
    ppm_io.hpp(.cpp) — модуль ввода-вывода PPM (PIMPL, экспорт в DLL)
build/
  test_data/       — сгенерированные тестовые PPM (в .gitignore)
  {task}/{c,cpp,ref}/ — артефакты Debug-сборки по задачам (объекты, exe, эталоны)
  release/{task}/{c,cpp,ref}/ — артефакты Release-сборки по задачам
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

- **OS:** Windows
- **Toolchain:** MSVC (Visual Studio Build Tools, `vcvars64.bat`)
- **Сборка:** из консоли, `cl /c` + `link`, без IDE
- **Консоль:** PowerShell / FAR Manager / cmd, кодировка UTF-8 (`chcp 65001`)
- **Просмотр PPM:** IrfanView, GIMP, ImageMagick (`magick out.ppm out.png`)

**Важно для Windows:** команды с перенаправлением ввода-вывода (`<`, `>`) должны выполняться в **Command Prompt (cmd.exe)**, а не в PowerShell.
Windows PowerShell 5.1 перекодирует вывод при перенаправлении `>`, из-за чего эталонные файлы могут получиться в формате, отличном от ожидаемого программой; при этом сравнение двух одинаково испорченных файлов не покажет расхождения.
Чтобы блок команд был исполним целиком в любой оболочке, для каждого shell-зависимого блока приведены два варианта:
- **PowerShell** — команды с перенаправлением обёрнуты в `cmd /c "…"`, проверка результата — `$LASTEXITCODE`;
- **cmd** — блок начинается с `chcp 65001 >nul`, проверка результата — `%errorlevel%`.
Блоки сборки (`cl`, `link`) одинаковы для обеих оболочек и не дублируются.

---

## Выпуск релиза
Перед тем, как присвоить тег по semversion коду, реализуется следующая последовательность действий:
1. Полная проверка по сценарию README (начиная с чистого клона репозитория): сборка и выполнение всех тестов (включая примеры для задачи 1 и модуль common/ppm_io) - без частичных или инкрементальных запусков.
2. Проверка идентичности поведения при сбоях: выполнение сборок на C и C++ с использованием фиксированного набора входных данных, провоцирующих ошибки (например, заголовки чрезмерного размера или потребитель, преждевременно закрывающий канал), и подтверждение того, что обе реализации завершаются одинаково (совпадение класса кода возврата и непустые диагностические сообщения, которые одинаково описывают возникшую ошибку), а не просто совпадение результатов при успешном выполнении.
3. Сверка CHANGELOG на основе diff: поочерёдный анализ коммитов в диапазоне git log <last-tag>..HEAD и сопоставление их с записями в CHANGELOG для подтверждения того, что каждое изменение отражено в файле.
4. Проверка согласованности версий: подтверждение того, что вывод команды --version совпадает с создаваемым тегом и версией, указанной в заголовке CHANGELOG.

---
## Сборка

Установить окружение:
```
vcvars64.bat
```

Все команды выполняются из корня репозитория; cd внутри команд не используется. Сборка содержит раздельные этапы.

Создать каталоги артефактов (один раз; повторный запуск безопасен).

**PowerShell:**
```powershell
New-Item -ItemType Directory -Force -Path build/test_data, build/common/c, build/common/cpp, build/01-image-gen/c, build/01-image-gen/cpp, build/01-image-gen/ref, build/02-image-passport/c, build/02-image-passport/cpp, build/02-image-passport/ref, build/03-image-stats/c, build/03-image-stats/cpp, build/03-image-stats/ref, build/04-image-filter/c, build/04-image-filter/cpp, build/04-image-filter/ref
New-Item -ItemType Directory -Force -Path build/release/common/c, build/release/common/cpp, build/release/01-image-gen/c, build/release/01-image-gen/cpp, build/release/01-image-gen/ref, build/release/02-image-passport/c, build/release/02-image-passport/cpp, build/release/02-image-passport/ref, build/release/03-image-stats/c, build/release/03-image-stats/cpp, build/release/03-image-stats/ref, build/release/04-image-filter/c, build/release/04-image-filter/cpp, build/release/04-image-filter/ref
```

**cmd:**
```bat
chcp 65001 >nul
mkdir build\test_data build\common\c build\common\cpp build\01-image-gen\c build\01-image-gen\cpp build\01-image-gen\ref build\02-image-passport\c build\02-image-passport\cpp build\02-image-passport\ref build\03-image-stats\c build\03-image-stats\cpp build\03-image-stats\ref build\04-image-filter\c build\04-image-filter\cpp build\04-image-filter\ref 2>nul
mkdir build\release\common\c build\release\common\cpp build\release\01-image-gen\c build\release\01-image-gen\cpp build\release\01-image-gen\ref build\release\02-image-passport\c build\release\02-image-passport\cpp build\release\02-image-passport\ref build\release\03-image-stats\c build\release\03-image-stats\cpp build\release\03-image-stats\ref build\release\04-image-filter\c build\release\04-image-filter\cpp build\release\04-image-filter\ref 2>nul
```

Общие флаги: `/std:c17` (C) или `/std:c++latest` (C++), `/W4 /permissive- /utf-8`; для C++ дополнительно `/EHsc`.

Debug + ASan (стартовый режим, § 4.7): `/Od /Zi /MDd /fsanitize=address`.

Release (замеры и поставка): компиляция `/O2 /Zi /DNDEBUG /MD`, линковка `/DEBUG /OPT:REF /OPT:ICF`.

Debug-артефакты размещаются в `build/`, Release-артефакты — в `build/release/`. Каталоги раздельны: объектные файлы и PDB разных конфигураций не перезаписывают друг друга.

## Тесты

Критерий успеха прогона: прогон успешен, когда каждый тестовый бинарник завершился кодом 0 и напечатал финальную строку All tests PASSED. Отсутствие финальной строки означает, что процесс был убит до конца набора.
Любой ненулевой код возврата означает дефект - в программе или в тестах. Ненулевой код означает, что прогон упал, независимо от того, сколько строк PASS было напечатано. Тег на такой ревизии не навешивается.
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

Все бинарники — в `build/` (Debug) и `build/release/` (Release).

---

## Приёмочное теистирование 
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
gen_image --version    -> "gen_image 0.1.5", exit 0
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
- `fc` между `gen_image.exe` (C) и `gen_image.exe` (C++) на одном seed покажет
  различия — это норма;
- массовые тестовые данные в примерах ниже генерируются C-версией; при замене
  её на C++-версию эталоны, посчитанные для этих данных, перестанут совпадать.

### Тесты — Debug
Для С:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/patterns.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/hsv_to_rgb.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/ppm_test.c
link /DEBUG build/01-image-gen/c/patterns.obj build/01-image-gen/c/parse_args.obj build/01-image-gen/c/hsv_to_rgb.obj build/01-image-gen/c/ppm_test.obj /OUT:build/01-image-gen/c/ppm_test.exe
build\01-image-gen\c\ppm_test.exe
```

Для С++:
```
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/patterns.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/parse_args.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/hsv_to_rgb.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/ppm_test.cpp
link /DEBUG build/01-image-gen/cpp/patterns.obj build/01-image-gen/cpp/parse_args.obj build/01-image-gen/cpp/hsv_to_rgb.obj build/01-image-gen/cpp/ppm_test.obj /OUT:build/01-image-gen/cpp/ppm_test.exe
build\01-image-gen\cpp\ppm_test.exe
```

### Тесты — Release
Для С:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/patterns.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/hsv_to_rgb.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/ppm_test.c
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/c/patterns.obj build/release/01-image-gen/c/parse_args.obj build/release/01-image-gen/c/hsv_to_rgb.obj build/release/01-image-gen/c/ppm_test.obj /OUT:build/release/01-image-gen/c/ppm_test.exe
build\release\01-image-gen\c\ppm_test.exe
```

Для С++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/patterns.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/parse_args.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/hsv_to_rgb.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/ppm_test.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/cpp/patterns.obj build/release/01-image-gen/cpp/parse_args.obj build/release/01-image-gen/cpp/hsv_to_rgb.obj build/release/01-image-gen/cpp/ppm_test.obj /OUT:build/release/01-image-gen/cpp/ppm_test.exe
build\release\01-image-gen\cpp\ppm_test.exe
```


Acceptance — ручной прогон с эталоном. Debug-бинарники.

**PowerShell:**
```powershell
# gradient 3x3
cmd /c "build\01-image-gen\ref\ref_gradient.exe > build\test_data\gradient_3x3.ppm"
cmd /c "build\01-image-gen\c\gen_image.exe 3 gradient > build\actual.ppm"
cmd /c fc build\actual.ppm build\test_data\gradient_3x3.ppm
$LASTEXITCODE                                        # -> 0

# error-кейсы (только exit-код; stderr — для человека)
build\01-image-gen\c\gen_image.exe; $LASTEXITCODE        # -> 64
build\01-image-gen\c\gen_image.exe abc; $LASTEXITCODE    # -> 64
build\01-image-gen\c\gen_image.exe 0; $LASTEXITCODE      # -> 64

# справка и версия
build\01-image-gen\c\gen_image.exe --help; $LASTEXITCODE     # -> 0, usage в stdout
build\01-image-gen\c\gen_image.exe --version; $LASTEXITCODE  # -> 0, "gen_image 0.1.5"
```

**cmd:**
```bat
chcp 65001 >nul

rem gradient 3x3
build\01-image-gen\ref\ref_gradient.exe > build\test_data\gradient_3x3.ppm
build\01-image-gen\c\gen_image.exe 3 gradient > build\actual.ppm
fc build\actual.ppm build\test_data\gradient_3x3.ppm
echo %errorlevel%                                        & rem expected 0

rem error-кейсы (только exit-код; stderr — для человека)
build\01-image-gen\c\gen_image.exe
echo %errorlevel%                                        & rem expected 64
build\01-image-gen\c\gen_image.exe abc
echo %errorlevel%                                        & rem expected 64
build\01-image-gen\c\gen_image.exe 0
echo %errorlevel%                                        & rem expected 64

rem справка и версия
build\01-image-gen\c\gen_image.exe --help
echo %errorlevel%                                        & rem expected 0, usage в stdout
build\01-image-gen\c\gen_image.exe --version
echo %errorlevel%                                        & rem expected 0, "gen_image 0.1.5"
```

Release-бинарники (`build\release\...`).

**PowerShell:**
```powershell
cmd /c "build\release\01-image-gen\ref\ref_gradient.exe > build\test_data\gradient_3x3.ppm"
cmd /c "build\release\01-image-gen\c\gen_image.exe 3 gradient > build\actual.ppm"
cmd /c fc build\actual.ppm build\test_data\gradient_3x3.ppm
$LASTEXITCODE                                        # -> 0

build\release\01-image-gen\c\gen_image.exe; $LASTEXITCODE        # -> 64
build\release\01-image-gen\c\gen_image.exe abc; $LASTEXITCODE    # -> 64
build\release\01-image-gen\c\gen_image.exe 0; $LASTEXITCODE      # -> 64

build\release\01-image-gen\c\gen_image.exe --help; $LASTEXITCODE     # -> 0
build\release\01-image-gen\c\gen_image.exe --version; $LASTEXITCODE  # -> 0
```

**cmd:**
```bat
chcp 65001 >nul

rem gradient 3x3
build\release\01-image-gen\ref\ref_gradient.exe > build\test_data\gradient_3x3.ppm
build\release\01-image-gen\c\gen_image.exe 3 gradient > build\actual.ppm
fc build\actual.ppm build\test_data\gradient_3x3.ppm
echo %errorlevel%                                        & rem expected 0

rem error-кейсы
build\release\01-image-gen\c\gen_image.exe
echo %errorlevel%                                        & rem expected 64
build\release\01-image-gen\c\gen_image.exe abc
echo %errorlevel%                                        & rem expected 64
build\release\01-image-gen\c\gen_image.exe 0
echo %errorlevel%                                        & rem expected 64

rem справка и версия
build\release\01-image-gen\c\gen_image.exe --help
echo %errorlevel%                                        & rem expected 0
build\release\01-image-gen\c\gen_image.exe --version
echo %errorlevel%                                        & rem expected 0
```

### Эталоны — Debug
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/ref/ 01-image-gen/ref/ref_gradient.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/ref/ 01-image-gen/ref/ref_checker.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/ref/ 01-image-gen/ref/ref_radial.c
link /DEBUG build/01-image-gen/ref/ref_gradient.obj /OUT:build/01-image-gen/ref/ref_gradient.exe
link /DEBUG build/01-image-gen/ref/ref_checker.obj /OUT:build/01-image-gen/ref/ref_checker.exe
link /DEBUG build/01-image-gen/ref/ref_radial.obj /OUT:build/01-image-gen/ref/ref_radial.exe
```

### Эталоны — Release
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/ref/ 01-image-gen/ref/ref_gradient.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/ref/ 01-image-gen/ref/ref_checker.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/ref/ 01-image-gen/ref/ref_radial.c
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/ref/ref_gradient.obj /OUT:build/release/01-image-gen/ref/ref_gradient.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/ref/ref_checker.obj /OUT:build/release/01-image-gen/ref/ref_checker.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/ref/ref_radial.obj /OUT:build/release/01-image-gen/ref/ref_radial.exe
```

### Сборка основного приложения — Debug

Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/hsv_to_rgb.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/patterns.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/main.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ common/c/strerror.c
link /DEBUG build/01-image-gen/c/parse_args.obj build/01-image-gen/c/patterns.obj build/01-image-gen/c/hsv_to_rgb.obj build/01-image-gen/c/main.obj build/01-image-gen/c/ppm_io.obj build/01-image-gen/c/strerror.obj /OUT:build/01-image-gen/c/gen_image.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/parse_args.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/hsv_to_rgb.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/patterns.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/main.cpp 
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ common/cpp/ppm_io.cpp
link /DEBUG build/01-image-gen/cpp/patterns.obj build/01-image-gen/cpp/parse_args.obj build/01-image-gen/cpp/hsv_to_rgb.obj build/01-image-gen/cpp/main.obj build/01-image-gen/cpp/ppm_io.obj /OUT:build/01-image-gen/cpp/gen_image.exe
```

### Сборка основного приложения — Release

Для C:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/hsv_to_rgb.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/patterns.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ 01-image-gen/c/main.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/c/ common/c/strerror.c
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/c/parse_args.obj build/release/01-image-gen/c/patterns.obj build/release/01-image-gen/c/hsv_to_rgb.obj build/release/01-image-gen/c/main.obj build/release/01-image-gen/c/ppm_io.obj build/release/01-image-gen/c/strerror.obj /OUT:build/release/01-image-gen/c/gen_image.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/parse_args.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/hsv_to_rgb.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/patterns.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ 01-image-gen/cpp/main.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/01-image-gen/cpp/ common/cpp/ppm_io.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/01-image-gen/cpp/patterns.obj build/release/01-image-gen/cpp/parse_args.obj build/release/01-image-gen/cpp/hsv_to_rgb.obj build/release/01-image-gen/cpp/main.obj build/release/01-image-gen/cpp/ppm_io.obj /OUT:build/release/01-image-gen/cpp/gen_image.exe
```

---

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

### Тесты — Debug

Сборка тестов
Для С:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport_test.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/pixel_word.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ common/c/strerror.c
link /DEBUG build/02-image-passport/c/read_passport_test.obj build/02-image-passport/c/read_passport.obj build/02-image-passport/c/pixel_word.obj build/02-image-passport/c/strerror.obj /OUT:build/02-image-passport/c/passport_tests.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport_test.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/pixel_word.cpp
link /DEBUG build/02-image-passport/cpp/read_passport_test.obj build/02-image-passport/cpp/read_passport.obj build/02-image-passport/cpp/pixel_word.obj /OUT:build/02-image-passport/cpp/passport_tests.exe
```

Юнит-тесты (склонение, `read_passport`, тексты сообщений). `read_passport` принимает `FILE*`, чтобы вход инжектировался в тестах:
```
build\02-image-passport\c\passport_tests.exe 
build\02-image-passport\cpp\passport_tests.exe
```

### Тесты — Release

Для С:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ 02-image-passport/c/read_passport_test.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ 02-image-passport/c/read_passport.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ 02-image-passport/c/pixel_word.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ common/c/strerror.c
link /DEBUG /OPT:REF /OPT:ICF build/release/02-image-passport/c/read_passport_test.obj build/release/02-image-passport/c/read_passport.obj build/release/02-image-passport/c/pixel_word.obj build/release/02-image-passport/c/strerror.obj /OUT:build/release/02-image-passport/c/passport_tests.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/cpp/ 02-image-passport/cpp/read_passport_test.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/cpp/ 02-image-passport/cpp/read_passport.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/cpp/ 02-image-passport/cpp/pixel_word.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/02-image-passport/cpp/read_passport_test.obj build/release/02-image-passport/cpp/read_passport.obj build/release/02-image-passport/cpp/pixel_word.obj /OUT:build/release/02-image-passport/cpp/passport_tests.exe
```

```powershell
build\release\02-image-passport\c\passport_tests.exe 
build\release\02-image-passport\cpp\passport_tests.exe
```

Acceptance — эталон `ref_passport.exe`, ручное сравнение. Вход подаётся файлом, который пишет `ref_passport_input` (не зависит от `echo` и кодировки консоли). Debug-бинарники.

**PowerShell:**
```powershell
# success-кейс: два слова + 1920
cmd /c "build\02-image-passport\ref\ref_passport_input.exe basic > build\test_data\passport_basic_in.txt"
cmd /c "build\02-image-passport\ref\ref_passport.exe basic > build\expected.txt"
cmd /c "build\02-image-passport\cpp\passport.exe < build\test_data\passport_basic_in.txt > build\actual.txt"
cmd /c fc build\actual.txt build\expected.txt
$LASTEXITCODE                                        # -> 0

# error: пустое имя
cmd /c "build\02-image-passport\ref\ref_passport_input.exe empty_name > build\test_data\passport_empty_name_in.txt"
cmd /c "build\02-image-passport\cpp\passport.exe < build\test_data\passport_empty_name_in.txt > build\actual.txt"
$LASTEXITCODE                                        # -> 65
```

**cmd:**
```bat
chcp 65001 >nul

rem success-кейс: два слова + 1920
build\02-image-passport\ref\ref_passport_input.exe basic > build\test_data\passport_basic_in.txt
build\02-image-passport\ref\ref_passport.exe basic > build\expected.txt
build\02-image-passport\cpp\passport.exe < build\test_data\passport_basic_in.txt > build\actual.txt
fc build\actual.txt build\expected.txt
echo %errorlevel%                                    & rem expected 0

rem error: пустое имя
build\02-image-passport\ref\ref_passport_input.exe empty_name > build\test_data\passport_empty_name_in.txt
build\02-image-passport\cpp\passport.exe < build\test_data\passport_empty_name_in.txt > build\actual.txt
echo %errorlevel%                                    & rem expected 65
```

Release-бинарники (`build\release\02-image-passport\...`).

**PowerShell:**
```powershell
cmd /c "build\release\02-image-passport\ref\ref_passport_input.exe basic > build\test_data\passport_basic_in.txt"
cmd /c "build\release\02-image-passport\ref\ref_passport.exe basic > build\expected.txt"
cmd /c "build\release\02-image-passport\cpp\passport.exe < build\test_data\passport_basic_in.txt > build\actual.txt"
cmd /c fc build\actual.txt build\expected.txt
$LASTEXITCODE                                        # -> 0

cmd /c "build\release\02-image-passport\ref\ref_passport_input.exe empty_name > build\test_data\passport_empty_name_in.txt"
cmd /c "build\release\02-image-passport\cpp\passport.exe < build\test_data\passport_empty_name_in.txt > build\actual.txt"
$LASTEXITCODE                                        # -> 65
```

**cmd:**
```bat
chcp 65001 >nul

rem success-кейс: два слова + 1920
build\release\02-image-passport\ref\ref_passport_input.exe basic > build\test_data\passport_basic_in.txt
build\release\02-image-passport\ref\ref_passport.exe basic > build\expected.txt
build\release\02-image-passport\cpp\passport.exe < build\test_data\passport_basic_in.txt > build\actual.txt
fc build\actual.txt build\expected.txt
echo %errorlevel%                                    & rem expected 0

rem error: пустое имя
build\release\02-image-passport\ref\ref_passport_input.exe empty_name > build\test_data\passport_empty_name_in.txt
build\release\02-image-passport\cpp\passport.exe < build\test_data\passport_empty_name_in.txt > build\actual.txt
echo %errorlevel%                                    & rem expected 65
```

Доступные кейсы (те же имена принимает `ref_passport_input`): `basic`, `single_1`, `plural_2`–`101`–`111`, `empty_name`, `no_input`, `bad_count`, `negative`, `zero`.

### Эталоны — Debug
Сборка эталонов (написаны на C)
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/ref/ 02-image-passport/ref/ref_passport.c 
link /DEBUG build/02-image-passport/ref/ref_passport.obj /OUT:build/02-image-passport/ref/ref_passport.exe
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/ref/ 02-image-passport/ref/ref_passport_input.c
link /DEBUG build/02-image-passport/ref/ref_passport_input.obj /OUT:build/02-image-passport/ref/ref_passport_input.exe
```

### Эталоны — Release
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/ref/ 02-image-passport/ref/ref_passport.c
link /DEBUG /OPT:REF /OPT:ICF build/release/02-image-passport/ref/ref_passport.obj /OUT:build/release/02-image-passport/ref/ref_passport.exe
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/ref/ 02-image-passport/ref/ref_passport_input.c
link /DEBUG /OPT:REF /OPT:ICF build/release/02-image-passport/ref/ref_passport_input.obj /OUT:build/release/02-image-passport/ref/ref_passport_input.exe
```

### Сборка основного приложения — Debug

Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/pixel_word.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport_main.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ common/c/strerror.c
link /DEBUG build/02-image-passport/c/read_passport_main.obj build/02-image-passport/c/read_passport.obj build/02-image-passport/c/pixel_word.obj build/02-image-passport/c/strerror.obj /OUT:build/02-image-passport/c/passport.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/pixel_word.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport_main.cpp
link /DEBUG build/02-image-passport/cpp/read_passport_main.obj build/02-image-passport/cpp/read_passport.obj build/02-image-passport/cpp/pixel_word.obj /OUT:build/02-image-passport/cpp/passport.exe
```

### Сборка основного приложения — Release

Для C:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ 02-image-passport/c/read_passport.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ 02-image-passport/c/pixel_word.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ 02-image-passport/c/read_passport_main.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/c/ common/c/strerror.c
link /DEBUG /OPT:REF /OPT:ICF build/release/02-image-passport/c/read_passport_main.obj build/release/02-image-passport/c/read_passport.obj build/release/02-image-passport/c/pixel_word.obj build/release/02-image-passport/c/strerror.obj /OUT:build/release/02-image-passport/c/passport.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/cpp/ 02-image-passport/cpp/read_passport.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/cpp/ 02-image-passport/cpp/pixel_word.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/02-image-passport/cpp/ 02-image-passport/cpp/read_passport_main.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/02-image-passport/cpp/read_passport_main.obj build/release/02-image-passport/cpp/read_passport.obj build/release/02-image-passport/cpp/pixel_word.obj /OUT:build/release/02-image-passport/cpp/passport.exe
```

---

## Задача 3 — Статистика изображения

Читает PPM `P3` из stdin до EOF, выводит:
- размеры (`W*H`)
- число пикселей
- средний цвет (округлённый)
- мин./макс. яркость (luma)
- гистограмму яркости (8 корзин)

Работает в конвейере:
```bat
build\01-image-gen\c\gen_image.exe 64 gradient | build\03-image-stats\c\image_stats.exe
build\01-image-gen\c\gen_image.exe --size 1024 --seed 42 | build\03-image-stats\c\image_stats.exe > report.txt
```

### Классы ошибок
- Пустой ввод -> exit 66
- IO-сбой -> exit 74
- Битый формат, не-число, `#` в данных, лишние/недостающие пиксели -> exit 65

### Тесты — Debug

Сборка тестов
Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/ppm_stats.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/ppm_stats_test.c
link /DEBUG build/03-image-stats/c/ppm_io.obj build/03-image-stats/c/strerror.obj build/03-image-stats/c/ppm_stats.obj build/03-image-stats/c/ppm_stats_test.obj /OUT:build/03-image-stats/c/ppm_stats_test.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats_test.cpp
link /DEBUG build/03-image-stats/cpp/ppm_io.obj build/03-image-stats/cpp/ppm_stats.obj build/03-image-stats/cpp/ppm_stats_test.obj /OUT:build/03-image-stats/cpp/ppm_stats_test.exe
```

Юнит-тесты (статистика через общий `ppm_io`):
``` 
build\03-image-stats\c\ppm_stats_test.exe  
build\03-image-stats\cpp\ppm_stats_test.exe
```

### Тесты — Release

Для C:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ 03-image-stats/c/ppm_stats.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ 03-image-stats/c/ppm_stats_test.c
link /DEBUG /OPT:REF /OPT:ICF build/release/03-image-stats/c/ppm_io.obj build/release/03-image-stats/c/strerror.obj build/release/03-image-stats/c/ppm_stats.obj build/release/03-image-stats/c/ppm_stats_test.obj /OUT:build/release/03-image-stats/c/ppm_stats_test.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats_test.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/03-image-stats/cpp/ppm_io.obj build/release/03-image-stats/cpp/ppm_stats.obj build/release/03-image-stats/cpp/ppm_stats_test.obj /OUT:build/release/03-image-stats/cpp/ppm_stats_test.exe
```

```
build\release\03-image-stats\c\ppm_stats_test.exe 
build\release\03-image-stats\cpp\ppm_stats_test.exe 
```

Acceptance — ручной прогон (конвейер). Debug-бинарники.

**PowerShell:**
```powershell
# эталон: ref_gradient | ref_stats
cmd /c "build\01-image-gen\ref\ref_gradient.exe | build\03-image-stats\ref\ref_stats.exe > build\test_data\stats_gradient_3x3.txt"

# прогон: gen_image | image_stats
cmd /c "build\01-image-gen\c\gen_image.exe 3 gradient | build\03-image-stats\c\image_stats.exe > build\actual.txt"

# сравнение
cmd /c fc build\actual.txt build\test_data\stats_gradient_3x3.txt
$LASTEXITCODE                                        # -> 0

# error-кейсы (только exit-код)
cmd /c "build\03-image-stats\c\image_stats.exe < nul"
$LASTEXITCODE                                        # -> 66
cmd /c "echo P5 | build\03-image-stats\c\image_stats.exe"
$LASTEXITCODE                                        # -> 65
```

**cmd:**
```bat
chcp 65001 >nul

rem эталон: ref_gradient | ref_stats
build\01-image-gen\ref\ref_gradient.exe | build\03-image-stats\ref\ref_stats.exe > build\test_data\stats_gradient_3x3.txt

rem прогон: gen_image | image_stats
build\01-image-gen\c\gen_image.exe 3 gradient | build\03-image-stats\c\image_stats.exe > build\actual.txt

rem сравнение
fc build\actual.txt build\test_data\stats_gradient_3x3.txt
echo %errorlevel%                                    & rem expected 0

rem error-кейсы (только exit-код)
build\03-image-stats\c\image_stats.exe < nul
echo %errorlevel%                                    & rem expected 66
echo P5 | build\03-image-stats\c\image_stats.exe
echo %errorlevel%                                    & rem expected 65
```

Release-бинарники (`build\release\...`).

**PowerShell:**
```powershell
cmd /c "build\release\01-image-gen\ref\ref_gradient.exe | build\release\03-image-stats\ref\ref_stats.exe > build\test_data\stats_gradient_3x3.txt"
cmd /c "build\release\01-image-gen\c\gen_image.exe 3 gradient | build\release\03-image-stats\c\image_stats.exe > build\actual.txt"
cmd /c fc build\actual.txt build\test_data\stats_gradient_3x3.txt
$LASTEXITCODE                                        # -> 0

cmd /c "build\release\03-image-stats\c\image_stats.exe < nul"
$LASTEXITCODE                                        # -> 66
cmd /c "echo P5 | build\release\03-image-stats\c\image_stats.exe"
$LASTEXITCODE                                        # -> 65
```

**cmd:**
```bat
chcp 65001 >nul

build\release\01-image-gen\ref\ref_gradient.exe | build\release\03-image-stats\ref\ref_stats.exe > build\test_data\stats_gradient_3x3.txt
build\release\01-image-gen\c\gen_image.exe 3 gradient | build\release\03-image-stats\c\image_stats.exe > build\actual.txt
fc build\actual.txt build\test_data\stats_gradient_3x3.txt
echo %errorlevel%                                    & rem expected 0

build\release\03-image-stats\c\image_stats.exe < nul
echo %errorlevel%                                    & rem expected 66
echo P5 | build\release\03-image-stats\c\image_stats.exe
echo %errorlevel%                                    & rem expected 65
```

### Эталоны — Debug
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/ref/ 03-image-stats/ref/ref_stats.c
link /DEBUG build/03-image-stats/ref/ref_stats.obj /OUT:build/03-image-stats/ref/ref_stats.exe
```

### Эталоны — Release
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/ref/ 03-image-stats/ref/ref_stats.c
link /DEBUG /OPT:REF /OPT:ICF build/release/03-image-stats/ref/ref_stats.obj /OUT:build/release/03-image-stats/ref/ref_stats.exe
```

### Сборка приложения — Debug

Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/ppm_stats.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/main.c
link /DEBUG build/03-image-stats/c/ppm_io.obj build/03-image-stats/c/strerror.obj build/03-image-stats/c/ppm_stats.obj build/03-image-stats/c/main.obj /OUT:build/03-image-stats/c/image_stats.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/image_stats.cpp
link /DEBUG build/03-image-stats/cpp/ppm_io.obj build/03-image-stats/cpp/ppm_stats.obj build/03-image-stats/cpp/image_stats.obj /OUT:build/03-image-stats/cpp/image_stats.exe
```

### Сборка приложения — Release

Для C:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ 03-image-stats/c/ppm_stats.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/c/ 03-image-stats/c/main.c
link /DEBUG /OPT:REF /OPT:ICF build/release/03-image-stats/c/ppm_io.obj build/release/03-image-stats/c/strerror.obj build/release/03-image-stats/c/ppm_stats.obj build/release/03-image-stats/c/main.obj /OUT:build/release/03-image-stats/c/image_stats.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/03-image-stats/cpp/ 03-image-stats/cpp/image_stats.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/03-image-stats/cpp/ppm_io.obj build/release/03-image-stats/cpp/ppm_stats.obj build/release/03-image-stats/cpp/image_stats.obj /OUT:build/release/03-image-stats/cpp/image_stats.exe
```

---

## Задача 4 — Фильтр изображения

Применяет преобразование к PPM `P3`, читает из stdin, пишет валидный PPM
в stdout. Использует общий модуль `common/ppm_io` (парсинг заголовка,
буфер пикселей, потоковую запись через `PpmWriter`).

Работает в конвейере:
```bat
build\01-image-gen\c\gen_image.exe 64 gradient | build\04-image-filter\c\filter.exe --grayscale
build\01-image-gen\c\gen_image.exe 4 gradient | build\04-image-filter\c\filter.exe --threshold 128 | build\03-image-stats\c\image_stats.exe
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
filter --version    -> "filter 0.1.5", exit 0
```

### Классы ошибок
- Без аргументов / неизвестный режим / `--threshold` без T / T не число или вне `[0; 255]` -> exit 64, stderr
- `--help` / `--version` -> exit 0, текст в stdout
- Пустой ввод -> exit 66
- Битый формат PPM (из `ppm_io`) -> exit 65
- IO-сбой -> exit 74


### Тесты — Debug

Юнит-тесты (grayscale, threshold, парсинг аргументов) — по пикселям, без интеграции:
```
build\04-image-filter\c\filter_tests.exe
build\04-image-filter\cpp\filter_tests.exe 
```
Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/filter.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/filter_test.c 
link /DEBUG build/04-image-filter/c/ppm_io.obj build/04-image-filter/c/strerror.obj build/04-image-filter/c/filter.obj build/04-image-filter/c/filter_test.obj /OUT:build/04-image-filter/c/filter_tests.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter_test.cpp 
link /DEBUG build/04-image-filter/cpp/ppm_io.obj build/04-image-filter/cpp/filter.obj build/04-image-filter/cpp/filter_test.obj /OUT:build/04-image-filter/cpp/filter_tests.exe
```

### Тесты — Release

Для C:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ 04-image-filter/c/filter.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ 04-image-filter/c/filter_test.c
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/c/ppm_io.obj build/release/04-image-filter/c/strerror.obj build/release/04-image-filter/c/filter.obj build/release/04-image-filter/c/filter_test.obj /OUT:build/release/04-image-filter/c/filter_tests.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/cpp/ 04-image-filter/cpp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/cpp/ 04-image-filter/cpp/filter_test.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/cpp/ppm_io.obj build/release/04-image-filter/cpp/filter.obj build/release/04-image-filter/cpp/filter_test.obj /OUT:build/release/04-image-filter/cpp/filter_tests.exe
```

```
build\release\04-image-filter\c\filter_tests.exe
build\release\04-image-filter\cpp\filter_tests.exe
```

### Эталоны — Debug
Для того чтобы проверить корректность работы самого фильтра, требуется собрать эталоны для первой задачи, а затем прогнать их под фильтром и сравнить с эталонным ответом фильтра. 
Помимо паттернов первой задачи используются probe-входы (2×2 и 3×3) с пикселями, подобранными под границы: округление яркости - luma - вниз/вверх, граница из-за использования float, строгая граница порога.
Подразумевается, что команды исполняются в cmd.

```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_gradient_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_gradient_3_3_threshold_128.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_radial_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_2_2.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_2_2_grayscale.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_2_2_threshold_100.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3_threshold_100.c
link /DEBUG build/04-image-filter/ref/ref_gradient_3_3_grayscale.obj /OUT:build/04-image-filter/ref/ref_gradient_3_3_grayscale.exe
link /DEBUG build/04-image-filter/ref/ref_gradient_3_3_threshold_128.obj /OUT:build/04-image-filter/ref/ref_gradient_3_3_threshold_128.exe
link /DEBUG build/04-image-filter/ref/ref_radial_3_3_grayscale.obj /OUT:build/04-image-filter/ref/ref_radial_3_3_grayscale.exe
link /DEBUG build/04-image-filter/ref/ref_probe_2_2.obj /OUT:build/04-image-filter/ref/ref_probe_2_2.exe
link /DEBUG build/04-image-filter/ref/ref_probe_2_2_grayscale.obj /OUT:build/04-image-filter/ref/ref_probe_2_2_grayscale.exe
link /DEBUG build/04-image-filter/ref/ref_probe_2_2_threshold_100.obj /OUT:build/04-image-filter/ref/ref_probe_2_2_threshold_100.exe
link /DEBUG build/04-image-filter/ref/ref_probe_3_3.obj /OUT:build/04-image-filter/ref/ref_probe_3_3.exe
link /DEBUG build/04-image-filter/ref/ref_probe_3_3_grayscale.obj /OUT:build/04-image-filter/ref/ref_probe_3_3_grayscale.exe
link /DEBUG build/04-image-filter/ref/ref_probe_3_3_threshold_100.obj /OUT:build/04-image-filter/ref/ref_probe_3_3_threshold_100.exe
```

### Эталоны — Release
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_gradient_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_gradient_3_3_threshold_128.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_radial_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_probe_2_2.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_probe_2_2_grayscale.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_probe_2_2_threshold_100.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3_threshold_100.c
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_gradient_3_3_grayscale.obj /OUT:build/release/04-image-filter/ref/ref_gradient_3_3_grayscale.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_gradient_3_3_threshold_128.obj /OUT:build/release/04-image-filter/ref/ref_gradient_3_3_threshold_128.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_radial_3_3_grayscale.obj /OUT:build/release/04-image-filter/ref/ref_radial_3_3_grayscale.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_probe_2_2.obj /OUT:build/release/04-image-filter/ref/ref_probe_2_2.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_probe_2_2_grayscale.obj /OUT:build/release/04-image-filter/ref/ref_probe_2_2_grayscale.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_probe_2_2_threshold_100.obj /OUT:build/release/04-image-filter/ref/ref_probe_2_2_threshold_100.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_probe_3_3.obj /OUT:build/release/04-image-filter/ref/ref_probe_3_3.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_probe_3_3_grayscale.obj /OUT:build/release/04-image-filter/ref/ref_probe_3_3_grayscale.exe
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/ref/ref_probe_3_3_threshold_100.obj /OUT:build/release/04-image-filter/ref/ref_probe_3_3_threshold_100.exe
```

### Примеры приёмки

Сборка probe (`cl`/`link`) одинакова для обеих оболочек и обеих конфигураций и приведена один раз. Остальные шаги используют перенаправление и даны для PowerShell и cmd.

**Часть 1. Паттерн первой задачи** (при условии собранных эталонов первой задачи; базовая картинка `radial_3x3.ppm` уже сгенерирована в `build/test_data`).

Debug — PowerShell:
```powershell
cmd /c "build\04-image-filter\ref\ref_radial_3_3_grayscale.exe > build\test_data\radial_3x3_grayscale.ppm"
cmd /c "build\04-image-filter\cpp\filter.exe --grayscale < build\test_data\radial_3x3.ppm > build\actual_filter.ppm"
cmd /c fc build\actual_filter.ppm build\test_data\radial_3x3_grayscale.ppm
$LASTEXITCODE                                        # -> 0
```

Debug — cmd:
```bat
chcp 65001 >nul
build\04-image-filter\ref\ref_radial_3_3_grayscale.exe > build\test_data\radial_3x3_grayscale.ppm
build\04-image-filter\cpp\filter.exe --grayscale < build\test_data\radial_3x3.ppm > build\actual_filter.ppm
fc build\actual_filter.ppm build\test_data\radial_3x3_grayscale.ppm
echo %errorlevel%                                    & rem expected 0
```

Release — PowerShell:
```powershell
cmd /c "build\release\04-image-filter\ref\ref_radial_3_3_grayscale.exe > build\test_data\radial_3x3_grayscale.ppm"
cmd /c "build\release\04-image-filter\cpp\filter.exe --grayscale < build\test_data\radial_3x3.ppm > build\actual_filter.ppm"
cmd /c fc build\actual_filter.ppm build\test_data\radial_3x3_grayscale.ppm
$LASTEXITCODE                                        # -> 0
```

Release — cmd:
```bat
chcp 65001 >nul
build\release\04-image-filter\ref\ref_radial_3_3_grayscale.exe > build\test_data\radial_3x3_grayscale.ppm
build\release\04-image-filter\cpp\filter.exe --grayscale < build\test_data\radial_3x3.ppm > build\actual_filter.ppm
fc build\actual_filter.ppm build\test_data\radial_3x3_grayscale.ppm
echo %errorlevel%                                    & rem expected 0
```

**Часть 2. Probe 3×3.** Сборка генератора входного изображения и генераторов эталонных отфильтрованных изображений (общая):
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3_grayscale.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/ref/ 04-image-filter/ref/ref_probe_3_3_threshold_100.c
link /DEBUG build/04-image-filter/ref/ref_probe_3_3.obj /OUT:build/04-image-filter/ref/ref_probe_3_3.exe
link /DEBUG build/04-image-filter/ref/ref_probe_3_3_grayscale.obj /OUT:build/04-image-filter/ref/ref_probe_3_3_grayscale.exe
link /DEBUG build/04-image-filter/ref/ref_probe_3_3_threshold_100.obj /OUT:build/04-image-filter/ref/ref_probe_3_3_threshold_100.exe
```

Генерация входного probe-изображения и эталонов.

Debug — PowerShell:
```powershell
cmd /c "build\04-image-filter\ref\ref_probe_3_3.exe > build\test_data\probe_3x3.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_3_3_grayscale.exe > build\test_data\probe_3x3_grayscale.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > build\test_data\probe_3x3_threshold_100.ppm"
```

Debug — cmd:
```bat
chcp 65001 >nul
build\04-image-filter\ref\ref_probe_3_3.exe > build\test_data\probe_3x3.ppm
build\04-image-filter\ref\ref_probe_3_3_grayscale.exe > build\test_data\probe_3x3_grayscale.ppm
build\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > build\test_data\probe_3x3_threshold_100.ppm
```

Release — PowerShell:
```powershell
cmd /c "build\release\04-image-filter\ref\ref_probe_3_3.exe > build\test_data\probe_3x3.ppm"
cmd /c "build\release\04-image-filter\ref\ref_probe_3_3_grayscale.exe > build\test_data\probe_3x3_grayscale.ppm"
cmd /c "build\release\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > build\test_data\probe_3x3_threshold_100.ppm"
```

Release — cmd:
```bat
chcp 65001 >nul
build\release\04-image-filter\ref\ref_probe_3_3.exe > build\test_data\probe_3x3.ppm
build\release\04-image-filter\ref\ref_probe_3_3_grayscale.exe > build\test_data\probe_3x3_grayscale.ppm
build\release\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > build\test_data\probe_3x3_threshold_100.ppm
```

Прогон фильтра на сгенерированном probe-изображении и сравнение с эталонами.

Debug — PowerShell:
```powershell
cmd /c "build\04-image-filter\cpp\filter.exe --grayscale < build\test_data\probe_3x3.ppm > build\actual_filter.ppm"
cmd /c fc build\actual_filter.ppm build\test_data\probe_3x3_grayscale.ppm
$LASTEXITCODE                                        # -> 0
cmd /c "build\04-image-filter\cpp\filter.exe --threshold 100 < build\test_data\probe_3x3.ppm > build\actual_filter.ppm"
cmd /c fc build\actual_filter.ppm build\test_data\probe_3x3_threshold_100.ppm
$LASTEXITCODE                                        # -> 0
```

Debug — cmd:
```bat
chcp 65001 >nul
build\04-image-filter\cpp\filter.exe --grayscale < build\test_data\probe_3x3.ppm > build\actual_filter.ppm
fc build\actual_filter.ppm build\test_data\probe_3x3_grayscale.ppm
echo %errorlevel%                                    & rem expected 0
build\04-image-filter\cpp\filter.exe --threshold 100 < build\test_data\probe_3x3.ppm > build\actual_filter.ppm
fc build\actual_filter.ppm build\test_data\probe_3x3_threshold_100.ppm
echo %errorlevel%                                    & rem expected 0
```

Release — PowerShell:
```powershell
cmd /c "build\release\04-image-filter\cpp\filter.exe --grayscale < build\test_data\probe_3x3.ppm > build\actual_filter.ppm"
cmd /c fc build\actual_filter.ppm build\test_data\probe_3x3_grayscale.ppm
$LASTEXITCODE                                        # -> 0
cmd /c "build\release\04-image-filter\cpp\filter.exe --threshold 100 < build\test_data\probe_3x3.ppm > build\actual_filter.ppm"
cmd /c fc build\actual_filter.ppm build\test_data\probe_3x3_threshold_100.ppm
$LASTEXITCODE                                        # -> 0
```

Release — cmd:
```bat
chcp 65001 >nul
build\release\04-image-filter\cpp\filter.exe --grayscale < build\test_data\probe_3x3.ppm > build\actual_filter.ppm
fc build\actual_filter.ppm build\test_data\probe_3x3_grayscale.ppm
echo %errorlevel%                                    & rem expected 0
build\release\04-image-filter\cpp\filter.exe --threshold 100 < build\test_data\probe_3x3.ppm > build\actual_filter.ppm
fc build\actual_filter.ppm build\test_data\probe_3x3_threshold_100.ppm
echo %errorlevel%                                    & rem expected 0
```

### Сборка основного приложения — Debug

Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/filter.c 
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/main.c
link /DEBUG build/04-image-filter/c/ppm_io.obj build/04-image-filter/c/strerror.obj build/04-image-filter/c/filter.obj build/04-image-filter/c/main.obj /OUT:build/04-image-filter/c/filter.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter_main.cpp
link /DEBUG build/04-image-filter/cpp/ppm_io.obj build/04-image-filter/cpp/filter.obj build/04-image-filter/cpp/filter_main.obj /OUT:build/04-image-filter/cpp/filter.exe
```

### Сборка основного приложения — Release

Для C:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ 04-image-filter/c/filter.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/c/ 04-image-filter/c/main.c
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/c/ppm_io.obj build/release/04-image-filter/c/strerror.obj build/release/04-image-filter/c/filter.obj build/release/04-image-filter/c/main.obj /OUT:build/release/04-image-filter/c/filter.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/cpp/ 04-image-filter/cpp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/04-image-filter/cpp/ 04-image-filter/cpp/filter_main.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/cpp/ppm_io.obj build/release/04-image-filter/cpp/filter.obj build/release/04-image-filter/cpp/filter_main.obj /OUT:build/release/04-image-filter/cpp/filter.exe
```

## Модуль `common/ppm_io` и сборка DLL

Задачи 1/3/4 используют общий модуль ввода-вывода PPM. Он собирается тремя
способами:

- **Объектный файл / статическая библиотека** — по умолчанию, без флагов
- **Динамическая библиотека (DLL / .so)** — с флагом `KV_DYNAMIC_LINK`. При этом, в  случае сборки с созданием динамической библиотеки, необходимо скопировать DLL(so на Linux/Mac) в папку с приложением-потребителем, иначе оно при запуске выдаст ошибку

Копирование DLL к потребителю приведено отдельными блоками для PowerShell и cmd (Debug и Release).

### Сборка DLL — Debug

Сборка для C (DLL + import-lib), линковка - с тестами для С реализации фильтров:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /utf-8 /DKV_DYNAMIC_LINK /LD common/c/ppm_io.c common/c/strerror.c /link /OUT:build/common/c/ppm_io.dll /IMPLIB:build/common/c/ppm_io.lib 
link /DEBUG build/04-image-filter/c/filter.obj build/04-image-filter/c/filter_test.obj build/common/c/ppm_io.lib /OUT:build/04-image-filter/c/filter_test_dll.exe
```

Сборка для C++ (DLL + import-lib), линковка - с тестами для С++ реализации фильтров:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /utf-8 /DKV_DYNAMIC_LINK /LD common/cpp/ppm_io.cpp /link /OUT:build/common/cpp/ppm_io.dll /IMPLIB:build/common/cpp/ppm_io.lib
link /DEBUG build/04-image-filter/cpp/filter.obj build/04-image-filter/cpp/filter_test.obj build/common/cpp/ppm_io.lib /OUT:build/04-image-filter/cpp/filter_test_dll.exe
```

Копирование DLL к потребителю (без него при запуске — ошибка разрешения символов).

PowerShell:
```powershell
Copy-Item -Path build/common/c/ppm_io.dll -Destination build/04-image-filter/c/
Copy-Item -Path build/common/cpp/ppm_io.dll -Destination build/04-image-filter/cpp/
```

cmd:
```bat
chcp 65001 >nul
copy /Y build\common\c\ppm_io.dll build\04-image-filter\c\
copy /Y build\common\cpp\ppm_io.dll build\04-image-filter\cpp\
```

### Сборка DLL — Release

Сборка для C (DLL + import-lib), линковка - с тестами для С реализации фильтров:
```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /DKV_DYNAMIC_LINK /LD common/c/ppm_io.c common/c/strerror.c /link /OUT:build/release/common/c/ppm_io.dll /IMPLIB:build/release/common/c/ppm_io.lib
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/c/filter.obj build/release/04-image-filter/c/filter_test.obj build/release/common/c/ppm_io.lib /OUT:build/release/04-image-filter/c/filter_test_dll.exe
```

Сборка для C++ (DLL + import-lib), линковка - с тестами для С++ реализации фильтров:
```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /DKV_DYNAMIC_LINK /LD common/cpp/ppm_io.cpp /link /OUT:build/release/common/cpp/ppm_io.dll /IMPLIB:build/release/common/cpp/ppm_io.lib
link /DEBUG /OPT:REF /OPT:ICF build/release/04-image-filter/cpp/filter.obj build/release/04-image-filter/cpp/filter_test.obj build/release/common/cpp/ppm_io.lib /OUT:build/release/04-image-filter/cpp/filter_test_dll.exe
```

Копирование DLL к потребителю.

PowerShell:
```powershell
Copy-Item -Path build/release/common/c/ppm_io.dll -Destination build/release/04-image-filter/c/
Copy-Item -Path build/release/common/cpp/ppm_io.dll -Destination build/release/04-image-filter/cpp/
```

cmd:
```bat
chcp 65001 >nul
copy /Y build\release\common\c\ppm_io.dll build\release\04-image-filter\c\
copy /Y build\release\common\cpp\ppm_io.dll build\release\04-image-filter\cpp\
```

- **Linux / macOS** — флаг игнорируется, символы `.so` экспортируются по умолчанию

Макрос экспорта: `KV_API` = `__declspec(dllexport)` на Windows при
`KV_DYNAMIC_LINK`, иначе пусто. `dllimport` не используется: потребитель,
линкующий import-lib, работает без него.

В C++ `Image` реализован как PIMPL-класс (move-only): поля скрыты за
`std::unique_ptr<Impl>`, публичный API — через аксессоры (`width()`,
`height()`, `pixel_count()`, `pixels()`). Это даёт стабильный layout
объекта и снимает dependency на `<vector>` из заголовка.

### Номер строки ошибки

Результат чтения (`struct PpmResult` в C, `struct PpmResult` в C++)
содержит номер строки, на которой возникла ошибка парсинга: поле
`error_line` (C) / `line` (C++). Диагностическое сообщение (`diagnostic`)
по-прежнему содержит и текст вида «строка N: …» — поле добавлено для
программного доступа к номеру строки без разбора текста.

### Инкрементальное чтение и внутренний шов аллокатора

Заголовок PPM задаёт целевое число пикселей, но память под них выделяется
**по мере чтения**: буфер растёт геометрически (C — `realloc`, C++ —
`std::pmr::vector`), а не резервируется по объявленному `W×H`. Поэтому заголовок
с завышенными размерами и короткими данными даёт «получено слишком мало пикселей»
без попытки выделить объявленный объём.

Для тестов ветки нехватки памяти есть внутренний шов (не экспортируется и не
входит в потребительский API):

- C: `common/c/ppm_io_alloc.h` — `struct PpmAllocator` и `ppm_read_with`;
- C++: неэкспортируемый `Image::read(std::istream&, std::pmr::memory_resource*)`.

Потребители (`ppm_read`, `Image::read(is)`) шов не видят.

---

## Тесты общих модулей

Логика общих модулей (`ppm_io`, `luma`) тестируется на уровне модуля;
юнит-тесты задач покрывают только специфичную для задачи логику.

### C — Debug


```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/common/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/common/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/common/c/ common/c/ppm_io_test.c
link /DEBUG build/common/c/ppm_io.obj build/common/c/strerror.obj build/common/c/ppm_io_test.obj /OUT:build/common/c/ppm_io_test.exe
buil\common\c\ppm_io_test.exe
```

### C — Release

`common/c/ppm_io_test.c` покрывает `ppm_io` (чтение/запись PPM) и `luma`:

```
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/common/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/common/c/ common/c/strerror.c
cl /std:c17 /W4 /permissive- /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/common/c/ common/c/ppm_io_test.c
link /DEBUG /OPT:REF /OPT:ICF build/release/common/c/ppm_io.obj build/release/common/c/strerror.obj build/release/common/c/ppm_io_test.obj /OUT:build/release/common/c/ppm_io_test.exe
build\release\common\c\ppm_io_test.exe
```

### C++ — Debug

`common/cpp/ppm_io_test.cpp` покрывает `ppm_io` и `luma` (`luma.hpp` header-only):

```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/common/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/common/cpp/ common/cpp/ppm_io_test.cpp
link /DEBUG build/common/cpp/ppm_io.obj build/common/cpp/ppm_io_test.obj /OUT:build/common/cpp/ppm_io_test.exe
build\common\cpp\ppm_io_test.exe
```

### C++ — Release

`common/cpp/ppm_io_test.cpp` покрывает `ppm_io` и `luma` (`luma.hpp` header-only):

```
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/common/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /O2 /Zi /DNDEBUG /MD /utf-8 /c /Fo:build/release/common/cpp/ common/cpp/ppm_io_test.cpp
link /DEBUG /OPT:REF /OPT:ICF build/release/common/cpp/ppm_io.obj build/release/common/cpp/ppm_io_test.obj /OUT:build/release/common/cpp/ppm_io_test.exe
buildr\release\commo\cpp\ppm_io_test.exe
```

---

## Массовые тестовые данные

Каталог `build/test_data/` (в `.gitignore`) — данные для acceptance-тестов:
тестовые входы и эталонные файлы. Генерируются на лету командами ниже,
в репозиторий не хранятся.

Команды ниже используют Debug-бинарники; для Release-прогона замените пути на `build\release\...`.

### Тестовые входы (random)

Случайные изображения для задач 3/4 (паттерн `random`). 6 файлов:
`N = {2, 64, 1024}` × `seed = {42, 9999}`.

**PowerShell:**
```powershell
cmd /c "build\01-image-gen\c\gen_image.exe --size 2 --seed 42 > build/test_data/random_2x2_seed42.ppm"
cmd /c "build\01-image-gen\c\gen_image.exe --size 2 --seed 9999 > build/test_data/random_2x2_seed9999.ppm"
cmd /c "build\01-image-gen\c\gen_image.exe --size 64 --seed 42 > build/test_data/random_64x64_seed42.ppm"
cmd /c "build\01-image-gen\c\gen_image.exe --size 64 --seed 9999 > build/test_data/random_64x64_seed9999.ppm"
cmd /c "build\01-image-gen\c\gen_image.exe --size 1024 --seed 42 > build/test_data/random_1024x1024_seed42.ppm"
cmd /c "build\01-image-gen\c\gen_image.exe --size 1024 --seed 9999 > build/test_data/random_1024x1024_seed9999.ppm"
```

**cmd:**
```bat
chcp 65001 >nul
build\01-image-gen\c\gen_image.exe --size 2 --seed 42 > build/test_data/random_2x2_seed42.ppm
build\01-image-gen\c\gen_image.exe --size 2 --seed 9999 > build/test_data/random_2x2_seed9999.ppm
build\01-image-gen\c\gen_image.exe --size 64 --seed 42 > build/test_data/random_64x64_seed42.ppm
build\01-image-gen\c\gen_image.exe --size 64 --seed 9999 > build/test_data/random_64x64_seed9999.ppm
build\01-image-gen\c\gen_image.exe --size 1024 --seed 42 > build/test_data/random_1024x1024_seed42.ppm
build\01-image-gen\c\gen_image.exe --size 1024 --seed 9999 > build/test_data/random_1024x1024_seed9999.ppm
```

### Эталонные файлы

Независимые reference-выходы, с которыми сравнивается фактический вывод.

Задача 1 (3 файла, 3×3) — через ref-генераторы.

PowerShell:
```powershell
cmd /c "build\01-image-gen\ref\ref_gradient.exe > build/test_data/gradient_3x3.ppm"
cmd /c "build\01-image-gen\ref\ref_checker.exe > build/test_data/checker_3x3.ppm"
cmd /c "build\01-image-gen\ref\ref_radial.exe > build/test_data/radial_3x3.ppm"
```

cmd:
```bat
chcp 65001 >nul
build\01-image-gen\ref\ref_gradient.exe > build/test_data/gradient_3x3.ppm
build\01-image-gen\ref\ref_checker.exe > build/test_data/checker_3x3.ppm
build\01-image-gen\ref\ref_radial.exe > build/test_data/radial_3x3.ppm
```

Задача 4 (9 файлов) — 3 эталона по паттернам + probe-входы 2×2/3×3 и их эталоны.

PowerShell:
```powershell
cmd /c "build\04-image-filter\ref\ref_gradient_3_3_grayscale.exe > build/test_data/gradient_3x3_grayscale.ppm"
cmd /c "build\04-image-filter\ref\ref_gradient_3_3_threshold_128.exe > build/test_data/gradient_3x3_threshold_128.ppm"
cmd /c "build\04-image-filter\ref\ref_radial_3_3_grayscale.exe > build/test_data/radial_3x3_grayscale.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_2_2.exe > build/test_data/probe_2x2.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_2_2_grayscale.exe > build/test_data/probe_2x2_grayscale.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_2_2_threshold_100.exe > build/test_data/probe_2x2_threshold_100.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_3_3.exe > build/test_data/probe_3x3.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_3_3_grayscale.exe > build/test_data/probe_3x3_grayscale.ppm"
cmd /c "build\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > build/test_data/probe_3x3_threshold_100.ppm"
```

cmd:
```bat
chcp 65001 >nul
build\04-image-filter\ref\ref_gradient_3_3_grayscale.exe > build/test_data/gradient_3x3_grayscale.ppm
build\04-image-filter\ref\ref_gradient_3_3_threshold_128.exe > build/test_data/gradient_3x3_threshold_128.ppm
build\04-image-filter\ref\ref_radial_3_3_grayscale.exe > build/test_data/radial_3x3_grayscale.ppm
build\04-image-filter\ref\ref_probe_2_2.exe > build/test_data/probe_2x2.ppm
build\04-image-filter\ref\ref_probe_2_2_grayscale.exe > build/test_data/probe_2x2_grayscale.ppm
build\04-image-filter\ref\ref_probe_2_2_threshold_100.exe > build/test_data/probe_2x2_threshold_100.ppm
build\04-image-filter\ref\ref_probe_3_3.exe > build/test_data/probe_3x3.ppm
build\04-image-filter\ref\ref_probe_3_3_grayscale.exe > build/test_data/probe_3x3_grayscale.ppm
build\04-image-filter\ref\ref_probe_3_3_threshold_100.exe > build/test_data/probe_3x3_threshold_100.ppm
```

Итого 18 файлов: 6 тестовых входов (random) + 12 эталонных/входных
(3 базовых для задачи 1; 9 для задачи 4 — 2 probe-входа и 7 эталонов фильтра).
Эталоны задач 2 и 3

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