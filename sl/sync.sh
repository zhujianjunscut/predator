#!/bin/sh
TIMEOUT="timeout 8"

ALL=data/test-00??.c
test -n "$1" && ALL="$*"
export ALL

echo "ATTENTION: This script is not intended to be run !!!"
echo "           Please do not commit the generated files to prevent" \
                 "a disaster..."

ticks(){
    while sleep .5; do
        printf .
    done
}

gen(){
    for i in $ALL; do
      printf -- "--- "
      printf "%s" "`basename $i .c` "
      test -n "$1" && printf "%s " "$1"
      $TIMEOUT ../gcc-install/bin/gcc \
        -m32 \
        -c $i \
        -o /dev/null \
        -DPREDATOR \
        -I../cl -I../cl/gcc -I../include -I../include/gcc \
        -fplugin=../sl_build/libsl.so $1 \
        2>&1 \
        | grep -v '\[internal location\]$' \
        | sed 's|data/||' \
        > data/`basename $i .c`.err$2
      printf "\n"
    done
}

ticks &
gen "-fplugin-arg-libsl-args=fast" .fast
gen
kill $!
