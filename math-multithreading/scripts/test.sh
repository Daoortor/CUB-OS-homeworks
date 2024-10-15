#!/usr/bin/bash

clang -fsanitize=address -Wall -Wextra -Werror ../dpnum.c -o dpnum
rm out.txt

MAX_THREADS=16
ITER_COUNT=20
RANGE_END=1000000

for t in $(seq 1 $MAX_THREADS); do
  RESULT_LIST=
  for _ in $(seq 1 $ITER_COUNT); do
    RESULT=$( { command time -f"%e" ./dpnum -e "$RANGE_END" -t "$t" > "/dev/null"; } 2>&1 )
    RESULT_LIST="$RESULT_LIST $RESULT"
  done
  echo "$t thread(s), $ITER_COUNT iterations."
  { echo $RESULT_LIST | awk '{s+=$1}END{print s/NR}' RS=" "; } >> out.txt
  echo $RESULT_LIST | awk '{s+=$1}END{print "average execution real time:",s/NR,"seconds"}' RS=" "
done

rm dpnum
