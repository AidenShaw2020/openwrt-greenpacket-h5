#!/bin/sh
# Run from the vanilla OpenWrt checkout after feeds update/install.
set -eu
root="$(pwd)"
test -f include/toplevel.mk

# h5-patches/<feed>/*.patch apply to feeds/<feed>.
for feed in packages luci; do
	test -d "feeds/$feed/.git"
	for patch in "$root"/h5-patches/"$feed"/*.patch; do
		[ -e "$patch" ] || continue
		if git -C "feeds/$feed" apply --check "$patch"; then
			git -C "feeds/$feed" apply "$patch"
		elif git -C "feeds/$feed" apply --reverse --check "$patch"; then
			printf 'Already applied: %s\n' "$(basename "$patch")"
		else
			printf 'Cannot apply feed patch: %s\n' "$patch" >&2
			exit 1
		fi
	done
done
