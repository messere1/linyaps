#!/bin/bash
# SPDX-FileCopyrightText: 2026 messere1
# SPDX-License-Identifier: LGPL-3.0-or-later
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
helper=${STRIP_HELPER:-"$root/libexec/linglong/builder/helper/symbols-strip.sh"}
temp=$(mktemp -d)
trap 'rm -rf "$temp"' EXIT
mkdir -p "$temp/files/bin" "$temp/tools" "$temp/tmp"
printf 'int main(void) { return 0; }\n' > "$temp/main.c"
cc -g -Wl,--build-id "$temp/main.c" -o "$temp/files/bin/example"
cat > "$temp/tools/eu-strip" << 'EOF'
#!/bin/sh
if [ "$STRIP_TEST_MODE" = fail ]; then exit 73; fi
printf 'debug fixture\n' > "$3"
EOF
chmod +x "$temp/tools/eu-strip"
export PATH="$temp/tools:$PATH" PREFIX="$temp/files" TMPDIR="$temp/tmp"
export STRIP_TEST_MODE=fail
set +e
bash "$helper" > "$temp/failure.log" 2>&1
status=$?
set -e
if [ "$status" -ne 73 ]; then
    cat "$temp/failure.log" >&2
    echo "strip failure was hidden (exit=$status)" >&2
    exit 1
fi
if find "$temp/tmp" -type f | grep -q .; then
    echo "temporary file leaked after failure" >&2
    exit 1
fi
export STRIP_TEST_MODE=success
bash "$helper"
test -z "$(find "$temp/tmp" -type f -print)"
test -n "$(find "$temp/files/lib/debug/.build-id" -name '*.debug' -type f -print)"
echo 'strip failures propagate and temporary lists are cleaned: passed'
