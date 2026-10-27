#!/bin/sh
# Run from the vanilla OpenWrt checkout after feeds update/install.
set -eu
root="$(pwd)"
test -f include/toplevel.mk
test -d feeds/packages/.git
for patch in "$root"/h5-patches/packages/*.patch; do
	if git -C feeds/packages apply --check "$patch"; then
		git -C feeds/packages apply "$patch"
	elif git -C feeds/packages apply --reverse --check "$patch"; then
		printf 'Already applied: %s\n' "$(basename "$patch")"
	else
		printf 'Cannot apply feed patch: %s\n' "$patch" >&2
		exit 1
	fi
done
