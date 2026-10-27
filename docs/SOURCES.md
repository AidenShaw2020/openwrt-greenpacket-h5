# Build Inputs

This project distributes the H5 source overlay and patches only. It does not
distribute prebuilt firmware or a bundled dependency-source archive. Follow
the pinned revisions and complete build commands in [README](../README.md#build).

The necessary project-specific inputs are already in Git:

- `target/linux/sdx/`: target, image recipes, kernel patches and CPE config seed.
- `package/`: custom packages and integration patches.
- `h5-patches/`: packages-feed patches and their application script.
- `h5-openwrt-25.12.6-bringup.patch`: these build inputs as one vanilla overlay patch.

Apply the exported patch once to the pinned OpenWrt tree. Do not also copy the
same directories. Fetch the pinned feeds, apply their patches, copy the config
seed and run the build as documented. OpenWrt downloads dependency sources
from their upstream locations and checks hashes specified by package recipes.
Internet access is required; offline builds and bit-for-bit reproducibility
have not been established. Availability of every upstream download is not
guaranteed indefinitely.

No device dump, private source tree, original vendor DTB, prebuilt custom
helper or private signing key is required to build the images. Signed modem
firmware stays in its original device partition and is used at runtime.

## Redistribution

Keeping this repository source-only avoids redistributing the entire download
cache and third-party firmware archives. It does not remove the licenses of
the source files and patches that are included here. Existing notices and
per-file licenses still apply.

If you distribute your own compiled images, satisfy each component's source,
license, copyright and firmware redistribution obligations. This overlay plus
generic upstream links is not automatically complete corresponding source
for a whole firmware image. Keep the actual build configuration, modified
sources, scripts and applicable notices for your build. A privacy scan or a
successful compilation is not a comprehensive license audit.