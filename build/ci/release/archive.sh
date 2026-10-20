#!/bin/bash
# artifacts/<name>/ -> release/*.zip.
set -euo pipefail
out="$PWD/release"
mkdir -p "$out" symbols
for runtime in steamrt3 steamrt4 windows; do
	(cd "artifacts/$ARTIFACT-$runtime" && zip -r "$out/$ARTIFACT-$TAG-$runtime.zip" .)
done
mv artifacts/"$ARTIFACT"-*-symbols symbols/
(cd symbols && zip -r "$out/$ARTIFACT-$TAG-symbols.zip" .)
for name in $EXTRA; do
	(cd "artifacts/$name" && zip -r "$out/$name-$TAG.zip" .)
done
