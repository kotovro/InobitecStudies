# Программа подготовки к стажировке в Inobitec — трек C/C++

Репозиторий с задачами оценочного периода. Домен задачника — растровые
изображения в текстовом формате PPM (`P3` — RGB, тройки ASCII).

## Структура проекта

```
AGENTS.md          — журнал работы и память агента
common/
  c/               — общие модули (C)
    exit_codes.h   — именованные exit-коды
    version.h      — версия программ (`KV_VERSION`)
    ppm_io.h(.c)   — модуль ввода-вывода PPM (парсинг/запись, экспорт в DLL)
  cpp/             — общие модули (C++)
    exit_codes.hpp — именованные exit-коды
    version.hpp    — версия программ (`kVersion`)
    ppm_io.hpp(.cpp) — модуль ввода-вывода PPM (PIMPL, экспорт в DLL)
build/
  test_data/       — сгенерированные тестовые PPM (в .gitignore)
  {task}/{c,cpp,ref}/ — артефакты сборки по задачам (объекты, exe, эталоны)
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
 04-image-filter/   — задача 4: фильтр изображения
   c/               — реализация на C
   cpp/             — реализация на C++
```

Все задачи решаются дважды: C (C17) и C++ (C++23).

---

## Требования к окружению

- **OS:** Windows
- **Toolchain:** MSVC (Visual Studio Build Tools, `vcvars64.bat`)
- **Сборка:** из консоли, `cl /c` + `link`, без IDE
- **Консоль:** PowerShell / FAR Manager, кодировка UTF-8 (`chcp 65001`)
- **Просмотр PPM:** IrfanView, GIMP, ImageMagick (`magick out.ppm out.png`)

---

## Сборка

Установить окружение:
```
vcvars64.bat
```

Все команды выполняются из корня репозитория; cd внутри команд не используется. Сборка содержит раздельные этапы.

Создать каталоги артефактов (один раз; `-Force` — повторный запуск безопасен):
```powershell
New-Item -ItemType Directory -Force -Path build/test_data, build/common/cpp, build/01-image-gen/c, build/01-image-gen/cpp, build/01-image-gen/ref, build/02-image-passport/c, build/02-image-passport/cpp, build/02-image-passport/ref, build/03-image-stats/c, build/03-image-stats/cpp, build/03-image-stats/ref, build/04-image-filter/c, build/04-image-filter/cpp
```

Флаги: `Debug + ASan` (`/Od /Zi /MDd /fsanitize=address`).

### Эталонные программы

Эталонные программы представляют из себя независимые реализации для acceptance-тестов


| Программа | Файл | Назначение |
|---|---|---|
| `ref_gradient N` | `01-image-gen/ref/ref_gradient.c` | PPM gradient N x N |
| `ref_checker N` | `01-image-gen/ref/ref_checker.c` | PPM checker N x N |
| `ref_radial N` | `01-image-gen/ref/ref_radial.c` | PPM radial N x N |
| `ref_stats` | `03-image-stats/c/ref_stats.c` | Статистика PPM из stdin |
| `ref_passport <case>` | `02-image-passport/ref/ref_passport.c` | Паспорт: эталонный вывод по кейсу |

Все бинарники — в `build/`.

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
gen_image --version    -> "gen_image 0.1.0", exit 0
```

### Поведение при ошибках
- Нет аргументов -> exit 64, stderr
- `N` не число / вне диапазона -> exit 64, stderr
- Неизвестный паттерн -> exit 64, stderr
- `--help` / `--version` -> exit 0, текст в stdout
- Успех -> exit 0, PPM в stdout

### Тесты
Для С:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/patterns.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/hsv_to_rgb.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/ppm_test.c
link /DEBUG build/01-image-gen/c/patterns.obj build/01-image-gen/c/parse_args.obj build/01-image-gen/c/hsv_to_rgb.obj build/01-image-gen/c/ppm_test.obj /OUT:build/01-image-gen/c/ppm_test.exe
build/01-image-gen/c/ppm_test.exe
```

Для С++:
```
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/patterns.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/parse_args.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/hsv_to_rgb.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/cpp/ 01-image-gen/cpp/ppm_test.cpp
link /DEBUG build/01-image-gen/cpp/patterns.obj build/01-image-gen/cpp/parse_args.obj build/01-image-gen/cpp/hsv_to_rgb.obj build/01-image-gen/cpp/ppm_test.obj /OUT:build/01-image-gen/cpp/ppm_test.exe
build/01-image-gen/cpp/ppm_test.exe
```


Acceptance — ручной прогон с эталоном через `fc`:
```powershell
# gradient 3x3
build\01-image-gen\ref\ref_gradient.exe 3 > build\test_data\gradient_3x3.ppm
build\01-image-gen\c\gen_image.exe 3 gradient > build\actual.ppm
fc build\actual.ppm build\test_data\gradient_3x3.ppm

# error-кейсы (только exit-код; stderr — для человека)
build\01-image-gen\c\gen_image.exe; $LASTEXITCODE        # -> 64
build\01-image-gen\c\gen_image.exe abc; $LASTEXITCODE    # -> 64
build\01-image-gen\c\gen_image.exe 0; $LASTEXITCODE      # -> 64

# справка и версия
build\01-image-gen\c\gen_image.exe --help; $LASTEXITCODE     # -> 0, usage в stdout
build\01-image-gen\c\gen_image.exe --version; $LASTEXITCODE  # -> 0, "gen_image 0.1.0"
```

### Сборка основного приложения
Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/parse_args.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/hsv_to_rgb.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/patterns.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ 01-image-gen/c/main.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/01-image-gen/c/ common/c/ppm_io.c
link /DEBUG build/01-image-gen/c/parse_args.obj build/01-image-gen/c/patterns.obj build/01-image-gen/c/hsv_to_rgb.obj build/01-image-gen/c/main.obj build/01-image-gen/c/ppm_io.obj /OUT:build/01-image-gen/c/gen_image.exe
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

### Тесты

Сборка тестов
Для С:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport_test.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/pixel_word.c
link /DEBUG build/02-image-passport/c/read_passport_test.obj build/02-image-passport/c/pixel_word.obj /OUT:build/02-image-passport/c/passport_tests.exe  
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport_test.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/pixel_word.cpp
link /DEBUG build/02-image-passport/cpp/read_passport_test.obj build/02-image-passport/cpp/pixel_word.obj /OUT:build/02-image-passport/cpp/passport_tests.exe  
```

Юнит-тесты (склонение «пиксель/пикселя/пикселей»):
```
build/02-image-passport/c/passport_tests.exe # C
build/02-image-passport/cpp/passport_tests.exe # C++
```

Acceptance — эталон `ref_passport.exe`, ручное сравнение через `fc`:
```powershell
# success-кейс: два слова + 1920
"морской закат`n1920" | build\02-image-passport\cpp\passport.exe > build\actual.txt
build\02-image-passport\ref\ref_passport.exe basic > build\expected.txt
fc build\actual.txt build\expected.txt
echo $LASTEXITCODE # -> 0

# error: пустое имя
"`n1920" | build\02-image-passport\cpp\passport.exe 2> build\actual_stderr.txt
echo $LASTEXITCODE # -> 65
```

Доступные кейсы: `basic`, `single_1`, `plural_2`–`101`–`111`, `empty_name`, `no_input`, `bad_count`, `negative`, `zero`.

### Эталоны
Cборка эталонов (написаны на C)
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/ref/ 02-image-passport/ref/ref_passport.c 
link /DEBUG build/02-image-passport/ref/ref_passport.obj /OUT:build/02-image-passport/ref/ref_passport.exe
```

### Сборка основного приложения

Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/pixel_word.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/c/ 02-image-passport/c/read_passport_main.c
link /DEBUG build/02-image-passport/c/read_passport_main.obj build/02-image-passport/c/read_passport.obj build/02-image-passport/c/pixel_word.obj /OUT:build/02-image-passport/c/passport.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/pixel_word.cpp
cl /std:c++latest /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/02-image-passport/cpp/ 02-image-passport/cpp/read_passport_main.cpp
link /DEBUG build/02-image-passport/cpp/read_passport_main.obj build/02-image-passport/cpp/read_passport.obj build/02-image-passport/cpp/pixel_word.obj /OUT:build/02-image-passport/cpp/passport.exe
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
```
build\01-image-gen\c\gen_image.exe 64 gradient | build\03-image-stats\c\image_stats.exe
build\01-image-gen\c\gen_image.exe --size 1024 --seed 42 | build\03-image-stats\c\image_stats.exe > report.txt
```

### Классы ошибок
- Пустой ввод -> exit 66
- IO-сбой -> exit 74
- Битый формат, не-число, `#` в данных, лишние/недостающие пиксели -> exit 65

### Тесты

Сборка тестов
Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/ppm_stats.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/ppm_stats_test.c
link /DEBUG build/03-image-stats/c/ppm_io.obj build/03-image-stats/c/ppm_stats.obj build/03-image-stats/c/ppm_stats_test.obj /OUT:build/03-image-stats/c/ppm_stats_test.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats_test.cpp
link /DEBUG build/03-image-stats/cpp/ppm_io.obj build/03-image-stats/cpp/ppm_stats.obj build/03-image-stats/cpp/ppm_stats_test.obj /OUT:build/03-image-stats/cpp/ppm_stats_test.exe
```

Юнит-тесты (luma, статистика через общий `ppm_io`):
```
build/03-image-stats/c/ppm_stats_test.exe   # compute_stats, luma
build/03-image-stats/cpp/ppm_stats_test.exe # C++
```

Acceptance — ручной прогон (конвейер + `fc`):
```powershell
# эталон: ref_gradient | ref_stats
build\01-image-gen\ref\ref_gradient.exe 2 | build\03-image-stats\ref\ref_stats.exe > build\test_data\stats_gradient_2x2.txt

# прогон: gen_image | image_stats
build\01-image-gen\c\gen_image.exe 2 gradient | build\03-image-stats\c\image_stats.exe > build\actual.txt

# сравнение
fc build\actual.txt build\test_data\stats_gradient_2x2.txt

# error-кейсы (только exit-код)
echo "" | build\03-image-stats\c\image_stats.exe; $LASTEXITCODE # -> 66
echo P5 | build\03-image-stats\c\image_stats.exe; $LASTEXITCODE # -> 65
```

### Эталоны 
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/ref/ 03-image-stats/c/ref_stats.c
link /DEBUG build/03-image-stats/ref/ref_stats.obj /OUT:build/03-image-stats/ref/ref_stats.exe
```

### Сборка приложения

Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/ppm_stats.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/c/ 03-image-stats/c/main.c
link /DEBUG build/03-image-stats/c/ppm_io.obj build/03-image-stats/c/ppm_stats.obj build/03-image-stats/c/main.obj /OUT:build/03-image-stats/c/image_stats.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/ppm_stats.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/03-image-stats/cpp/ 03-image-stats/cpp/image_stats.cpp
link /DEBUG build/03-image-stats/cpp/ppm_io.obj build/03-image-stats/cpp/ppm_stats.obj build/03-image-stats/cpp/image_stats.obj /OUT:build/03-image-stats/cpp/image_stats.exe
```

---

## Задача 4 — Фильтр изображения

Применяет преобразование к PPM `P3`, читает из stdin, пишет валидный PPM
в stdout. Использует общий модуль `common/ppm_io` (парсинг заголовка,
буфер пикселей, потоковую запись через `PpmWriter`).

Работает в конвейере:
```
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
filter --version    -> "filter 0.1.0", exit 0
```

### Классы ошибок
- Без аргументов / неизвестный режим / `--threshold` без T / T не число или вне `[0; 255]` -> exit 64, stderr
- `--help` / `--version` -> exit 0, текст в stdout
- Пустой ввод -> exit 66
- Битый формат PPM (из `ppm_io`) -> exit 65
- IO-сбой -> exit 74

### Тесты

Юнит-тесты (luma, grayscale, threshold, парсинг аргументов) и integration
(полный конвейер `argv -> stdin -> stdout` через `run_filter`):
```
build/04-image-filter/c/filter_tests.exe    # C
build/04-image-filter/cpp/filter_tests.exe  # C++
```
Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/filter.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/filter_test.c 
link /DEBUG build/04-image-filter/c/ppm_io.obj build/04-image-filter/c/filter.obj build/04-image-filter/c/filter_test.obj /OUT:build/04-image-filter/c/filter_tests.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter_test.cpp 
link /DEBUG build/04-image-filter/cpp/ppm_io.obj build/04-image-filter/cpp/filter.obj build/04-image-filter/cpp/filter_test.obj /OUT:build/04-image-filter/cpp/filter_tests.exe
```
### Сборка основного приложения
Для C:
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ common/c/ppm_io.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/filter.c 
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/c/ 04-image-filter/c/main.c
link /DEBUG build/04-image-filter/c/ppm_io.obj build/04-image-filter/c/filter.obj build/04-image-filter/c/main.obj /OUT:build/04-image-filter/c/filter.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /utf-8 /c /Fo:build/04-image-filter/cpp/ 04-image-filter/cpp/filter_main.cpp
link /DEBUG build/04-image-filter/cpp/ppm_io.obj build/04-image-filter/cpp/filter.obj build/04-image-filter/cpp/filter_main.obj /OUT:build/04-image-filter/cpp/filter.exe
```
### Модуль `common/ppm_io` и сборка DLL

Задачи 1/3/4 используют общий модуль ввода-вывода PPM. Он собирается тремя
способами:

- **Объектный файл / статическая библиотека** — по умолчанию, без флагов
- **Динамическая библиотека (DLL / .so)** — с флагом `KV_DYNAMIC_LINK`:

```
# C++ DLL + import-lib
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /DKV_DYNAMIC_LINK /LD common/cpp/ppm_io.cpp /link /OUT:build/common/cpp/ppm_io.dll /IMPLIB:build/common/cpp/ppm_io.lib

# линковка потребителя через import-lib (вместо ppm_io.obj)
link /DEBUG build/04-image-filter/cpp/filter.obj build/04-image-filter/cpp/filter_test.obj build/common/cpp/ppm_io.lib /OUT:build/04-image-filter/cpp/filter_test_dll.exe
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

---

## Массовые тестовые данные

Данные в `build/test_data/` (каталог в `.gitignore`) генерируются вручную
через `gen_image` с seed:

```
build\01-image-gen\c\gen_image.exe --size 2 --seed 42 > build/test_data/random_2x2_seed42.ppm
build\01-image-gen\c\gen_image.exe --size 1024 --seed 9999 > build/test_data/random_1024x1024_seed9999.ppm
```

Эталонные файлы — через ref-генераторы:

```
build\01-image-gen\ref\ref_gradient.exe 4 > build/test_data/gradient_4x4.ppm
```

Итого 15 файлов: random (2×2, 64×64, 1024×1024 × 3 seed) + reference
(gradient/checker/radial 2×2, 4×4).

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