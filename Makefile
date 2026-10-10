# =============================================================================
#  Makefile — сборка, юнит-тесты и приёмка всего проекта. Версия 1. Ассистент - Claude Opus v5.5; используются анти-подхалимажные приемы и итеративный подход
#
#  Один вызов make = один вариант сборки = один граф. Вариант задают ручки
#  (CONFIG, RUNTIME, LINK, SANITIZE); каталог варианта кодирует их все.
#  Перебор допустимых вариантов — цели print-variants и matrix (решение «б»:
#  допустимость сочетаний описана только здесь).
#
#  Рецепты — однострочники через переменные платформы (MKDIR, CP, TOUCH, CMP,
#  RMTREE, RUN) и функции тулчейна (compile, link_exe, make_lib, make_dll).
#  Перенаправления, конвейеры и проверка ожидаемого кода возврата вынесены
#  в tools/run_case.c — в рецептах нет < > |.
#
#  Примеры:
#    make                              debug, /MDd, LINK=lib, ASan — собрать всё
#    make test                         юнит-тесты и приёмка текущего варианта
#    make CONFIG=release LINK=dll test release, потребители через ppm_io.dll
#    make print-variants               список допустимых и исключённых вариантов
#    make -k matrix                    test по всем допустимым вариантам
#    make format-check                 проверка clang-format (без компилятора)
#    make help                         ручки и цели
#
#  Статус блоков: MSVC — заполнен; GCC — заполнен (Linux, без .so);
#  macOS/clang, coverage, SDK-пакет, документация — следующие заходы.
# =============================================================================

# Флаги сухого прогона определяются ДО того, как мы допишем свои MAKEFLAGS:
# первое слово MAKEFLAGS — склейка однобуквенных опций (n, k, s...).
DRY_RUN := $(findstring n,$(filter-out -%,$(firstword $(MAKEFLAGS))))

MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# Требование ТЗ — make 4.4+: .NOTINTERMEDIATE появился в 4.4.
# Сравнение строковое: sort ставит меньшую версию первой.
ifneq ($(firstword $(sort $(MAKE_VERSION) 4.4)),4.4)
  $(error Нужен GNU make >= 4.4, а это $(MAKE_VERSION). Windows: winget install ezwinports.make)
endif

# Цель упавшего рецепта удаляется: недописанный файл не сойдёт за свежий.
.DELETE_ON_ERROR:
# Промежуточные файлы (выходы генераторов, отчёты) make не удаляет никогда.
.NOTINTERMEDIATE:
# Нужна для order-only каталогов вида $$(@D)/ в правилах-шаблонах.
.SECONDEXPANSION:

# =============================================================================
#  Ручки варианта. Комментарии — только на отдельных строках: `X ?= a  # ...`
#  оставил бы хвостовые пробелы в значении.
# =============================================================================

# Платформа (ОС и оболочка) — отдельная ось от тулчейна (решение 2).
ifndef PLATFORM
  ifeq ($(OS),Windows_NT)
    PLATFORM := windows
  else
    PLATFORM := $(if $(filter Linux,$(shell uname -s)),linux,$(shell uname -s))
  endif
endif

# msvc | gcc. По умолчанию — родной тулчейн платформы.
TOOLCHAIN_windows := msvc
TOOLCHAIN_linux   := gcc
TOOLCHAIN ?= $(TOOLCHAIN_$(PLATFORM))
# debug | release
CONFIG ?= debug
# dynamic (/MD, libstdc++.so) | static (/MT, -static-libstdc++) | full (-static, только gcc)
RUNTIME ?= dynamic
# Как потребители получают common/ppm_io: obj | lib | dll
LINK ?= lib
# address | none. По умолчанию ASan в debug и выключен в release.
SANITIZE_DEFAULT_debug   := address
SANITIZE_DEFAULT_release := none
SANITIZE ?= $(SANITIZE_DEFAULT_$(CONFIG))
# 0 | 1. Отдельная coverage-сборка (сама цель coverage — следующий заход).
COVERAGE ?= 0
# Суффикс версии gcc: make GCC_SUFFIX=-14 даст gcc-14 / g++-14.
GCC_SUFFIX ?=
# clang-format: путь и закреплённая версия (пусто — версию не проверять).
CLANG_FORMAT ?= clang-format
CLANG_FORMAT_VERSION ?=

# =============================================================================
#  Модель ошибок сборки: недопустимое значение или сочетание останавливает
#  make при разборе, до первой компиляции (решение 11).
# =============================================================================

# Ровно одно слово и только из списка. $(filter список,значение): допустимые
# слова — шаблоны без %, поэтому значение «%» ничему не совпадёт.
check_value = $(if $(filter-out 1,$(words $($1))),$(error $1="$($1)": нужно ровно одно значение из: $2),$(if $(filter $2,$($1)),,$(error $1=$($1): ожидается одно из: $2)))
$(call check_value,PLATFORM,windows linux)
$(call check_value,TOOLCHAIN,msvc gcc)
$(call check_value,CONFIG,debug release)
$(call check_value,RUNTIME,dynamic static full)
$(call check_value,LINK,obj lib dll)
$(call check_value,SANITIZE,address none)
$(call check_value,COVERAGE,0 1)

# Единственный источник правды о недопустимых сочетаниях: им пользуются и
# проверка текущего варианта, и print-variants/matrix.
# Аргументы: 1 тулчейн, 2 config, 3 runtime, 4 link, 5 sanitize, 6 coverage.
# Возвращает причины с видом в скобках или пусто. Запятые в текстах нельзя:
# они разделяют аргументы функций.
#   [решение]  — выбор проекта, записан в журнале;
#   [тулчейн]  — ограничение компилятора, правкой кода не лечится;
#   [код]      — ограничение текущего кода (сейчас таких нет).
exclusion = $(strip \
  $(if $(and $(filter msvc,$1),$(filter full,$3)),[тулчейн] RUNTIME=full (-static) есть только у gcc.) \
  $(if $(and $(filter dll,$4),$(filter-out dynamic,$3)),[решение 5] DLL только с динамическим runtime: через границу идут FILE* и std::istream.) \
  $(if $(and $(filter gcc,$1),$(filter dll,$4)),[решение] .so отложен: поставка на Linux — полная статика.) \
  $(if $(and $(filter full,$3),$(filter address,$5)),[тулчейн] ASan несовместим с -static.) \
  $(if $(filter 1,$6),$(if $(filter-out debug,$2),[coverage] только CONFIG=debug.)) \
  $(if $(filter 1,$6),$(if $(filter-out none,$5),[coverage] только SANITIZE=none.)) \
  $(if $(filter 1,$6),$(if $(filter-out obj,$4),[coverage] только LINK=obj: из .lib линкер берёт лишь нужные объекты и непокрытые файлы выпали бы из отчёта.)))

EXCLUSION := $(call exclusion,$(TOOLCHAIN),$(CONFIG),$(RUNTIME),$(LINK),$(SANITIZE),$(COVERAGE))
ifneq ($(EXCLUSION),)
  $(error Недопустимый вариант: $(EXCLUSION))
endif

# Каталог варианта кодирует ВСЕ оси, меняющие объектный код или данные
# тестов: иначе после смены варианта make сочтёт чужие файлы свежими.
VARIANT := $(TOOLCHAIN)-$(CONFIG)-$(RUNTIME)-$(LINK)-$(SANITIZE)$(if $(filter 1,$(COVERAGE)),-cov)
B  := build/$(VARIANT)
TD := $(B)/test_data
# Инструменты сборки не зависят от ручек варианта — общий каталог на тулчейн.
TOOLS_DIR := build/$(TOOLCHAIN)-tools

# =============================================================================
#  Блок платформы: оболочка, пути, файловые операции.
# =============================================================================
ifeq ($(PLATFORM),windows)
  # Рецепты исполняет cmd.exe (README: перенаправления — только в cmd).
  # Запускать make можно из любой консоли, в т.ч. из PowerShell.
  SHELL := cmd.exe
  native  = $(subst /,\,$1)
  MKDIR   = mkdir $(call native,$(patsubst %/,%,$1))
  RMTREE  = if exist $(call native,$1) rmdir /S /Q $(call native,$1)
  RMFILE  = if exist $(call native,$1) del /F /Q $(call native,$1)
  CP      = copy /Y $(call native,$1) $(call native,$2)
  TOUCH   = type nul > $(call native,$1)
  # Побайтовое сравнение: без /B fc сравнивает как текст.
  CMP     = fc /B $(call native,$1) $(call native,$2)
  RUN     = $(call native,$1)
  which   = $(shell where $1 2>nul)
  EXE     := .exe
else
  native  = $1
  MKDIR   = mkdir -p $1
  RMTREE  = rm -rf $1
  RMFILE  = rm -f $1
  CP      = cp -f $1 $2
  TOUCH   = touch $1
  CMP     = cmp $1 $2
  RUN     = $1
  which   = $(shell command -v $1 2>/dev/null)
  EXE     :=
endif

# =============================================================================
#  Блок тулчейна. Снаружи блока известны только имена: OBJ, STATIC_LIB_NAME,
#  DLL, IMPLIB, FLAGS_c, FLAGS_cpp, LDFLAGS и функции
#    compile(lang,src,obj)  link_exe(lang,exe,inputs)  make_lib(lib,objs)
#    make_dll(lang,dll,implib,objs)  build_tool(src,exe)
# =============================================================================
ifeq ($(TOOLCHAIN),msvc)

  COMPILER := cl
  OBJ := .obj
  STATIC_LIB_NAME := ppm_io.lib
  DLL    := .dll
  IMPLIB := .lib

  WARN    := -nologo -W4 -permissive- -utf-8
  C_STD   := -std:c17
  # c++latest — плавающая цель (решение 18); оставлено как в README.
  CXX_STD := -std:c++latest -EHsc

  CFG_debug   := -Od
  CFG_release := -O2 -DNDEBUG

  RT_dynamic_debug   := -MDd
  RT_dynamic_release := -MD
  RT_static_debug    := -MTd
  RT_static_release  := -MT

  SAN_address := -fsanitize=address
  SAN_none    :=

  # /Zi во всех конфигурациях: PDB лежит рядом с exe и в Release (ТЗ).
  # -FS: объекты одного каталога пишут общий compile.pdb при make -j.
  DBG := -Zi -FS

  LD_BASE := -nologo -DEBUG
  # ASan несовместим с инкрементальной линковкой (иначе LNK4300).
  LD_debug   := -INCREMENTAL:NO
  LD_release := -OPT:REF -OPT:ICF
  # Coverage (Microsoft.CodeCoverage.Console, статическая инструментация).
  # -DEBUG указан явно, хотя /PROFILE его и так подразумевает. По документации
  # MSVC (learn.microsoft.com/cpp/build/reference/profile-performance-tools-profiler)
  # /PROFILE включает /DEBUG:FULL /DEBUGTYPE:cv,fixup /OPT:REF /OPT:NOICF
  # /INCREMENTAL:NO /FIXED:NO — поэтому только в coverage-варианте, не в поставке.
  LD_COV_0 :=
  LD_COV_1 := -DEBUG -PROFILE
  CC_COV_0 :=
  CC_COV_1 :=

  FLAGS_COMMON := $(WARN) $(CFG_$(CONFIG)) $(RT_$(RUNTIME)_$(CONFIG)) $(SAN_$(SANITIZE)) $(DBG) $(CC_COV_$(COVERAGE))
  FLAGS_c   := $(C_STD) $(FLAGS_COMMON)
  FLAGS_cpp := $(CXX_STD) $(FLAGS_COMMON)
  LDFLAGS   := $(LD_BASE) $(LD_$(CONFIG)) $(LD_COV_$(COVERAGE))

  # Свой compile.pdb на каталог объектов: один PDB на весь вариант
  # сериализовал бы параллельную компиляцию через mspdbsrv.
  compile    = cl $(FLAGS_$1) $(EXTRA_DEFS) -c $2 -Fo$3 -Fd$(dir $3)compile.pdb
  # Один линкер для C и C++.
  link_exe   = link $(LDFLAGS) -OUT:$2 $3
  make_lib   = lib -nologo -OUT:$1 $2
  make_dll   = link $(LDFLAGS) -DLL -OUT:$2 -IMPLIB:$3 $4
  build_tool = cl -nologo -W4 -permissive- -utf-8 -std:c17 -O2 -MD $1 -Fo$(dir $2) -Fe$2

else ifeq ($(TOOLCHAIN),gcc)

  CC_gcc  := gcc$(GCC_SUFFIX)
  CXX_gcc := g++$(GCC_SUFFIX)
  COMPILER := $(CXX_gcc)
  OBJ := .o
  STATIC_LIB_NAME := libppm_io.a
  DLL    := .so
  IMPLIB := .so

  WARN    := -Wall -Wextra -Wpedantic
  C_STD   := -std=c17
  # <print> в common/cpp требует libstdc++ из GCC 14.
  CXX_STD := -std=c++23

  CFG_debug   := -O0
  CFG_release := -O2 -DNDEBUG
  # Аналог /Zi: отладочная информация всегда; вынос в .debug — этап поставки.
  DBG := -g

  SAN_address := -fsanitize=address -fno-omit-frame-pointer
  SAN_none    :=

  # Runtime на GCC — свойство линковки, не компиляции.
  LD_RT_dynamic :=
  LD_RT_static  := -static-libstdc++ -static-libgcc
  LD_RT_full    := -static

  CC_COV_0 :=
  CC_COV_1 := --coverage
  LD_COV_0 :=
  LD_COV_1 := --coverage

  FLAGS_COMMON := $(WARN) $(CFG_$(CONFIG)) $(SAN_$(SANITIZE)) $(DBG) $(CC_COV_$(COVERAGE))
  FLAGS_c   := $(C_STD) $(FLAGS_COMMON)
  FLAGS_cpp := $(CXX_STD) $(FLAGS_COMMON)
  LDFLAGS   := $(LD_RT_$(RUNTIME)) $(SAN_$(SANITIZE)) $(LD_COV_$(COVERAGE))

  COMPILER_c   := $(CC_gcc)
  COMPILER_cpp := $(CXX_gcc)
  # На GCC линкует драйвер своего языка: g++ подтягивает libstdc++.
  LINKER_c     := $(CC_gcc)
  LINKER_cpp   := $(CXX_gcc)

  # -MMD -MP: автоматические header-зависимости (у cl аналога нет).
  compile    = $(COMPILER_$1) $(FLAGS_$1) $(EXTRA_DEFS) -MMD -MP -c $2 -o $3
  link_exe   = $(LINKER_$1) -o $2 $3 $(LDFLAGS)
  make_lib   = ar rcs $1 $2
  make_dll   = $(error .so не собирается: см. exclusion)
  build_tool = $(CC_gcc) -std=c17 -O2 -Wall -Wextra -o $2 $1

endif

# DLL модуля строится всегда, когда она допустима (решение 5): MSVC и /MD.
DLL_ENABLED := $(and $(filter msvc,$(TOOLCHAIN)),$(filter dynamic,$(RUNTIME)))

# Проверка окружения — только если цели нужен компилятор и это не make -n.
NO_TOOLCHAIN_GOALS := help print-variants matrix clean clean-all format-check format
NEEDS_TOOLCHAIN := $(filter-out $(NO_TOOLCHAIN_GOALS) matrix/%,$(or $(MAKECMDGOALS),all))
ifneq ($(and $(NEEDS_TOOLCHAIN),$(if $(DRY_RUN),,yes)),)
  ifeq ($(call which,$(COMPILER)),)
    $(error Не найден $(COMPILER). $(if $(filter msvc,$(TOOLCHAIN)),Запустите make из Developer-окружения (vcvars64.bat).,Установите компилятор или задайте GCC_SUFFIX.))
  endif
endif

# =============================================================================
#  Общие правила
# =============================================================================
SRC_EXT_c   := .c
SRC_EXT_cpp := .cpp

# Каталоги — order-only пререквизиты: появление файлов внутри не вызывает
# пересборку. MKDIR создаёт и промежуточные каталоги (mkdir -p; в cmd —
# по умолчанию). Цепочку «каталог <- родитель» шаблоном не выразить:
# make не применяет одно неявное правило дважды в одной цепочке.
%/:
	$(call MKDIR,$@)

# Компиляция: язык определяется расширением исходника. Путь объекта
# повторяет путь исходника внутри каталога варианта.
$(B)/%$(OBJ): %.c | $$(@D)/
	$(call compile,c,$<,$@)
$(B)/%$(OBJ): %.cpp | $$(@D)/
	$(call compile,cpp,$<,$@)

# Инструмент приёмки: не зависит от ручек варианта.
RUN_CASE_EXE := $(TOOLS_DIR)/run_case$(EXE)
RUN_CASE = $(call RUN,$(RUN_CASE_EXE))
$(RUN_CASE_EXE): tools/run_case.c | $$(@D)/
	$(call build_tool,$<,$@)

# =============================================================================
#  Модуль common/ppm_io
# =============================================================================
# Ручной список зависимостей (решение 12), сверен с выводом findstr #include.
# Грубо: каждый объект модуля зависит от всех заголовков модуля.
# На GCC его дополняют точные списки из -MMD.
COMMON_SRCS_c   := ppm_io strerror
COMMON_SRCS_cpp := ppm_io
COMMON_HDRS_c   := common/c/api.h common/c/ppm_io.h common/c/ppm_io_alloc.h common/c/strerror.h common/c/luma.h
COMMON_HDRS_cpp := common/cpp/ppm_io.hpp common/cpp/luma.hpp
# Общие заголовки, которые включают задачи (но не сам модуль).
SHARED_HDRS_c   := common/c/version.h common/c/exit_codes.h
SHARED_HDRS_cpp := common/cpp/version.hpp common/cpp/exit_codes.hpp

# $1 = c | cpp
define MODULE_template
LIB_OBJS_$1   := $(patsubst %,$(B)/common/$1/%$(OBJ),$(COMMON_SRCS_$1))
DLL_OBJS_$1   := $(patsubst %,$(B)/common/$1/dll/%$(OBJ),$(COMMON_SRCS_$1))
STATIC_LIB_$1 := $(B)/common/$1/$(STATIC_LIB_NAME)
DLL_$1        := $(B)/common/$1/dll/ppm_io$(DLL)
IMPLIB_$1     := $(B)/common/$1/dll/ppm_io$(IMPLIB)
ALL_OBJS += $(patsubst %,$(B)/common/$1/%$(OBJ),$(COMMON_SRCS_$1))

$(patsubst %,$(B)/common/$1/%$(OBJ),$(COMMON_SRCS_$1)) $(patsubst %,$(B)/common/$1/dll/%$(OBJ),$(COMMON_SRCS_$1)): $(COMMON_HDRS_$1)

# Объекты DLL — отдельный каталог: другой макрос экспорта (решение «LINK —
# ось, .lib и .dll — узлы»). private: макрос не наследуется пререквизитами.
$(B)/common/$1/dll/%$(OBJ): private EXTRA_DEFS := -DKV_DYNAMIC_LINK
$(B)/common/$1/dll/%$(OBJ): common/$1/%$(SRC_EXT_$1) | $$$$(@D)/
	$$(call compile,$1,$$<,$$@)

# Старый архив удаляется: иначе ar оставил бы в нём удалённые объекты.
$(B)/common/$1/$(STATIC_LIB_NAME): $(patsubst %,$(B)/common/$1/%$(OBJ),$(COMMON_SRCS_$1))
	$$(call RMFILE,$$@)
	$$(call make_lib,$$@,$$^)

$(if $(DLL_ENABLED),$(call DLL_RULE,$1))
endef

# Одна команда link даёт и .dll, и import .lib — группированная цель.
# Правило есть только там, где DLL допустима (на gcc .so отложен).
define DLL_RULE
$(B)/common/$1/dll/ppm_io$(DLL) $(B)/common/$1/dll/ppm_io$(IMPLIB) &: $(patsubst %,$(B)/common/$1/dll/%$(OBJ),$(COMMON_SRCS_$1))
	$$(call make_dll,$1,$(B)/common/$1/dll/ppm_io$(DLL),$(B)/common/$1/dll/ppm_io$(IMPLIB),$$^)
endef
$(foreach L,c cpp,$(eval $(call MODULE_template,$L)))

# Что получает потребитель модуля. Вид зависимости:
#   none     — модуль не нужен;
#   strerror — только strerror (C-паспорт);
#   ppm_io   — весь модуль;
#   static   — весь модуль и всегда статически (тест самого модуля).
# $1 = язык, $2 = вид.
module_objs   = $(if $(filter strerror,$2),$(B)/common/$1/strerror$(OBJ),$(LIB_OBJS_$1))
module_inputs = $(if $(filter none,$2),,$(if $(filter obj,$(LINK)),$(call module_objs,$1,$2),$(if $(and $(filter dll,$(LINK)),$(filter-out static,$2)),$(IMPLIB_$1),$(STATIC_LIB_$1))))
uses_dll      = $(and $(filter dll,$(LINK)),$(filter strerror ppm_io,$1))

# Заголовки, от которых зависят объекты каталога $1 (язык $2, вид
# зависимости $3): свои заголовки каталога (перечислением каталога — новый
# заголовок задачи учитывается сам) + общие по виду зависимости.
# luma — header-only, его может включить и тот, кто модуль не линкует.
MODULE_HDRS_none_c       := common/c/luma.h
MODULE_HDRS_none_cpp     := common/cpp/luma.hpp
MODULE_HDRS_strerror_c   := common/c/strerror.h common/c/api.h
MODULE_HDRS_ppm_io_c     := $(COMMON_HDRS_c)
MODULE_HDRS_ppm_io_cpp   := $(COMMON_HDRS_cpp)
MODULE_HDRS_static_c     := $(COMMON_HDRS_c)
MODULE_HDRS_static_cpp   := $(COMMON_HDRS_cpp)
hdrs_of = $(wildcard $1/*.h $1/*.hpp) $(SHARED_HDRS_$2) $(MODULE_HDRS_$3_$2)

# =============================================================================
#  Исполняемые файлы
# =============================================================================
# $1 каталог исходников, $2 язык, $3 имя exe, $4 исходники без расширения,
# $5 вид зависимости от модуля.
define EXE_template
$(B)/$1/$3$(EXE): $(patsubst %,$(B)/$1/%$(OBJ),$4) $(call module_inputs,$2,$5) | $(B)/$1/
	$$(call link_exe,$2,$$@,$$^)
$(patsubst %,$(B)/$1/%$(OBJ),$4): $(call hdrs_of,$1,$2,$5)
ALL_EXES += $(B)/$1/$3$(EXE)
ALL_OBJS += $(patsubst %,$(B)/$1/%$(OBJ),$4)
$(if $(call uses_dll,$5),DLL_CONSUMERS += $(B)/$1/$3$(EXE))
endef

# Тест модуля линкуется статически всегда.
$(eval $(call EXE_template,common/c,c,ppm_io_test,ppm_io_test,static))
$(eval $(call EXE_template,common/cpp,cpp,ppm_io_test,ppm_io_test,static))

T1 := 01-image-gen
T2 := 02-image-passport
T3 := 03-image-stats
T4 := 04-image-filter

# Задача 1. Тест ppm_test модуль не использует.
$(eval $(call EXE_template,$(T1)/c,c,gen_image,main parse_args hsv_to_rgb patterns,ppm_io))
$(eval $(call EXE_template,$(T1)/c,c,ppm_test,ppm_test parse_args hsv_to_rgb patterns,none))
$(eval $(call EXE_template,$(T1)/cpp,cpp,gen_image,main parse_args hsv_to_rgb patterns,ppm_io))
$(eval $(call EXE_template,$(T1)/cpp,cpp,ppm_test,ppm_test parse_args hsv_to_rgb patterns,none))

# Задача 2. C-версия использует только strerror, C++-версия — ничего.
$(eval $(call EXE_template,$(T2)/c,c,passport,read_passport_main read_passport pixel_word,strerror))
$(eval $(call EXE_template,$(T2)/c,c,passport_tests,read_passport_test read_passport pixel_word,strerror))
$(eval $(call EXE_template,$(T2)/cpp,cpp,passport,read_passport_main read_passport pixel_word,none))
$(eval $(call EXE_template,$(T2)/cpp,cpp,passport_tests,read_passport_test read_passport pixel_word,none))

# Задача 3.
$(eval $(call EXE_template,$(T3)/c,c,image_stats,main ppm_stats,ppm_io))
$(eval $(call EXE_template,$(T3)/c,c,ppm_stats_test,ppm_stats_test ppm_stats,ppm_io))
$(eval $(call EXE_template,$(T3)/cpp,cpp,image_stats,image_stats ppm_stats,ppm_io))
$(eval $(call EXE_template,$(T3)/cpp,cpp,ppm_stats_test,ppm_stats_test ppm_stats,ppm_io))

# Задача 4.
$(eval $(call EXE_template,$(T4)/c,c,filter,main filter,ppm_io))
$(eval $(call EXE_template,$(T4)/c,c,filter_tests,filter_test filter,ppm_io))
$(eval $(call EXE_template,$(T4)/cpp,cpp,filter,filter_main filter,ppm_io))
$(eval $(call EXE_template,$(T4)/cpp,cpp,filter_tests,filter_test filter,ppm_io))

# Эталонные программы: независимые реализации на C, по одному исходнику.
REFS_T1 := ref_gradient ref_checker ref_radial
REFS_T2 := ref_passport ref_passport_input
REFS_T3 := ref_stats
REFS_T4 := ref_gradient_3_3_grayscale ref_gradient_3_3_threshold_128 ref_radial_3_3_grayscale \
           ref_probe_2_2 ref_probe_2_2_grayscale ref_probe_2_2_threshold_100 \
           ref_probe_3_3 ref_probe_3_3_grayscale ref_probe_3_3_threshold_100
$(foreach t,T1 T2 T3 T4,$(foreach r,$(REFS_$t),$(eval $(call EXE_template,$($t)/ref,c,$r,$r,none))))

# Короткие имена программ для приёмки.
GEN_IMAGE_c   := $(B)/$(T1)/c/gen_image$(EXE)
GEN_IMAGE_cpp := $(B)/$(T1)/cpp/gen_image$(EXE)
PASSPORT_c    := $(B)/$(T2)/c/passport$(EXE)
PASSPORT_cpp  := $(B)/$(T2)/cpp/passport$(EXE)
STATS_c       := $(B)/$(T3)/c/image_stats$(EXE)
STATS_cpp     := $(B)/$(T3)/cpp/image_stats$(EXE)
FILTER_c      := $(B)/$(T4)/c/filter$(EXE)
FILTER_cpp    := $(B)/$(T4)/cpp/filter$(EXE)
ref_exe = $(B)/$1/ref/$2$(EXE)

# DLL нужна потребителю во время запуска, а не линковки: копия рядом с exe —
# узел графа, пререквизит каждого запуска этого exe.
ifeq ($(LINK),dll)
define DLLCOPY_template
$1ppm_io$(DLL): $(DLL_$2) | $1
	$$(call CP,$$<,$$@)
endef
$(foreach L,c cpp,$(foreach d,$(sort $(dir $(filter $(B)/%/$L/,$(dir $(DLL_CONSUMERS))))),$(eval $(call DLLCOPY_template,$d,$L))))
endif
runtime_deps = $(if $(filter $1,$(DLL_CONSUMERS)),$(dir $1)ppm_io$(DLL))

# =============================================================================
#  Юнит-тесты: маркер X.passed рядом с X. Успех — код 0; строку
#  «All tests PASSED» гарантирует сам тест (решение 13).
# =============================================================================
define UNIT_template
$(patsubst %$(EXE),%,$1).passed: $1 $(call runtime_deps,$1) | $(dir $1)
	$$(call RMFILE,$$@)
	$$(call RUN,$$<)
	$$(call TOUCH,$$@)
UNIT_MARKERS += $(patsubst %$(EXE),%,$1).passed
endef
UNIT_TESTS := \
  $(B)/common/c/ppm_io_test$(EXE) $(B)/common/cpp/ppm_io_test$(EXE) \
  $(foreach L,c cpp,$(B)/$(T1)/$L/ppm_test$(EXE) $(B)/$(T2)/$L/passport_tests$(EXE) \
                    $(B)/$(T3)/$L/ppm_stats_test$(EXE) $(B)/$(T4)/$L/filter_tests$(EXE))
$(foreach t,$(UNIT_TESTS),$(eval $(call UNIT_template,$t)))

# =============================================================================
#  Приёмка: данные тестов — узлы графа.
#    вход/эталон  <- генератор;
#    фактический  <- (программа, вход);
#    вердикт .ok  <- (фактический, эталон) или (программа, ожидаемый код).
#  Вердикт сначала удаляется: упавшая проверка не оставит старый .ok
#  (.DELETE_ON_ERROR удаляет цель, только если рецепт успел её изменить).
#  Всё порождённое лежит в каталоге варианта; общие только фикстуры из ref/.
# =============================================================================
# Пустое поле в таблицах кейсов — «-»; «+» в аргументах — пробел.
field = $(if $(filter -,$1),,$(subst +, ,$1))

# Запуск программы с выводом в файл. $1 выход, $2 exe, $3 аргументы, $4 stdin.
define GEN_template
$1: $2 $4 $$(RUN_CASE_EXE) $(call runtime_deps,$2) | $(dir $1)
	$$(RUN_CASE) $(if $4,--in $4) --out $$@ -- $2 $3
endef

# Побайтовое сравнение. $1 вердикт, $2 фактический, $3 эталон.
define CMP_template
$1: $2 $3 | $(dir $1)
	$$(call RMFILE,$$@)
	$$(call CMP,$2,$3)
	$$(call TOUCH,$$@)
ACCEPT_OKS += $1
endef

# Генерация + сравнение: вердикт — <выход>.ok.
define GENCMP_template
$(call GEN_template,$1,$2,$3,$4)
$(call CMP_template,$1.ok,$1,$5)
endef

# Только код возврата (stdout отбрасывается, stderr — в лог для человека).
# $1 вердикт, $2 exe, $3 ожидаемый код, $4 аргументы, $5 stdin.
define EXPECT_template
$1: $2 $5 $$(RUN_CASE_EXE) $(call runtime_deps,$2) | $(dir $1)
	$$(call RMFILE,$$@)
	$$(RUN_CASE) $(if $5,--in $5) --expect $3 -- $2 $4
	$$(call TOUCH,$$@)
ACCEPT_OKS += $1
endef

REF := $(TD)/ref

# Ошибочные входы, которые нельзя получить генератором, — фикстуры в
# репозитории (исходный файл — тоже узел). Пустой ввод — stdin по умолчанию.
P5_FIXTURE := $(T3)/ref/fixtures/p5.txt

# --- Эталоны (одни на вариант, не зависят от языка проверяемой программы) ---
PATTERNS := gradient checker radial
$(foreach p,$(PATTERNS),$(eval $(call GEN_template,$(REF)/$(p)_3x3.ppm,$(call ref_exe,$(T1),ref_$(p)),,)))

# Задача 4: <эталонная программа>:<файл>.
REF_FILES_T4 := \
  ref_gradient_3_3_grayscale:gradient_3x3_grayscale \
  ref_gradient_3_3_threshold_128:gradient_3x3_threshold_128 \
  ref_radial_3_3_grayscale:radial_3x3_grayscale \
  ref_probe_2_2:probe_2x2 \
  ref_probe_2_2_grayscale:probe_2x2_grayscale \
  ref_probe_2_2_threshold_100:probe_2x2_threshold_100 \
  ref_probe_3_3:probe_3x3 \
  ref_probe_3_3_grayscale:probe_3x3_grayscale \
  ref_probe_3_3_threshold_100:probe_3x3_threshold_100
$(foreach x,$(REF_FILES_T4),$(eval $(call GEN_template,$(REF)/$(word 2,$(subst :, ,$x)).ppm,$(call ref_exe,$(T4),$(word 1,$(subst :, ,$x))),,)))

# Задача 3: эталон статистики = ref_stats на эталонном изображении.
$(foreach p,$(PATTERNS),$(eval $(call GEN_template,$(REF)/stats_$(p)_3x3.txt,$(call ref_exe,$(T3),ref_stats),,$(REF)/$(p)_3x3.ppm)))

# Задача 2: <кейс>:<ожидаемый код>. Код 0 — сравнение с ref_passport.
# ДОПУЩЕНИЕ: «plural_2–101–111» из README прочитано как три кейса;
# zero считается успешным (README не относит 0 к ошибкам). Сверить с
# 02-image-passport/ref/ref_passport_input.c.
PASSPORT_CASES := basic:0 single_1:0 plural_2:0 plural_101:0 plural_111:0 zero:65 \
                  empty_name:65 no_input:66 bad_count:65 negative:65
pp_case = $(word 1,$(subst :, ,$1))
pp_code = $(word 2,$(subst :, ,$1))
$(foreach x,$(PASSPORT_CASES),$(eval $(call GEN_template,$(REF)/passport_$(call pp_case,$x)_in.txt,$(call ref_exe,$(T2),ref_passport_input),$(call pp_case,$x),)))
$(foreach x,$(filter %:0,$(PASSPORT_CASES)),$(eval $(call GEN_template,$(REF)/passport_$(call pp_case,$x)_expected.txt,$(call ref_exe,$(T2),ref_passport),$(call pp_case,$x),)))

# --- Кейсы ошибок: <имя>:<код>:<аргументы>:<stdin> ---
# Задача 1 — по спецификации README (1..512, известные паттерны).
GEN_ERRORS := noargs:64:-:- abc:64:abc:- zero:64:0:- too_big:64:513:- \
              bad_pattern:64:3+bogus:- help:0:--help:- version:0:--version:-
# Задача 3.
STATS_ERRORS := empty_input:66:-:- bad_format:65:-:$(P5_FIXTURE)
# Задача 4.
FILTER_ERRORS := noargs:64:-:- unknown_mode:64:--bogus:- threshold_no_t:64:--threshold:- \
                 threshold_256:64:--threshold+256:- threshold_nan:64:--threshold+abc:- \
                 help:0:--help:- version:0:--version:- \
                 empty_input:66:--grayscale:- bad_format:65:--grayscale:$(P5_FIXTURE)
# Задача 4, успешные: <эталон>:<вход>:<аргументы>. Вход один для C и C++.
FILTER_CASES := \
  gradient_3x3_grayscale:gradient_3x3:--grayscale \
  gradient_3x3_threshold_128:gradient_3x3:--threshold+128 \
  radial_3x3_grayscale:radial_3x3:--grayscale \
  probe_2x2_grayscale:probe_2x2:--grayscale \
  probe_2x2_threshold_100:probe_2x2:--threshold+100 \
  probe_3x3_grayscale:probe_3x3:--grayscale \
  probe_3x3_threshold_100:probe_3x3:--threshold+100

# $1 = префикс вердикта, $2 = exe, $3 = строка таблицы ошибок.
expect_case = $(eval $(call EXPECT_template,$1_$(word 1,$(subst :, ,$3)).ok,$2,$(word 2,$(subst :, ,$3)),$(call field,$(word 3,$(subst :, ,$3))),$(call field,$(word 4,$(subst :, ,$3)))))

# --- Прогоны по языкам ---
define ACCEPT_LANG
# Задача 1: генератор против эталонов.
$(foreach p,$(PATTERNS),$(eval $(call GENCMP_template,$(TD)/$1/gen_$(p)_3.ppm,$(GEN_IMAGE_$1),3 $(p),,$(REF)/$(p)_3x3.ppm)))
$(foreach x,$(GEN_ERRORS),$(call expect_case,$(TD)/$1/gen,$(GEN_IMAGE_$1),$x))

# Задача 2: вход пишет ref_passport_input (не зависит от echo и кодировки).
$(foreach x,$(filter %:0,$(PASSPORT_CASES)),$(eval $(call GENCMP_template,$(TD)/$1/passport_$(call pp_case,$x).txt,$(PASSPORT_$1),,$(REF)/passport_$(call pp_case,$x)_in.txt,$(REF)/passport_$(call pp_case,$x)_expected.txt)))
$(foreach x,$(filter-out %:0,$(PASSPORT_CASES)),$(eval $(call EXPECT_template,$(TD)/$1/passport_$(call pp_case,$x).ok,$(PASSPORT_$1),$(call pp_code,$x),,$(REF)/passport_$(call pp_case,$x)_in.txt)))

# Задача 3: конвейер gen_image | image_stats — промежуточный файл стал узлом
# (код генератора больше не маскируется конвейером).
$(foreach p,$(PATTERNS),$(eval $(call GENCMP_template,$(TD)/$1/stats_$(p)_3x3.txt,$(STATS_$1),,$(TD)/$1/gen_$(p)_3.ppm,$(REF)/stats_$(p)_3x3.txt)))
$(foreach x,$(STATS_ERRORS),$(call expect_case,$(TD)/$1/stats,$(STATS_$1),$x))

# Задача 4: фильтр на общих эталонных входах.
$(foreach x,$(FILTER_CASES),$(eval $(call GENCMP_template,$(TD)/$1/filter_$(word 1,$(subst :, ,$x)).ppm,$(FILTER_$1),$(call field,$(word 3,$(subst :, ,$x))),$(REF)/$(word 2,$(subst :, ,$x)).ppm,$(REF)/$(word 1,$(subst :, ,$x)).ppm)))
$(foreach x,$(FILTER_ERRORS),$(call expect_case,$(TD)/$1/filter,$(FILTER_$1),$x))
endef
$(foreach L,c cpp,$(eval $(call ACCEPT_LANG,$L)))

# =============================================================================
#  Фазовые цели
# =============================================================================
.PHONY: all lib exes tools unit acceptance test clean clean-all help \
        print-variants matrix format-check format coverage
.DEFAULT_GOAL := all

all: lib exes
lib: $(STATIC_LIB_c) $(STATIC_LIB_cpp) $(if $(DLL_ENABLED),$(DLL_c) $(DLL_cpp))
exes: $(ALL_EXES)
tools: $(RUN_CASE_EXE)
unit: $(UNIT_MARKERS)
acceptance: $(ACCEPT_OKS)
test: unit acceptance

# Удаляет только текущий вариант; clean-all — весь build/.
clean:
	$(call RMTREE,$(B))
clean-all:
	$(call RMTREE,build)

coverage:
	$(error coverage — следующий заход: COVERAGE=1 уже задаёт флаги и ограничения варианта)

# =============================================================================
#  Матрица вариантов (решение «б»: Makefile — источник допустимых сочетаний)
# =============================================================================
CONFIG_VALUES       := debug release
RUNTIME_VALUES_msvc := dynamic static
RUNTIME_VALUES_gcc  := dynamic static full
LINK_VALUES         := obj lib dll
SANITIZE_VALUES     := address none
# Кодировка варианта в имени цели: config+runtime+link+sanitize.
ALL_VARIANTS := $(foreach c,$(CONFIG_VALUES),$(foreach r,$(RUNTIME_VALUES_$(TOOLCHAIN)),$(foreach l,$(LINK_VALUES),$(foreach s,$(SANITIZE_VALUES),$c+$r+$l+$s))))
v_word   = $(word $2,$(subst +, ,$1))
v_args   = CONFIG=$(call v_word,$1,1) RUNTIME=$(call v_word,$1,2) LINK=$(call v_word,$1,3) SANITIZE=$(call v_word,$1,4)
v_reason = $(call exclusion,$(TOOLCHAIN),$(call v_word,$1,1),$(call v_word,$1,2),$(call v_word,$1,3),$(call v_word,$1,4),0)
OK_VARIANTS := $(foreach v,$(ALL_VARIANTS),$(if $(call v_reason,$v),,$v))

print-variants:
	$(foreach v,$(ALL_VARIANTS),$(info $(if $(call v_reason,$v),# исключён: $(call v_args,$v) -- $(call v_reason,$v),$(call v_args,$v))))
	$(info # $(TOOLCHAIN): допустимо $(words $(OK_VARIANTS)) из $(words $(ALL_VARIANTS)))

# Каждый вариант — отдельный вызов make со своим графом; варианты не
# пересекаются по каталогам, поэтому make -j matrix безопасен.
MATRIX_GOAL ?= test
matrix: $(addprefix matrix/,$(OK_VARIANTS))
.PHONY: $(addprefix matrix/,$(OK_VARIANTS))
$(addprefix matrix/,$(OK_VARIANTS)): matrix/%:
	$(MAKE) --no-print-directory TOOLCHAIN=$(TOOLCHAIN) $(call v_args,$*) $(MATRIX_GOAL)

# =============================================================================
#  Форматирование. Проверка — узлы графа вне каталога варианта: её входы
#  (файл, .clang-format, версия инструмента) от ручек не зависят.
#  format мутирует исходники — .PHONY и никогда не пререквизит сборки.
# =============================================================================
FORMAT_SRCS := $(wildcard common/c/*.c common/c/*.h common/cpp/*.cpp common/cpp/*.hpp tools/*.c \
                          0*/c/*.c 0*/c/*.h 0*/cpp/*.cpp 0*/cpp/*.hpp 0*/cpp/*.h 0*/ref/*.c 0*/ref/*.h)
FORMAT_OKS  := $(patsubst %,build/format/%.ok,$(FORMAT_SRCS))

# Версия проверяется, только когда запрошены цели форматирования.
ifneq ($(filter format format-check,$(MAKECMDGOALS)),)
  ifneq ($(CLANG_FORMAT_VERSION),)
    ifeq ($(findstring version $(CLANG_FORMAT_VERSION),$(shell $(CLANG_FORMAT) --version)),)
      $(error Нужен clang-format $(CLANG_FORMAT_VERSION), найден: $(shell $(CLANG_FORMAT) --version))
    endif
  endif
endif

format-check: $(FORMAT_OKS)
build/format/%.ok: % .clang-format | $$(@D)/
	$(CLANG_FORMAT) --dry-run --Werror $<
	$(call TOUCH,$@)

format:
	$(CLANG_FORMAT) -i $(FORMAT_SRCS)

# =============================================================================
help:
	$(info Ручки (текущие значения):)
	$(info $(empty)  PLATFORM=$(PLATFORM)  TOOLCHAIN=$(TOOLCHAIN)  CONFIG=$(CONFIG)  RUNTIME=$(RUNTIME))
	$(info $(empty)  LINK=$(LINK)  SANITIZE=$(SANITIZE)  COVERAGE=$(COVERAGE)  GCC_SUFFIX=$(GCC_SUFFIX))
	$(info Каталог варианта: $(B))
	$(info Цели: all lib exes tools unit acceptance test clean clean-all)
	$(info $(empty)      print-variants matrix [MATRIX_GOAL=test] format-check format)

# Точные header-зависимости от gcc -MMD (на MSVC файлов .d нет).
-include $(patsubst %$(OBJ),%.d,$(ALL_OBJS) $(DLL_OBJS_c) $(DLL_OBJS_cpp))
