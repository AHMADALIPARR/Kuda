#!/bin/sh
# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
set -eu

NPROC=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
VARIANTS=${VARIANTS:-4}

make -j"$NPROC" all

pids=""
i=1
while [ "$i" -le "$VARIANTS" ]; do
    ( ./test_harness >"test_$i.log" 2>&1; echo $? >"test_$i.rc" ) &
    pids="$pids $!"
    i=$((i + 1))
done

for pid in $pids; do
    wait "$pid" || true
done

ok=0
bad=0
i=1
while [ "$i" -le "$VARIANTS" ]; do
    rc=$(cat "test_$i.rc")
    if [ "$rc" = "0" ]; then
        ok=$((ok + 1))
    else
        echo "variant $i exit=$rc" >&2
        cat "test_$i.log" >&2
        bad=$((bad + 1))
    fi
    i=$((i + 1))
done

echo "variants=$VARIANTS ok=$ok bad=$bad"
[ "$bad" = "0" ]
