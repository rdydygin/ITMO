#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT/SDK_Snake"
PLUGINS="${CUBEIDE_PLUGINS:-/Applications/STM32CubeIDE.app/Contents/Eclipse/plugins}"
fail() { printf '%s\n' "$*" >&2; exit 1; }
find_tool() {
    local name="$1" candidate
    if command -v "$name" >/dev/null 2>&1; then command -v "$name"; return; fi
    for candidate in "$PLUGINS"/*/tools/bin/"$name"; do
        if [[ -x "$candidate" ]]; then printf '%s\n' "$candidate"; return; fi
    done
    return 1
}
setup_build() {
    local gcc_path
    gcc_path="$(find_tool arm-none-eabi-gcc)" || fail 'Не найден arm-none-eabi-gcc. Установите ARM GNU Toolchain и добавьте tools/bin в PATH.'
    export PATH="$(dirname "$gcc_path"):$PATH"
    MAKE_BIN="$(find_tool make)" || fail 'Не найден make.'
}
build() {
    setup_build
    "$MAKE_BIN" TOOLCHAIN="$(dirname "$(find_tool arm-none-eabi-gcc)")/arm-none-eabi-" -j "${JOBS:-4}" all
}
setup_openocd() {
    OPENOCD_BIN="${OPENOCD_BIN:-$(find_tool openocd || true)}"
    [[ -x "$OPENOCD_BIN" ]] || fail 'Не найден OpenOCD. Установите его или задайте OPENOCD_BIN.'
    if [[ -z "${OPENOCD_SCRIPTS:-}" ]]; then
        local candidate
        for candidate in "$(dirname "$OPENOCD_BIN")/../share/openocd/scripts" "$PLUGINS"/*/resources/openocd/st_scripts /opt/homebrew/share/openocd/scripts /usr/local/share/openocd/scripts; do
            if [[ -f "$candidate/target/stm32f4x.cfg" ]]; then OPENOCD_SCRIPTS="$candidate"; break; fi
        done
    fi
    [[ -f "${OPENOCD_SCRIPTS:-}/target/stm32f4x.cfg" ]] || fail 'Не найдены конфигурации OpenOCD. Задайте OPENOCD_SCRIPTS — каталог с target/stm32f4x.cfg.'
}
case "${1:-help}" in
    build) build ;;
    flash) build; setup_openocd; "$OPENOCD_BIN" -s "$OPENOCD_SCRIPTS" -f SDK11M.cfg -c 'program build/SDK_Snake.elf verify reset exit' ;;
    debug) build; setup_openocd; "$OPENOCD_BIN" -s "$OPENOCD_SCRIPTS" -f SDK11M.cfg -c 'init; reset halt' ;;
    gdb) setup_build; GDB_BIN="$(find_tool arm-none-eabi-gdb)" || fail 'Не найден arm-none-eabi-gdb.'; "$GDB_BIN" build/SDK_Snake.elf -ex 'target extended-remote localhost:3333' ;;
    clean) setup_build; "$MAKE_BIN" clean ;;
    help|-h|--help) printf '%s\n' './lab.sh build — собрать' './lab.sh flash — собрать, прошить, проверить и запустить' './lab.sh clean — удалить результаты сборки' './lab.sh debug — запустить сервер отладки (процессор остановлен)' './lab.sh gdb — подключиться к серверу из второго терминала' ;;
    *) fail "Неизвестная команда: $1. Используйте ./lab.sh help." ;;
esac
