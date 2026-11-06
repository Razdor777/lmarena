#!/usr/bin/env bash
# Все статические проверки порта 26.62 разом. Компилятора нет, поэтому это
# единственный способ поймать ошибки уровня «нет члена / нет тела / нет имени».
#
#   bash tools/check_all.sh          # проверить
#   bash tools/check_all.sh --fix    # заодно пересобрать отчёты в docs/
#
# Код возврата 0 — всё чисто.
set -u
cd "$(dirname "$0")/.." || exit 2
fail=0

run() {
    local name="$1"; shift
    printf '== %-24s ' "$name"
    if out=$("$@" 2>&1); then
        printf 'ok  %s\n' "$(printf '%s' "$out" | tail -1)"
    else
        printf 'FAIL\n%s\n' "$out"
        fail=1
    fi
}

run "includes"        python3 tools/check_includes.py 26.62/src
run "members"         python3 tools/check_members.py --vocab-exclude Features -- 26.62/src
run "undefined"       python3 tools/check_undefined.py 26.62/src
run "event-visibility" python3 tools/check_event_visibility.py 26.62/src
run "sig-names"       python3 tools/check_sig_names.py 26.62/src
run "method-case"     python3 tools/check_method_case.py 26.62/src
run "enums"           python3 tools/check_enums.py 26.62/src
run "duplicates"      python3 tools/check_duplicates.py 26.62/src
run "unity-collisions" python3 tools/check_unity_collisions.py 26.62/src
run "header-guards"    python3 tools/check_header_guards.py 26.62/src

if [ "${1:-}" = "--fix" ]; then
    printf '== %-24s ' "resources"
    python3 tools/gen_resources.py >/dev/null && echo "пересобраны"
    printf '== %-24s ' "sig-checklist"
    python3 tools/sig_checklist.py --out docs/SIGS_26.62_RU.md >/dev/null && echo "обновлён docs/SIGS_26.62_RU.md"
fi

[ $fail -eq 0 ] && echo "всё чисто" || echo "есть замечания"
exit $fail
