# Программа подготовки к стажировке в Inobitec — трек C/C++

Репозиторий с задачами оценочного периода. Домен задачника — растровые
изображения в текстовом формате PPM (`P3` — RGB, тройки ASCII).

## Структура проекта

```
AGENTS.md          — журнал работы и память агента
common/
  c/               — общие модули (C)
    exit_codes.h   — именованные exit-коды
    ppm_io.h(.c)   — модуль ввода-вывода PPM (парсинг/запись, экспорт в DLL)
  cpp/             — общие модули (C++)
    exit_codes.hpp — именованные exit-коды
    ppm_io.hpp(.cpp) — модуль ввода-вывода PPM (PIMPL, экспорт в DLL)
build/
  test_data/       — сгенерированные тестовые PPM (в .gitignore)
dialog_logs/       — сырые логи диалогов с DeepSeek
00-intro-hello/    — hello world
 01-image-gen/      — задача 1: генератор PPM
   c/               — реализация на C (включая ref_*.c — эталоны)
   cpp/             — реализация на C++
 02-image-passport/ — задача 2: паспорт изображения
   c/               — реализация на C
   cpp/             — реализация на C++
   ref_passport.c   — эталон для acceptance-тестов
 03-image-stats/    — задача 3: статистика изображения
   c/
   cpp/
 04-image-filter/   — задача 4: фильтр изображения
   c/               — реализация на C
   cpp/             — реализация на C++
gen_mass_test_data.ps1 — генератор массовых тестовых данных
```

Все задачи решаются дважды: C (C17) и C++ (C++23).

---

## Требования к окружению

- **OS:** Windows
- **Toolchain:** MSVC (Visual Studio Build Tools, `vcvars64.bat`)
- **Сборка:** из консоли, `cl /c` + `link`, без IDE
- **Консоль:** PowerShell / FAR Manager
- **Просмотр PPM:** IrfanView, GIMP, ImageMagick (`magick out.ppm out.png`)

---

## Сборка

Установить окружение:
```
vcvars64.bat
```

Раздельные этапы (пример для задачи 1, C):
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /c parse_args.c
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /c gen.c
link /DEBUG parse_args.obj hsv_to_rgb.obj gen.obj /OUT:gen_image.exe
```

Для C++:
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /c gen.cpp
link /DEBUG parse_args.obj hsv_to_rgb.obj gen.obj /OUT:gen_image.exe
```

Флаги: `Debug + ASan` (`/Od /Zi /MDd /fsanitize=address`).

> Исходники на русском языке сохранены в CP1251 — флаг `/utf-8` не требуется.
> MSVC по умолчанию интерпретирует файлы в кодировке системной локали, что
> соответствует CP1251 на русской Windows.

### Эталонные программы

Независимые реализации для acceptance-тестов. Собираются одними командами:

```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd ref_gradient.c
link /DEBUG ref_gradient.obj /OUT:ref_gradient.exe
```

| Программа | Файл | Назначение |
|---|---|---|
| `ref_gradient N` | `01-image-gen/c/ref_gradient.c` | ППМ gradient N x N |
| `ref_checker N` | `01-image-gen/c/ref_checker.c` | ППМ checker N x N |
| `ref_radial N` | `01-image-gen/c/ref_radial.c` | ППМ radial N x N |
| `ref_stats` | `03-image-stats/c/ref_stats.c` | Статистика ППМ из stdin |
| `ref_passport <case>` | `02-image-passport/ref_passport.c` | Паспорт: эталонный вывод по кейсу |

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

### Поведение при ошибках
- Нет аргументов -> exit 66, stderr
- `N` не число / вне диапазона -> exit 64, stderr
- Неизвестный паттерн -> exit 64, stderr
- Успех -> exit 0, PPM в stdout

### Тесты
```
cl /std:c17 /W4 /permissive- /Od /Zi /MDd /fsanitize=address /c ppm_test.c
link /DEBUG parse_args.obj hsv_to_rgb.obj ppm_test.obj /OUT:ppm_test.exe
ppm_test.exe
```

Acceptance — ручной прогон с эталоном через `fc`:
```powershell
# gradient 3x3
ref_gradient 3 > build\etalons\gradient_3x3.ppm
gen_image 3 gradient > build\actual.ppm
fc build\actual.ppm build\etalons\gradient_3x3.ppm

# error-кейсы (только exit-код; stderr — для человека)
gen_image; $LASTEXITCODE        # -> 66
gen_image abc; $LASTEXITCODE    # -> 64
gen_image 0; $LASTEXITCODE      # -> 64
```

---

## Задача 2 — Паспорт изображения

Запрашивает название и число пикселей, выводит фразу с правильным склонением
«пиксель/пикселя/пикселей».

```
read_passport
  -> Введите название изображения: морской закат
  -> Введите количество пикселей: 1920
  -> Изображение «морской закат»: 1920 пикселей.
```

### Классы ошибок
- Пустой ввод / EOF -> exit 66
- Пустое имя / не-число / отрицательное -> exit 65
- IO-сбой -> exit 74

### Тесты

Юнит-тесты (склонение «пиксель/пикселя/пикселей»):
```
read_passport_test.exe   (C и C++)
```

Acceptance — эталон `ref_passport.exe` в `build/`, ручное сравнение через `fc`:
```powershell
# сборка эталона
cl /std:c17 /W4 /permissive- /Od /Zi /MDd 02-image-passport/ref_passport.c
link /DEBUG ref_passport.obj /OUT:build/ref_passport.exe

# success-кейс: два слова + 1920
"морской закат`n1920" | .\02-image-passport\cpp\read_passport.exe > build\actual.txt
.\build\ref_passport.exe basic > build\expected.txt
fc build\actual.txt build\expected.txt
echo $LASTEXITCODE              # -> 0

# error: пустое имя
"`n1920" | .\02-image-passport\c\read_passport.exe 2> build\actual_stderr.txt
$LASTEXITCODE                   # -> 65
```

Доступные кейсы: `basic`, `single_1`, `plural_2`–`101`–`111`, `empty_name`, `no_input`, `bad_count`, `negative`, `zero`.

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
gen_image 64 gradient | image_stats
gen_image --size 1024 --seed 42 | image_stats > report.txt
```

### Классы ошибок
- Пустой ввод -> exit 66
- IO-сбой -> exit 74
- Битый формат, не-число, `#` в данных, лишние/недостающие пиксели -> exit 65

### Тесты

Юнит-тесты:
```
ppm_stats_test.exe   # luma, парсинг, статистики
```

Acceptance — ручной прогон (конвейер + `fc`):
```powershell
# эталон: ref_gradient | ref_stats
ref_gradient 2 | ref_stats > build\etalons\stats_gradient_2x2.txt

# прогон: gen_image | image_stats
gen_image 2 gradient | image_stats > build\actual.txt

# сравнение
fc build\actual.txt build\etalons\stats_gradient_2x2.txt

# error-кейсы (только exit-код)
echo "" | image_stats; $LASTEXITCODE           # -> 66
echo P5 | image_stats; $LASTEXITCODE           # -> 65
```

---

## Задача 4 — Фильтр изображения

Применяет преобразование к PPM `P3`, читает из stdin, пишет валидный PPM
в stdout. Использует общий модуль `common/ppm_io` (парсинг заголовка,
буфер пикселей, потоковую запись через `PpmWriter`).

Работает в конвейере:
```
gen_image 64 gradient | image_filter --grayscale
gen_image 4 gradient | image_filter --threshold 128 | image_stats
```

### Режимы

```
image_filter --grayscale      -> конверсия в оттенки серого по luma
image_filter --threshold T    -> бинаризация по порогу яркости (0 <= T <= 255)
```

- `--grayscale`: `y = round(0.299·R + 0.587·G + 0.114·B)`, пиксель -> `(y, y, y)`
- `--threshold T`: пиксель с `luma > T` -> белый `(255,255,255)`, иначе чёрный `(0,0,0)`

### Классы ошибок
- Без аргументов / неизвестный режим / `--threshold` без T / T не число или вне `[0; 255]` -> exit 64, stderr
- Пустой ввод -> exit 66
- Битый формат PPM (из `ppm_io`) -> exit 65
- IO-сбой -> exit 74

### Тесты

Юнит-тесты (luma, grayscale, threshold, парсинг аргументов) и integration
(полный конвейер `argv -> stdin -> stdout` через `run_filter`):
```
build/filter_test.exe      # C++
build/filter_test_c.exe    # C
```

Сборка (пример C++):
```
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /c common/cpp/ppm_io.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /c 04-image-filter/cpFp/filter.cpp
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /fsanitize=address /c 04-image-filter/cpp/filter_test.cpp
link /DEBUG ppm_io.obj filter.obj filter_test.obj /OUT:build/filter_test.exe
```

### Модуль `common/ppm_io` и сборка DLL

Задачи 1/3/4 используют общий модуль ввода-вывода PPM. Он собирается тремя
способами (слой 9 лестницы):

- **Объектный файл / статическая библиотека** — по умолчанию, без флагов
- **Динамическая библиотека (DLL / .so)** — с флагом `KV_DYNAMIC_LINK`:

```
# C++ DLL + import-lib
cl /std:c++latest /W4 /permissive- /EHsc /Od /Zi /MDd /D KV_DYNAMIC_LINK /LD common/cpp/ppm_io.cpp /link /OUT:build/ppm_io.dll /IMPLIB:build/ppm_io.lib

# линковка потребителя через import-lib (вместо ppm_io.obj)
link /DEBUG filter.obj filter_test.obj build/ppm_io.lib /OUT:build/filter_test_dll.exe
```

- **Linux / macOS** — флаг игнорируется, символы `.so` экспортируются по умолчанию

Макрос экспорта: `KV_API` = `__declspec(dllexport)` на Windows при
`KV_DYNAMIC_LINK`, иначе пусто. `dllimport` не используется: потребитель,
линкующий import-lib, работает без него.

В C++ `Image` реализован как PIMPL-класс (move-only): поля скрыты за
`std::unique_ptr<Impl>`, публичный API — через аксессоры (`width()`,
`height()`, `pixel_count()`, `pixels()`). Это даёт стабильный layout
объекта и снимает dependency на `<vector>` из заголовка.

---

## Массовые тестовые данные

`gen_mass_test_data.ps1` генерирует PPM в `build/test_data/`:

```
chcp 1251 >$null
gen_mass_test_data.ps1          # C
gen_mass_test_data.ps1 -Cpp     # C++
```

Создаёт 15 файлов: random (2 * 2, 64 * 64, 1024 * 1024 * 3 seed) + reference
(gradient/checker/radial 2 * 2, 4 * 4).

> Перед запуском установите кодовую страницу консоли: `chcp 1251 >$null`.
> Это обеспечит корректный вывод русского текста программами.

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