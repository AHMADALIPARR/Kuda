#!/bin/sh
# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# Policy audit: zero .py, zero YAML CI. Exits 1 on any violation.
viol=0
for pat in '*.py' '*.yml' '*.yaml' 'pytest' 'setup.py' 'tox.ini'; do
  hits=$(find . -name "$pat" -not -path './.git/*' 2>/dev/null)
  if [ -n "$hits" ]; then
    echo "VIOLATION: pattern $pat:"
    echo "$hits"
    viol=1
  fi
done
if [ "$viol" -eq 0 ]; then
  echo "AUDIT PASS: zero python, zero YAML CI artifacts"
fi
exit $viol
