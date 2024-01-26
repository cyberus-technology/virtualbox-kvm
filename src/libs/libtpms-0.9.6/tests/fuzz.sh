#!/usr/bin/env bash

TESTDIR=${abs_top_testdir:-$(dirname "$0")}
DIR=${PWD}

MAXLINES=128
l=1

corpus=$(ls "$TESTDIR/corpus-execute-command/"*)

while :; do
  echo "Passing test cases $l to $((l + MAXLINES))"
  tmp=$(echo "${corpus}" | sed -n "${l},$((l + MAXLINES))p")
  [ -z "${tmp}" ] && exit 0
  ${DIR}/fuzz ${tmp}
  rc=$?
  [ $rc -ne 0 ] && exit $rc
  l=$((l + MAXLINES))
done
