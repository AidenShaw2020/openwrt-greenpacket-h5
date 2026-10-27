# Licensing

This is a mixed-license OpenWrt overlay, not a single-license firmware bundle.

| Material | License |
| --- | --- |
| Original otherwise unmarked overlay code and documentation | GPL-2.0-only |
| Files carrying an SPDX identifier | The identifier in that file |
| H5 device-tree files | BSD-3-Clause, as marked in each file |
| Linux/OpenWrt/feed changes | License of the affected upstream files |
| tqftpserv upstream source | BSD-3-Clause, as declared by its package recipe |
| Third-party packages downloaded during a build | Their own upstream licenses |

The full GPL-2.0 text is in ../LICENSE. The BSD-3-Clause reference text is in
BSD-3-Clause.txt. Existing copyright and SPDX notices are retained.
Makefile `PKG_LICENSE` fields describe the associated packages; a build-recipe
header can itself be GPL while the fetched program has a different license.

Kernel patches remain subject to the Linux kernel's licensing rules. For
complete dependency terms, inspect the pinned OpenWrt/feed source, each
package's `PKG_LICENSE`/`PKG_LICENSE_FILES`, and the original source archives.
The absence of a license notice in a binary image is not permission to
redistribute it without satisfying all applicable source and notice duties.

This project distributes only its source overlay, patches and documentation,
not compiled images or bundled dependency archives. See ../docs/SOURCES.md.
No device-specific proprietary modem firmware is distributed. Anyone building
and redistributing binaries must preserve the applicable source, notice and
license obligations. Merely linking this overlay is not a substitute for
corresponding source for the complete firmware. No trademark rights or vendor
endorsement are granted.