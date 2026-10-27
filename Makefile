# =============================================================================
#  Makefile — единая точка сборки, тестов и приёмки проекта.
#
#  Один вызов make = один вариант сборки. Вариант задают ручки
#  (PLATFORM, CONFIG, RUNTIME, LINK, SANITIZE, COVERAGE, TOOLCHAIN); каталог
#  варианта кодирует их все, поэтому смена ручки не подхватывает чужие
#  объекты. PLATFORM задаётся явно, автоопределения нет.
#
#  Примеры:
#    make PLATFORM=linux                        debug, ASan, LINK=lib
#    make PLATFORM=linux test                   unit + приёмка
#    make PLATFORM=windows CONFIG=release       release, /MD
#    make PLATFORM=windows RUNTIME=static       release, /MT
#    make PLATFORM=windows LINK=dll             потребители через ppm_io.dll
#    make PLATFORM=linux 01-image-gen           код задачи 1
#    make PLATFORM=linux format-check           clang-format
#    make PLATFORM=linux help                   ручки и цели
#
#  Точка входа приложения — main.<ext>; юнит-тест — <имя>_test.<ext>;
#  остальные исходники каталога задачи — общая часть (линкуется и в exe,
#  и в тест). Так исходники задачи собираются глобом $(wildcard), а не
#  перечислением: новый файл подхватывается сам.
#
#  Соглашения об экранировании в рецептах:
#    $$ — escape слоя make, превращается в один $. Нужен, чтобы автоматические
#         переменные ($@, $<) и вызовы make-функций не раскрылись на проходах
#         $(eval $(call ...)), а дожили до исполнения рецепта; и чтобы shell
#         получил $$? (код возврата) в POSIX-ветке.
#    %% — escape слоя cmd (только Windows): make отдаёт рецепт в cmd, где один
#         % начинает подстановку, поэтому переменную цикла for пишем %%A.
#
#  Соглашение о путях:
#    Цели и пререквизиты make — всегда через / (иначе не строится граф).
#    Всё, что уходит в shell (запуск exe, < >, del/copy/type/mkdir) —
#    через native/RUN: \ на Windows, / на POSIX. Так требуют cmd-парсер
#    (относительный путь с / не запускается) и встроенные команды.
#    Пути для cl/link/clang-format не конвертируются — они принимают /.
# =============================================================================

MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# Сухой прогон (make -n) — читаем до того, как допишем свои MAKEFLAGS.
DRY_RUN := $(findstring n,$(filter-out -%,$(firstword $(MAKEFLAGS))))

ifneq ($(firstword $(sort $(MAKE_VERSION) 4.4)),4.4)
  $(error Нужен GNU make >= 4.4, а это $(MAKE_VERSION). Установка: winget install ezwinports.make)
endif

# Упавший рецепт удаляет недописанную цель.
.DELETE_ON_ERROR:
# Промежуточные файлы (выходы генераторов) make не удаляет никогда.
.NOTINTERMEDIATE:
.SECONDEXPANSION:

# =============================================================================
#  Ручки варианта (комментарии — отдельной строкой: `X ?= a  # ...` оставил
#  бы хвостовые пробелы в значении).
# =============================================================================
# windows | linux. Обязательная ручка: автоопределение не выполняется.
PLATFORM ?=

ifeq ($(PLATFORM),)
  $(error PLATFORM не задан. Укажите её явно: make PLATFORM=windows ... или make PLATFORM=linux ...)
endif

TOOLCHAIN_windows := msvc
TOOLCHAIN_linux   := gcc
# msvc | gcc
TOOLCHAIN ?= $(TOOLCHAIN_$(PLATFORM))
# debug | release
CONFIG ?= debug
# dynamic (/MD, libstdc++.so) | static (/MT, -static-libstdc++) | full (-static, только gcc)
RUNTIME ?= dynamic
# Как потребители получают common/ppm_io: obj | lib | dll
LINK ?= lib
# У MSVC link.exe читает одноимённую переменную окружения: LINK=dll
# превратился бы во входной файл dll.obj.
unexport LINK
# address | none. По умолчанию ASan в debug, выключен в release.
SANITIZE_DEFAULT_debug   := address
SANITIZE_DEFAULT_release := none
SANITIZE ?= $(SANITIZE_DEFAULT_$(CONFIG))
# 0 | 1. Отдельная coverage-сборка (сама цель coverage — задел).
COVERAGE ?= 0
# 0 | 1. Предупреждения GCC как ошибки (WERROR=1 -> -Werror).
WERROR ?= 0
# Суффикс версии gcc: пусто — системные gcc/g++, make GCC_SUFFIX=-15 даст gcc-15 / g++-15.
GCC_SUFFIX ?=
# clang-format: имя в PATH или полный путь; версия не фиксируется.
CLANG_FORMAT ?= clang-format

# =============================================================================
#  Модель ошибок сборки: недопустимое значение или сочетание останавливает
#  make при разборе, до первой компиляции.
# =============================================================================
check_value = $(if $(filter-out 1,$(words $($1))),$(error $1="$($1)": нужно ровно одно значение из: $2),$(if $(filter $2,$($1)),,$(error $1=$($1): ожидается одно из: $2)))
$(call check_value,PLATFORM,windows linux)
$(call check_value,TOOLCHAIN,msvc gcc)
$(call check_value,CONFIG,debug release)
$(call check_value,RUNTIME,dynamic static full)
$(call check_value,LINK,obj lib dll)
$(call check_value,SANITIZE,address none)
$(call check_value,COVERAGE,0 1)
$(call check_value,WERROR,0 1)

# Единственный источник правды о недопустимых сочетаниях. Аргументы:
# 1 тулчейн, 2 config, 3 runtime, 4 link, 5 sanitize, 6 coverage.
exclusion = $(strip \
  $(if $(and $(filter msvc,$1),$(filter full,$3)),[тулчейн] RUNTIME=full есть только у gcc.) \
  $(if $(and $(filter dll,$4),$(filter-out dynamic,$3)),[решение] DLL только с динамическим runtime: через границу идут FILE* и std::istream.) \
  $(if $(and $(filter gcc,$1),$(filter dll,$4)),[решение] .so отложен: поставка на Linux — полная статика.) \
  $(if $(and $(filter full,$3),$(filter address,$5)),[тулчейн] ASan несовместим с -static.) \
  $(if $(filter 1,$6),$(if $(filter-out debug,$2),[coverage] только CONFIG=debug.)) \
  $(if $(filter 1,$6),$(if $(filter-out none,$5),[coverage] только SANITIZE=none.)) \
  $(if $(filter 1,$6),$(if $(filter-out obj,$4),[coverage] только LINK=obj.)))

EXCLUSION := $(call exclusion,$(TOOLCHAIN),$(CONFIG),$(RUNTIME),$(LINK),$(SANITIZE),$(COVERAGE))
ifneq ($(EXCLUSION),)
  $(error Недопустимый вариант: $(EXCLUSION))
endif

# =============================================================================
#  Раскладка.
# =============================================================================
VARIANT := $(TOOLCHAIN)-$(CONFIG)-$(RUNTIME)-$(LINK)-$(SANITIZE)$(if $(filter 1,$(COVERAGE)),-cov)
BUILD_DIR := build/$(VARIANT)
TEST_DIR  := $(BUILD_DIR)/test_data
REF_DIR   := $(TEST_DIR)/ref

# 1 | 0: потребители берут common/ppm_io через DLL.
DLL_ENABLED := $(if $(filter dll,$(LINK)),1,0)

# =============================================================================
#  Платформенный блок: оболочка, файловые операции, запуск и сравнение.
# =============================================================================
ifeq ($(PLATFORM),windows)
  SHELL := cmd.exe
  native = $(subst /,\,$1)
  MKDIR  = mkdir $(call native,$(patsubst %/,%,$1))
  RMFILE = if exist $(call native,$1) del /F /Q $(call native,$1)
  RMTREE = if exist $(call native,$1) rmdir /S /Q $(call native,$1)
  CP     = copy /Y $(call native,$1) $(call native,$2) > NUL
  CMP    = fc /B $(call native,$1) $(call native,$2)
  RUN    = $(call native,$1)
  NUL    = NUL
  TOUCH  = type nul > $(call native,$1)
  # Точный код возврата. `if errorlevel N` значит «>= N», поэтому сначала
  # отсекаем всё больше ожидаемого, затем добираем остаток до него.
  CHECK0  =
  CHECK64 = & if errorlevel 65 exit /b 1 & if errorlevel 64 exit /b 0 & exit /b 1
  CHECK65 = & if errorlevel 66 exit /b 1 & if errorlevel 65 exit /b 0 & exit /b 1
  CHECK66 = & if errorlevel 67 exit /b 1 & if errorlevel 66 exit /b 0 & exit /b 1
  # Непустой stderr. Под cmd переменную цикла for пишем как %%A: make отдаёт
  # рецепт в cmd, где один % начинает подстановку и превращается в пусто.
  NONEMPTY = for %%A in ($(call native,$(ERR))) do if %%~zA==0 exit /b 1
  which = $(shell where $1 2>nul)
else
  SHELL := /bin/sh
  native = $1
  MKDIR  = mkdir -p $1
  RMFILE = rm -f $1
  RMTREE = rm -rf $1
  CP     = cp -f $1 $2
  CMP    = cmp $1 $2
  RUN    = $1
  NUL    = /dev/null
  TOUCH  = touch $1
  CHECK0  =
  CHECK64 = ; test $$? -eq 64
  CHECK65 = ; test $$? -eq 65
  CHECK66 = ; test $$? -eq 66
  NONEMPTY = test -s $(ERR)
  which = $(shell command -v $1 2>/dev/null)
endif

# =============================================================================
#  Блок тулчейна.
# =============================================================================
ifeq ($(TOOLCHAIN),msvc)
  OBJ         := .obj
  EXE         := .exe
  DLLEXT      := .dll
  IMPLIBEXT   := .lib
  STATIC_NAME := ppm_io.lib
  COMPILER    := cl

  WARN    := -nologo -W4 -permissive- -utf-8
  CSTD    := -std:c17
  CXXSTD  := -std:c++latest -EHsc
  CFG_debug   := -Od
  CFG_release := -O2 -DNDEBUG
  RT_dynamic_debug   := -MDd
  RT_dynamic_release := -MD
  RT_static_debug    := -MTd
  RT_static_release  := -MT
  SAN_address := -fsanitize=address
  SAN_none    :=
  DBG := -Zi -FS

  FLAGS_c   := $(CSTD) $(WARN) $(CFG_$(CONFIG)) $(RT_$(RUNTIME)_$(CONFIG)) $(SAN_$(SANITIZE)) $(DBG)
  FLAGS_cpp := $(CXXSTD) $(WARN) $(CFG_$(CONFIG)) $(RT_$(RUNTIME)_$(CONFIG)) $(SAN_$(SANITIZE)) $(DBG)

  LDFLAGS_debug   := -INCREMENTAL:NO
  LDFLAGS_release := -OPT:REF -OPT:ICF
  LDFLAGS := -nologo -DEBUG $(LDFLAGS_$(CONFIG))

  COMPILE_c   = cl $(FLAGS_c) -c $< -Fo$@ -Fd$(dir $@)compile.pdb
  COMPILE_cpp = cl $(FLAGS_cpp) -c $< -Fo$@ -Fd$(dir $@)compile.pdb
  COMPILE_c_dll   = cl $(FLAGS_c) -DKV_DYNAMIC_LINK -c $< -Fo$@ -Fd$(dir $@)compile.pdb
  COMPILE_cpp_dll = cl $(FLAGS_cpp) -DKV_DYNAMIC_LINK -c $< -Fo$@ -Fd$(dir $@)compile.pdb
  LINK_c   = link $(LDFLAGS) -OUT:$@ $^
  LINK_cpp = link $(LDFLAGS) -OUT:$@ $^
  MAKELIB  = lib -nologo -OUT:$@ $^
  MAKEDLL_c   = link $(LDFLAGS) -DLL -OUT:$(BUILD_DIR)/common/c/dll/ppm_io$(DLLEXT) -IMPLIB:$(BUILD_DIR)/common/c/dll/ppm_io$(IMPLIBEXT) $^
  MAKEDLL_cpp = link $(LDFLAGS) -DLL -OUT:$(BUILD_DIR)/common/cpp/dll/ppm_io$(DLLEXT) -IMPLIB:$(BUILD_DIR)/common/cpp/dll/ppm_io$(IMPLIBEXT) $^

else ifeq ($(TOOLCHAIN),gcc)
  OBJ         := .o
  EXE         :=
  DLLEXT      := .so
  IMPLIBEXT   := .so
  STATIC_NAME := libppm_io.a
  COMPILER    := g++$(GCC_SUFFIX)

  WARN    := -Wall -Wextra -Wpedantic$(if $(filter 1,$(WERROR)), -Werror)
  CSTD    := -std=c17
  CXXSTD  := -std=c++23
  CFG_debug   := -O0
  CFG_release := -O2 -DNDEBUG
  SAN_address := -fsanitize=address -fno-omit-frame-pointer
  SAN_none    :=
  DBG := -g

  FLAGS_c   := $(CSTD) $(WARN) $(CFG_$(CONFIG)) $(SAN_$(SANITIZE)) $(DBG)
  FLAGS_cpp := $(CXXSTD) $(WARN) $(CFG_$(CONFIG)) $(SAN_$(SANITIZE)) $(DBG)

  LD_RT_dynamic :=
  LD_RT_static  := -static-libstdc++ -static-libgcc
  LD_RT_full    := -static
  LDLIBS_c      := -lm
  LDFLAGS := $(LD_RT_$(RUNTIME)) $(SAN_$(SANITIZE))

  COMPILE_c   = gcc$(GCC_SUFFIX) $(FLAGS_c) -MMD -MP -c $< -o $@
  COMPILE_cpp = g++$(GCC_SUFFIX) $(FLAGS_cpp) -MMD -MP -c $< -o $@
  LINK_c   = gcc$(GCC_SUFFIX) $(LDFLAGS) -o $@ $^ $(LDLIBS_c)
  LINK_cpp = g++$(GCC_SUFFIX) $(LDFLAGS) -o $@ $^
  MAKELIB  = ar rcs $@ $^
endif

DLL_TARGETS := $(if $(filter 1,$(DLL_ENABLED)),$(BUILD_DIR)/common/c/dll/ppm_io$(DLLEXT) $(BUILD_DIR)/common/cpp/dll/ppm_io$(DLLEXT))

# Проверка окружения — только если цели нужен компилятор и это не make -n.
NO_TOOLCHAIN_GOALS := help clean clean-all format format-check coverage
NEEDS_TOOLCHAIN := $(filter-out $(NO_TOOLCHAIN_GOALS),$(or $(MAKECMDGOALS),all))
ifneq ($(and $(NEEDS_TOOLCHAIN),$(if $(DRY_RUN),,yes)),)
  ifeq ($(call which,$(COMPILER)),)
    $(error Не найден $(COMPILER). $(if $(filter msvc,$(TOOLCHAIN)),Запустите make из Developer-окружения (vcvars64.bat).,Установите компилятор или задайте GCC_SUFFIX.))
  endif
endif

# =============================================================================
#  Вспомогательные штуки приёмки (используются из рецептов, т.е. раскрываются
#  при запуске — поэтому в eval-шаблонах вызываются через `$$`).
# =============================================================================
ERR = $@.err

# Запуск команды с перенаправлением stdout в файл (пути — нативные).
# $1 команда (exe и аргументы), $2 stdout-файл, $3 stdin-файл (пусто — пустой ввод из NUL)
run_to = $(call RUN,$1) $(if $3,< $(call native,$3),< $(NUL)) > $(call native,$2)

# Запуск команды с ожидаемым кодом возврата: stdout отбрасывается, stderr —
# в $@.err (проверяется на непустоту отдельно). $1 команда, $2 код, $3 stdin-файл (пусто — пустой ввод из NUL).
expect = $(call RUN,$1) $(if $3,< $(call native,$3),< $(NUL)) > $(NUL) 2> $(call native,$(ERR)) $(CHECK$2)

# Непустой stderr нужен только на ошибочных кодах (--help/--version дают
# пустой stderr законно). $1 ожидаемый код.
stderr_check = $(if $(filter 0,$1),,$(NONEMPTY))

# Поле таблицы: «-» = пусто, «+» = пробел.
field = $(if $(filter -,$1),,$(subst +, ,$1))

# =============================================================================
#  Общие правила.
# =============================================================================

# Каталоги — order-only пререквизиты (появление файлов внутри не вызывает
# пересборку). MKDIR создаёт промежуточные каталоги.
%/:
	$(call MKDIR,$@)

# Компиляция: язык определяется расширением исходника; путь объекта
# повторяет путь исходника внутри каталога варианта.
$(BUILD_DIR)/%$(OBJ): %.c | $$(@D)/
	$(COMPILE_c)
$(BUILD_DIR)/%$(OBJ): %.cpp | $$(@D)/
	$(COMPILE_cpp)

# =============================================================================
#  Модуль common/ppm_io.
# =============================================================================
COMMON_C_SRCS   := ppm_io strerror
COMMON_CPP_SRCS := ppm_io

COMMON_C_HDRS   := common/c/ppm_io.h common/c/ppm_io_alloc.h common/c/api.h \
                   common/c/strerror.h common/c/luma.h common/c/exit_codes.h
COMMON_CPP_HDRS := common/cpp/ppm_io.hpp common/cpp/luma.hpp common/cpp/exit_codes.hpp common/c/api.h

COMMON_C_OBJS   := $(patsubst %,$(BUILD_DIR)/common/c/%$(OBJ),$(COMMON_C_SRCS))
COMMON_CPP_OBJS := $(patsubst %,$(BUILD_DIR)/common/cpp/%$(OBJ),$(COMMON_CPP_SRCS))
COMMON_C_DLL_OBJS   := $(patsubst %,$(BUILD_DIR)/common/c/dll/%$(OBJ),$(COMMON_C_SRCS))
COMMON_CPP_DLL_OBJS := $(patsubst %,$(BUILD_DIR)/common/cpp/dll/%$(OBJ),$(COMMON_CPP_SRCS))

STATIC_LIB_c   := $(BUILD_DIR)/common/c/$(STATIC_NAME)
STATIC_LIB_cpp := $(BUILD_DIR)/common/cpp/$(STATIC_NAME)
IMPLIB_c   := $(BUILD_DIR)/common/c/dll/ppm_io$(IMPLIBEXT)
IMPLIB_cpp := $(BUILD_DIR)/common/cpp/dll/ppm_io$(IMPLIBEXT)

$(COMMON_C_OBJS) $(COMMON_C_DLL_OBJS): $(COMMON_C_HDRS)
$(COMMON_CPP_OBJS) $(COMMON_CPP_DLL_OBJS): $(COMMON_CPP_HDRS)

# common/c/int_token.h - общий заголовок, но включают его не все модули: зависимость
# ведётся по объектам, чтобы правка не пересобирала task3 и саму библиотеку ppm_io.
INT_TOKEN_HDR := common/c/int_token.h
$(BUILD_DIR)/01-image-gen/c/parse_args$(OBJ): $(INT_TOKEN_HDR)
$(BUILD_DIR)/02-image-passport/c/read_passport$(OBJ): $(INT_TOKEN_HDR)
$(BUILD_DIR)/04-image-filter/c/filter$(OBJ): $(INT_TOKEN_HDR)
$(BUILD_DIR)/common/c/ppm_io_test$(OBJ): $(INT_TOKEN_HDR)

$(STATIC_LIB_c): $(COMMON_C_OBJS) | $(BUILD_DIR)/common/c/
	$(call RMFILE,$@)
	$(MAKELIB)
$(STATIC_LIB_cpp): $(COMMON_CPP_OBJS) | $(BUILD_DIR)/common/cpp/
	$(call RMFILE,$@)
	$(MAKELIB)

ifeq ($(DLL_ENABLED),1)
$(BUILD_DIR)/common/c/dll/%$(OBJ): common/c/%.c | $(BUILD_DIR)/common/c/dll/
	$(COMPILE_c_dll)
$(BUILD_DIR)/common/cpp/dll/%$(OBJ): common/cpp/%.cpp | $(BUILD_DIR)/common/cpp/dll/
	$(COMPILE_cpp_dll)

$(BUILD_DIR)/common/c/dll/ppm_io$(DLLEXT) $(BUILD_DIR)/common/c/dll/ppm_io$(IMPLIBEXT) &: $(COMMON_C_DLL_OBJS) | $(BUILD_DIR)/common/c/dll/
	$(MAKEDLL_c)
$(BUILD_DIR)/common/cpp/dll/ppm_io$(DLLEXT) $(BUILD_DIR)/common/cpp/dll/ppm_io$(IMPLIBEXT) &: $(COMMON_CPP_DLL_OBJS) | $(BUILD_DIR)/common/cpp/dll/
	$(MAKEDLL_cpp)
endif

# Что получает потребитель модуля. $1 язык, $2 вид: none | strerror | ppm_io | static.
MODULE_OBJS_c_none     :=
MODULE_OBJS_c_strerror := $(BUILD_DIR)/common/c/strerror$(OBJ)
MODULE_OBJS_c_ppm_io   := $(COMMON_C_OBJS)
MODULE_OBJS_c_static   := $(COMMON_C_OBJS)
MODULE_OBJS_cpp_none     :=
MODULE_OBJS_cpp_strerror :=
MODULE_OBJS_cpp_ppm_io   := $(COMMON_CPP_OBJS)
MODULE_OBJS_cpp_static   := $(COMMON_CPP_OBJS)

module_inputs = $(if $(filter none,$2),,\
  $(if $(filter obj,$(LINK)),$(MODULE_OBJS_$1_$2),\
  $(if $(and $(filter dll,$(LINK)),$(filter-out static,$2)),$(IMPLIB_$1),$(STATIC_LIB_$1))))

# =============================================================================
#  Задачи. Манифест: одна строка на задачу плюс вид зависимости от модуля
#  на каждый язык. Исходники подхватываются глобом.
# =============================================================================
TASKS := 01-image-gen 02-image-passport 03-image-stats 04-image-filter
LANGS := c cpp

PROG_01-image-gen      := gen_image
PROG_02-image-passport := passport
PROG_03-image-stats    := image_stats
PROG_04-image-filter   := filter

MOD_01-image-gen_c        := ppm_io
MOD_01-image-gen_cpp      := ppm_io
MOD_02-image-passport_c   := strerror
MOD_02-image-passport_cpp := none
MOD_03-image-stats_c      := ppm_io
MOD_03-image-stats_cpp    := ppm_io
MOD_04-image-filter_c     := ppm_io
MOD_04-image-filter_cpp   := ppm_io

APP_01-image-gen_c        := $(BUILD_DIR)/01-image-gen/c/gen_image$(EXE)
APP_01-image-gen_cpp      := $(BUILD_DIR)/01-image-gen/cpp/gen_image$(EXE)
APP_02-image-passport_c   := $(BUILD_DIR)/02-image-passport/c/passport$(EXE)
APP_02-image-passport_cpp := $(BUILD_DIR)/02-image-passport/cpp/passport$(EXE)
APP_03-image-stats_c      := $(BUILD_DIR)/03-image-stats/c/image_stats$(EXE)
APP_03-image-stats_cpp    := $(BUILD_DIR)/03-image-stats/cpp/image_stats$(EXE)
APP_04-image-filter_c     := $(BUILD_DIR)/04-image-filter/c/filter$(EXE)
APP_04-image-filter_cpp   := $(BUILD_DIR)/04-image-filter/cpp/filter$(EXE)

SHARED_HDRS_c   := common/c/version.h common/c/exit_codes.h
SHARED_HDRS_cpp := common/cpp/version.hpp common/cpp/exit_codes.hpp
MODHDRS_none_c       := common/c/luma.h
MODHDRS_none_cpp     := common/cpp/luma.hpp
MODHDRS_strerror_c   := common/c/strerror.h common/c/api.h
MODHDRS_strerror_cpp := common/c/api.h
MODHDRS_ppm_io_c     := $(COMMON_C_HDRS)
MODHDRS_ppm_io_cpp   := $(COMMON_CPP_HDRS)

# Обнаружение исходников. $1 задача, $2 язык.
all_srcs  = $(wildcard $1/$2/*.$2)
app_srcs  = $(filter-out %_test.$2,$(call all_srcs,$1,$2))
core_srcs = $(filter-out $1/$2/main.$2,$(call app_srcs,$1,$2))
test_srcs = $(filter %_test.$2,$(call all_srcs,$1,$2))
# Объекты. $3 — список исходников.
objs = $(patsubst $1/$2/%.$2,$(BUILD_DIR)/$1/$2/%$(OBJ),$3)

ALL_EXES :=
ALL_OBJS :=
UNIT_MARKERS :=

# Правила на задачу/язык: приложение, юнит-тест, зависимости от заголовков,
# маркер прогона, цель задачи. $1 задача, $2 язык.
define TASK_template

    ALL_EXES += $(APP_$1_$2)
    ALL_OBJS += $(call objs,$1,$2,$(call all_srcs,$1,$2))

    TEST_$1_$2 := $(BUILD_DIR)/$1/$2/$(notdir $(basename $(call test_srcs,$1,$2)))$(EXE)
    ALL_EXES += $$(TEST_$1_$2)

    $(APP_$1_$2): $(call objs,$1,$2,$(call app_srcs,$1,$2)) $(call module_inputs,$2,$(MOD_$1_$2)) | $(BUILD_DIR)/$1/$2/
	    $$(LINK_$2)

    $$(TEST_$1_$2): $(call objs,$1,$2,$(call core_srcs,$1,$2)) $(call objs,$1,$2,$(call test_srcs,$1,$2)) $(call module_inputs,$2,static) | $(BUILD_DIR)/$1/$2/
	    $$(LINK_$2)

    $(call objs,$1,$2,$(call all_srcs,$1,$2)): $(wildcard $1/$2/*.h $1/$2/*.hpp) $(SHARED_HDRS_$2) $(MODHDRS_$(MOD_$1_$2)_$2)

    UNIT_$1_$2 := $(patsubst %$(EXE),%,$$(TEST_$1_$2)).passed
    $$(UNIT_$1_$2): $$(TEST_$1_$2) | $(BUILD_DIR)/$1/$2/
	    $$(call RMFILE,$$@)
	    $$(call RUN,$$<)
	    $$(call TOUCH,$$@)
    UNIT_MARKERS += $$(UNIT_$1_$2)

    .PHONY: $1
    $1: $(APP_$1_$2)
endef
$(foreach t,$(TASKS),$(foreach l,$(LANGS),$(eval $(call TASK_template,$t,$l))))

# Копия DLL рядом с потребителем (иначе при запуске — ошибка разрешения).
ifeq ($(DLL_ENABLED),1)
define DLLCOPY_template
    $(APP_$1_$2): | $(BUILD_DIR)/$1/$2/ppm_io$(DLLEXT)
    $(BUILD_DIR)/$1/$2/ppm_io$(DLLEXT): $(BUILD_DIR)/common/$2/dll/ppm_io$(DLLEXT) | $(BUILD_DIR)/$1/$2/
	    $$(call CP,$$<,$$@)
endef
$(foreach t,$(TASKS),$(foreach l,$(LANGS),$(if $(filter strerror ppm_io,$(MOD_$(t)_$l)),$(eval $(call DLLCOPY_template,$t,$l)),)))
endif

# Тесты самого модуля (линкуются статически).
$(BUILD_DIR)/common/c/ppm_io_test$(EXE): $(BUILD_DIR)/common/c/ppm_io_test$(OBJ) $(call module_inputs,c,static) | $(BUILD_DIR)/common/c/
	$(LINK_c)
$(BUILD_DIR)/common/cpp/ppm_io_test$(EXE): $(BUILD_DIR)/common/cpp/ppm_io_test$(OBJ) $(call module_inputs,cpp,static) | $(BUILD_DIR)/common/cpp/
	$(LINK_cpp)
$(BUILD_DIR)/common/c/ppm_io_test$(OBJ): $(COMMON_C_HDRS)
$(BUILD_DIR)/common/cpp/ppm_io_test$(OBJ): $(COMMON_CPP_HDRS)
ALL_EXES += $(BUILD_DIR)/common/c/ppm_io_test$(EXE) $(BUILD_DIR)/common/cpp/ppm_io_test$(EXE)
ALL_OBJS += $(BUILD_DIR)/common/c/ppm_io_test$(OBJ) $(BUILD_DIR)/common/cpp/ppm_io_test$(OBJ)

$(BUILD_DIR)/common/c/ppm_io_test.passed: $(BUILD_DIR)/common/c/ppm_io_test$(EXE)
	$(call RMFILE,$@)
	$(call RUN,$<)
	$(call TOUCH,$@)
$(BUILD_DIR)/common/cpp/ppm_io_test.passed: $(BUILD_DIR)/common/cpp/ppm_io_test$(EXE)
	$(call RMFILE,$@)
	$(call RUN,$<)
	$(call TOUCH,$@)
UNIT_MARKERS += $(BUILD_DIR)/common/c/ppm_io_test.passed $(BUILD_DIR)/common/cpp/ppm_io_test.passed

# =============================================================================
#  Эталонные программы: каждый 0*/ref/*.c — отдельный exe (статический
#  pattern rule, без перечисления имён).
# =============================================================================
REF_SRCS := $(wildcard 0*/ref/*.c)
REF_EXES := $(patsubst %.c,$(BUILD_DIR)/%$(EXE),$(REF_SRCS))
$(REF_EXES): $(BUILD_DIR)/%$(EXE): $(BUILD_DIR)/%$(OBJ)
	$(LINK_c)
ALL_EXES += $(REF_EXES)
ALL_OBJS += $(patsubst %.c,$(BUILD_DIR)/%$(OBJ),$(REF_SRCS))

# =============================================================================
#  Приёмка: данные — узлы графа. Эталоны (ref/) одни на вариант; фактический
#  вывод и вердикты .ok — в каталоге варианта. .ok не создаётся, если
#  команда или сравнение вернули ненулевой код.
# =============================================================================
ACCEPT_OKS :=

T1 := 01-image-gen
T2 := 02-image-passport
T3 := 03-image-stats
T4 := 04-image-filter

# --- Эталоны-изображения: <выход_без_.ppm>:<имя_ref> ---
T1_REFS := gradient_3x3:ref_gradient checker_3x3:ref_checker radial_3x3:ref_radial
T4_REFS := gradient_3x3_grayscale:ref_gradient_3_3_grayscale \
           gradient_3x3_threshold_128:ref_gradient_3_3_threshold_128 \
           radial_3x3_grayscale:ref_radial_3_3_grayscale \
           probe_2x2:ref_probe_2_2 \
           probe_2x2_grayscale:ref_probe_2_2_grayscale \
           probe_2x2_threshold_100:ref_probe_2_2_threshold_100 \
           probe_3x3:ref_probe_3_3 \
           probe_3x3_grayscale:ref_probe_3_3_grayscale \
           probe_3x3_threshold_100:ref_probe_3_3_threshold_100

# $1 строка <выход>:<ref>, $2 каталог задачи
define REFGEN
    $(REF_DIR)/$(word 1,$(subst :, ,$1)).ppm: $(BUILD_DIR)/$2/ref/$(word 2,$(subst :, ,$1))$(EXE) | $(REF_DIR)/
	    $$(call run_to,$$<,$$@,)
endef
$(foreach x,$(T1_REFS),$(eval $(call REFGEN,$x,$(T1))))
$(foreach x,$(T4_REFS),$(eval $(call REFGEN,$x,$(T4))))

# --- Эталон статистики задачи 3: ref_stats на эталонном изображении ---
$(REF_DIR)/stats_gradient_3x3.txt: $(BUILD_DIR)/$(T3)/ref/ref_stats$(EXE) $(REF_DIR)/gradient_3x3.ppm | $(REF_DIR)/
	$(call run_to,$<,$@,$(REF_DIR)/gradient_3x3.ppm)
$(REF_DIR)/stats_checker_3x3.txt: $(BUILD_DIR)/$(T3)/ref/ref_stats$(EXE) $(REF_DIR)/checker_3x3.ppm | $(REF_DIR)/
	$(call run_to,$<,$@,$(REF_DIR)/checker_3x3.ppm)
$(REF_DIR)/stats_radial_3x3.txt: $(BUILD_DIR)/$(T3)/ref/ref_stats$(EXE) $(REF_DIR)/radial_3x3.ppm | $(REF_DIR)/
	$(call run_to,$<,$@,$(REF_DIR)/radial_3x3.ppm)

# --- Задача 2: вход и эталонный вывод по кейсам ---
PP_OK       := basic single_1 plural_2 plural_101 plural_111
PP_ERR      := empty_name:65 no_input:66 bad_count:65 negative:65 zero:65
PP_ERR_NAMES := empty_name no_input bad_count negative zero

# $1 имя кейса
define PP_INPUT
    $(REF_DIR)/passport_$1_in.txt: $(BUILD_DIR)/$(T2)/ref/ref_passport_input$(EXE) | $(REF_DIR)/
	    $$(call run_to,$$< $1,$$@,)
endef
$(foreach c,$(PP_OK) $(PP_ERR_NAMES),$(eval $(call PP_INPUT,$c)))

define PP_EXPECTED
    $(REF_DIR)/passport_$1_expected.txt: $(BUILD_DIR)/$(T2)/ref/ref_passport$(EXE) | $(REF_DIR)/
	    $$(call run_to,$$< $1,$$@,)
endef
$(foreach c,$(PP_OK),$(eval $(call PP_EXPECTED,$c)))

# --- Некорректный формат (P5) — генерируется на лету ---
$(REF_DIR)/bad_format.ppm: | $(REF_DIR)/
	echo P5 > $(call native,$@)

# Кейсы ошибок: <имя>:<код>[:<аргументы>][:<stdin>] («-» = поле пусто, «+» = пробел).
T1_ERRORS := noargs:64:- abc:64:abc zero:64:0 too_big:64:513 bad_pattern:64:3+bogus help:0:--help version:0:--version
T3_ERRORS := empty_input:66:- bad_format:65:$(REF_DIR)/bad_format.ppm
T4_ERRORS := noargs:64:-:- unknown_mode:64:--bogus:- threshold_no_t:64:--threshold:- \
             threshold_256:64:--threshold+256:- threshold_nan:64:--threshold+abc:- \
             help:0:--help:- version:0:--version:- \
             empty_input:66:--grayscale:- bad_format:65:--grayscale:$(REF_DIR)/bad_format.ppm

# =============================================================================
#  Приёмочные правила. По одному вердикту .ok на кейс/язык. Успех — сравнение
#  с эталоном; ошибка — точный код возврата и непустой stderr.
# =============================================================================

# ---- Задача 1: успех (генератор против эталона) ----
# $1 язык, $2 паттерн
define T1_OK
    $(TEST_DIR)/$1/gen_$2_3.ppm: $(APP_$(T1)_$1) | $(TEST_DIR)/$1/
	    $$(call run_to,$(APP_$(T1)_$1) 3 $2,$$@,)
    $(TEST_DIR)/$1/gen_$2_3.ok: $(TEST_DIR)/$1/gen_$2_3.ppm $(REF_DIR)/$2_3x3.ppm | $(TEST_DIR)/$1/
	    $$(call CMP,$$<,$(REF_DIR)/$2_3x3.ppm)
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/gen_$2_3.ok
endef
$(foreach l,$(LANGS),$(foreach p,gradient checker radial,$(eval $(call T1_OK,$l,$p))))

# ---- Задача 1: ошибки ----
# $1 язык, $2 имя, $3 код, $4 аргументы
define T1_ERR
    $(TEST_DIR)/$1/gen_$2.ok: $(APP_$(T1)_$1) | $(TEST_DIR)/$1/
	    $$(call expect,$(APP_$(T1)_$1) $(call field,$4),$3)
	    $$(call stderr_check,$3)
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/gen_$2.ok
endef
$(foreach l,$(LANGS),$(foreach c,$(T1_ERRORS),$(eval $(call T1_ERR,$l,$(word 1,$(subst :, ,$c)),$(word 2,$(subst :, ,$c)),$(word 3,$(subst :, ,$c))))))

# ---- Задача 2: успех ----
# $1 язык, $2 кейс
define T2_OK
    $(TEST_DIR)/$1/passport_$2.txt: $(APP_$(T2)_$1) $(REF_DIR)/passport_$2_in.txt | $(TEST_DIR)/$1/
	    $$(call run_to,$(APP_$(T2)_$1),$$@,$(REF_DIR)/passport_$2_in.txt)
    $(TEST_DIR)/$1/passport_$2.ok: $(TEST_DIR)/$1/passport_$2.txt $(REF_DIR)/passport_$2_expected.txt | $(TEST_DIR)/$1/
	    $$(call CMP,$$<,$(REF_DIR)/passport_$2_expected.txt)
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/passport_$2.ok
endef
$(foreach l,$(LANGS),$(foreach c,$(PP_OK),$(eval $(call T2_OK,$l,$c))))

# ---- Задача 2: ошибки ----
# $1 язык, $2 строка <имя>:<код>
define T2_ERR
    $(TEST_DIR)/$1/passport_$(word 1,$(subst :, ,$2)).ok: $(APP_$(T2)_$1) $(REF_DIR)/passport_$(word 1,$(subst :, ,$2))_in.txt | $(TEST_DIR)/$1/
	    $$(call expect,$(APP_$(T2)_$1),$(word 2,$(subst :, ,$2)),$(REF_DIR)/passport_$(word 1,$(subst :, ,$2))_in.txt)
	    $$(call stderr_check,$(word 2,$(subst :, ,$2)))
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/passport_$(word 1,$(subst :, ,$2)).ok
endef
$(foreach l,$(LANGS),$(foreach c,$(PP_ERR),$(eval $(call T2_ERR,$l,$c))))

# ---- Задача 3: успех (конвейер gen | stats через файл) ----
# $1 язык, $2 паттерн
define T3_OK
    $(TEST_DIR)/$1/stats_$2_3x3.txt: $(APP_$(T3)_$1) $(TEST_DIR)/$1/gen_$2_3.ppm | $(TEST_DIR)/$1/
	    $$(call run_to,$(APP_$(T3)_$1),$$@,$(TEST_DIR)/$1/gen_$2_3.ppm)
    $(TEST_DIR)/$1/stats_$2_3x3.ok: $(TEST_DIR)/$1/stats_$2_3x3.txt $(REF_DIR)/stats_$2_3x3.txt | $(TEST_DIR)/$1/
	    $$(call CMP,$$<,$(REF_DIR)/stats_$2_3x3.txt)
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/stats_$2_3x3.ok
endef
$(foreach l,$(LANGS),$(foreach p,gradient checker radial,$(eval $(call T3_OK,$l,$p))))

# ---- Задача 3: ошибки ----
# $1 язык, $2 строка <имя>:<код>:<stdin>
define T3_ERR
    $(TEST_DIR)/$1/stats_$(word 1,$(subst :, ,$2)).ok: $(APP_$(T3)_$1) $(call field,$(word 3,$(subst :, ,$2))) | $(TEST_DIR)/$1/
	    $$(call expect,$(APP_$(T3)_$1),$(word 2,$(subst :, ,$2)),$(call field,$(word 3,$(subst :, ,$2))))
	    $$(call stderr_check,$(word 2,$(subst :, ,$2)))
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/stats_$(word 1,$(subst :, ,$2)).ok
endef
$(foreach l,$(LANGS),$(foreach c,$(T3_ERRORS),$(eval $(call T3_ERR,$l,$c))))

# ---- Задача 4: успех ----
# $1 язык, $2 строка <выход>:<вход>:<аргументы>
define T4_OK
    $(TEST_DIR)/$1/filter_$(word 1,$(subst :, ,$2)).ppm: $(APP_$(T4)_$1) $(REF_DIR)/$(word 2,$(subst :, ,$2)).ppm | $(TEST_DIR)/$1/
	    $$(call run_to,$(APP_$(T4)_$1) $(call field,$(word 3,$(subst :, ,$2))),$$@,$(REF_DIR)/$(word 2,$(subst :, ,$2)).ppm)
    $(TEST_DIR)/$1/filter_$(word 1,$(subst :, ,$2)).ok: $(TEST_DIR)/$1/filter_$(word 1,$(subst :, ,$2)).ppm $(REF_DIR)/$(word 1,$(subst :, ,$2)).ppm | $(TEST_DIR)/$1/
	    $$(call CMP,$$<,$(REF_DIR)/$(word 1,$(subst :, ,$2)).ppm)
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/filter_$(word 1,$(subst :, ,$2)).ok
endef
T4_OK_CASES := gradient_3x3_grayscale:gradient_3x3:--grayscale \
               gradient_3x3_threshold_128:gradient_3x3:--threshold+128 \
               radial_3x3_grayscale:radial_3x3:--grayscale \
               probe_2x2_grayscale:probe_2x2:--grayscale \
               probe_2x2_threshold_100:probe_2x2:--threshold+100 \
               probe_3x3_grayscale:probe_3x3:--grayscale \
               probe_3x3_threshold_100:probe_3x3:--threshold+100
$(foreach l,$(LANGS),$(foreach c,$(T4_OK_CASES),$(eval $(call T4_OK,$l,$c))))

# ---- Задача 4: ошибки ----
# $1 язык, $2 строка <имя>:<код>:<аргументы>:<stdin>
define T4_ERR
    $(TEST_DIR)/$1/filter_$(word 1,$(subst :, ,$2)).ok: $(APP_$(T4)_$1) $(call field,$(word 4,$(subst :, ,$2))) | $(TEST_DIR)/$1/
	    $$(call expect,$(APP_$(T4)_$1) $(call field,$(word 3,$(subst :, ,$2))),$(word 2,$(subst :, ,$2)),$(call field,$(word 4,$(subst :, ,$2))))
	    $$(call stderr_check,$(word 2,$(subst :, ,$2)))
	    $$(call TOUCH,$$@)
    ACCEPT_OKS += $(TEST_DIR)/$1/filter_$(word 1,$(subst :, ,$2)).ok
endef
$(foreach l,$(LANGS),$(foreach c,$(T4_ERRORS),$(eval $(call T4_ERR,$l,$c))))

# =============================================================================
#  Cross-parity: успешные кейсы C и C++ на одном входе.
# =============================================================================
PARITY_OKS :=

define PARITY_PAIR
    $(TEST_DIR)/parity/$1.ok: $2 $3 | $(TEST_DIR)/parity/
	    $$(call CMP,$2,$3)
	    $$(call TOUCH,$$@)
    PARITY_OKS += $(TEST_DIR)/parity/$1.ok
endef

$(foreach p,gradient checker radial,\
  $(eval $(call PARITY_PAIR,t1_$p,$(TEST_DIR)/c/gen_$p_3.ppm,$(TEST_DIR)/cpp/gen_$p_3.ppm)))

$(foreach c,$(PP_OK),\
  $(eval $(call PARITY_PAIR,t2_$c,$(TEST_DIR)/c/passport_$c.txt,$(TEST_DIR)/cpp/passport_$c.txt)))

$(foreach p,gradient checker radial,\
  $(eval $(call PARITY_PAIR,t3_$p,$(TEST_DIR)/c/stats_$p_3x3.txt,$(TEST_DIR)/cpp/stats_$p_3x3.txt)))

$(foreach c,$(T4_OK_CASES),\
  $(eval $(call PARITY_PAIR,t4_$(word 1,$(subst :, ,$c)),\
    $(TEST_DIR)/c/filter_$(word 1,$(subst :, ,$c)).ppm,\
    $(TEST_DIR)/cpp/filter_$(word 1,$(subst :, ,$c)).ppm)))

# =============================================================================
#  Фазовые цели.
# =============================================================================
.PHONY: all lib exes unit acceptance parity test common clean clean-all help \
        format format-check coverage
.DEFAULT_GOAL := all

all: lib exes test
lib: $(STATIC_LIB_c) $(STATIC_LIB_cpp) $(DLL_TARGETS)
exes: $(ALL_EXES)
common: $(STATIC_LIB_c) $(STATIC_LIB_cpp) $(DLL_TARGETS)

unit: $(UNIT_MARKERS)
acceptance: $(ACCEPT_OKS)
parity: $(PARITY_OKS)
test: format-check unit acceptance parity

clean:
	$(call RMTREE,$(BUILD_DIR))
clean-all:
	$(call RMTREE,build)

coverage:
	$(error coverage — задел: COVERAGE=1 уже задаёт флаги и ограничения варианта)

# =============================================================================
#  Форматирование.
# =============================================================================
FORMAT_SRCS := $(wildcard common/c/*.c common/c/*.h common/cpp/*.cpp common/cpp/*.hpp \
                           0*/c/*.c 0*/c/*.h 0*/cpp/*.cpp 0*/cpp/*.hpp 0*/cpp/*.h 0*/ref/*.c 0*/ref/*.h)

FORMAT_GOALS := format format-check test all
NEEDS_FORMAT := $(if $(filter $(FORMAT_GOALS),$(or $(MAKECMDGOALS),all)),1,)
ifneq ($(and $(NEEDS_FORMAT),$(if $(DRY_RUN),,yes)),)
  ifeq ($(shell "$(CLANG_FORMAT)" --version 2>$(NUL)),)
    $(error Не найден clang-format. Добавьте его в PATH или задайте CLANG_FORMAT=<путь>.)
  endif
endif

format-check:
	"$(CLANG_FORMAT)" --dry-run --Werror $(FORMAT_SRCS)

format:
	"$(CLANG_FORMAT)" -i $(FORMAT_SRCS)

# =============================================================================
help:
	$(info Ручки: PLATFORM=$(PLATFORM) TOOLCHAIN=$(TOOLCHAIN) CONFIG=$(CONFIG) RUNTIME=$(RUNTIME) LINK=$(LINK) SANITIZE=$(SANITIZE) COVERAGE=$(COVERAGE))
	$(info Каталог варианта: $(BUILD_DIR))
	$(info Цели: all lib exes unit acceptance parity test common clean clean-all help)
	$(info Задачи: $(TASKS))
	$(info format-check format coverage)

# Точные header-зависимости от gcc -MMD (на MSVC файлов .d нет).
-include $(patsubst %$(OBJ),%.d,$(ALL_OBJS))
