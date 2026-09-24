#!/usr/bin/env bash
# Export the verified package inputs from an ISO build; never copy host repositories.
set -Eeuo pipefail
umask 077

if [[ ${1:-} == --help ]]; then
    echo "Usage: $0 [BUILD_DIRECTORY] [OUTPUT.tar.gz]"
    echo 'Defaults: last ISO build under ~/iso/xiso and ./offline-iso-packages-complete.tar.gz'
    exit 0
fi
build_dir=${1:-}
if [[ -z $build_dir ]]; then
    [[ -r $HOME/iso/xiso/last-build.txt ]] || { echo 'No recorded ISO build. Run the online creator first.' >&2; exit 1; }
    IFS= read -r build_dir < "$HOME/iso/xiso/last-build.txt"
fi
output=${2:-$PWD/offline-iso-packages-complete.tar.gz}
[[ ! -e $output ]] || { echo "Refusing to overwrite $output" >&2; exit 1; }
[[ -s $build_dir/package-files.txt && -s $build_dir/profile/packages.x86_64 ]] || {
    echo 'The build has not completed package resolution.' >&2; exit 1;
}
output_dir=$(dirname -- "$output")
mkdir -p "$output_dir"
output_dir=$(realpath -- "$output_dir")
output="$output_dir/$(basename -- "$output")"
staging=$(mktemp -d "$output_dir/.offline-package-XXXXXX")
trap 'rm -rf -- "$staging"' EXIT
mkdir -p "$staging/offline-packages/pkg" "$staging/offline-packages/sync"
for repo in core extra; do
    cp -- "$build_dir/db/sync/$repo.db" "$staging/offline-packages/sync/"
    if [[ -f $build_dir/db/sync/$repo.db.sig ]]; then cp -- "$build_dir/db/sync/$repo.db.sig" "$staging/offline-packages/sync/"; fi
done
while IFS= read -r file; do
    [[ $file != */* && $file == *.pkg.tar.* && -s $build_dir/pkg/$file ]] || {
        echo "Missing or invalid package: $file. Finish the online package download first." >&2; exit 1;
    }
    cp --reflink=auto -- "$build_dir/pkg/$file" "$staging/offline-packages/pkg/"
    if [[ -f $build_dir/pkg/$file.sig ]]; then cp -- "$build_dir/pkg/$file.sig" "$staging/offline-packages/pkg/"; fi
done < "$build_dir/package-files.txt"
cp -- "$build_dir/profile/packages.x86_64" "$build_dir/package-files.txt" "$staging/offline-packages/"
printf 'FORMAT=1\nARCH=x86_64\nREPOSITORIES=arch-core-extra\n' > "$staging/offline-packages/archive.meta"
tar -czf "$staging/archive.tar.gz" -C "$staging" offline-packages
# Hard-link publication fails rather than overwriting an archive created concurrently.
ln -- "$staging/archive.tar.gz" "$output"
echo "Offline package created: $output"
echo 'Select this archive in the ISO creator. Package completeness and signatures are checked before snapshotting.'
