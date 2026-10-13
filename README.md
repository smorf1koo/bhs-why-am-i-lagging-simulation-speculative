# Simulation Engineer: «Спекулятивный тик»

Тестовое задание проекта **Why_Am_I_Lagging** (Bear Head Studio).

## О задании

Детерминированная симуляция, загрузчик трассы задержек, цикл её проигрывания и эталонные тесты готовы.
Вы реализуете снапшоты состояния и спекулятивный сервер: недостающие вводы заменяются предсказанием
«повтор последнего», при приходе настоящего ввода сервер подтверждает результат (commit) или
откатывается и пересчитывает (rollback).

Задание — в документе
[`Тестовое_задание_Simulation_Engineer.docx`](Тестовое_задание_Simulation_Engineer.docx).

## Состав пакета

| Файл / папка | Назначение |
|---|---|
| `Тестовое_задание_Simulation_Engineer.docx` | задание: задача, входные данные, что сделать, как сдать |
| `starter/` | C++20 / CMake. Готово: `game_state.hpp`, `simulation.cpp` (`step`, `hash`), `trace.*`, цикл проигрывания в `main.cpp`. TODO: `src/snapshot.cpp`, `speculative_server.{hpp,cpp}`, метрики в `main.cpp` (ошибки предсказания, доля тиков с совпавшим показанным состоянием, откаты), свои тесты в `tests/test_own.cpp` |
| `starter/tests/` | 11 эталонных тестов (детерминизм, snapshot / restore, подтверждённое и показываемое состояние против оракула) на самописном раннере |
| `data/input_trace.csv`, `data/FORMAT.md` | трасса: 4 игрока × 10 000 тиков, задержка каждого ввода; игрок 3 — отстающий |
| `data/generate_trace.py` | генератор трасс (stdlib): `--check`, `--seed`, `--ticks`, `--out` |
| `report_template.md` | шаблон `report.md` |

## Как начать

macOS / Linux, CMake:

```bash
python3 -I data/generate_trace.py --check   # трасса не повреждена
cd starter && cmake -B build && cmake --build build -j
./build/spec_tests   # в стартере: 4 PASS, 7 FAIL с сообщением TODO
./build/spec_run --trace ../data/input_trace.csv
```

Windows, Visual Studio (генератор CMake с несколькими конфигурациями, поэтому конфигурация указывается
при сборке, а программы лежат в `build\Release\`):

```bat
py -3 -I data\generate_trace.py --check
cd starter
cmake -B build
cmake --build build --config Release
build\Release\spec_tests.exe
build\Release\spec_run.exe --trace ..\data\input_trace.csv
```

Без CMake (clang++ или g++, из папки `starter/`):

```bash
c++ -std=c++20 -O2 -pthread -Iinclude src/simulation.cpp src/trace.cpp \
    src/snapshot.cpp src/speculative_server.cpp tests/*.cpp -o spec_tests
c++ -std=c++20 -O2 -pthread -Iinclude src/*.cpp -o spec_run
./spec_tests
./spec_run --trace ../data/input_trace.csv
```
