#!/bin/sh
cd "$(dirname "$0")/.." || exit 1

PARENT=./build/parent
fail=0

check() {
	name=$1
	input=$2
	expected=$3
	actual=$(printf '%b' "$input" | $PARENT)
	if [ "$actual" = "$(printf '%b' "$expected")" ]; then
		echo "PASS: $name"
	else
		echo "FAIL: $name"
		echo "  expected: $(printf '%b' "$expected")"
		echo "  actual:   $actual"
		fail=1
	fi
}

check "простая строка"        'Hello World\n'                 'hello_world'
check "несколько строк"       'ONE two\nThree  Four\n'        'one_two\nthree__four'
check "табуляция и CR"          'A\tB\rC\n'                     'a_b_c'
check "пустая строка"         '\n'                            ''
check "без пробелов"          'ABCdef\n'                      'abcdef'
check "только пробелы"        '   \n'                         '___'
check "кириллица"             'Привет МИР Ёж\n'               'привет_мир_ёж'
check "без перевода строки"   'NO NEWLINE'                    'no_newline'
check "пустой ввод"           ''                              ''

long=$(head -c 1048576 /dev/zero | tr '\0' 'A')
res=$(printf '%s %s\n' "$long" "$long" | $PARENT)
exp="$(printf '%s' "$long" | tr 'A' 'a')_$(printf '%s' "$long" | tr 'A' 'a')"
if [ "$res" = "$exp" ]; then
	echo "PASS: длинная строка (2 МБ)"
else
	echo "FAIL: длинная строка (2 МБ)"
	fail=1
fi

cd /tmp || exit 1
res=$(printf 'Other Dir\n' | "$OLDPWD/build/parent")
cd "$OLDPWD" || exit 1
if [ "$res" = "other_dir" ]; then
	echo "PASS: запуск из другого каталога"
else
	echo "FAIL: запуск из другого каталога ($res)"
	fail=1
fi

exit $fail
