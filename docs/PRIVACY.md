# Privacy Review

Review date: 2026-10-27.

The publication directory was populated from an explicit source allowlist:
the SDX target, custom packages, feed patches, documentation and exported patch.
The development workspace itself, device backups and diagnostic directories
were not used as the Git repository.

The source overlay was scanned for private keys, credentials, configured
SIM secrets, local build identities and a known device-specific identifier.
No private-data findings were reported in its 59 source files.
The final publication directory is also checked before the initial commit.

Both privately tested firmware images were inspected beyond their compressed
containers. The review decompressed each kernel, parsed the initramfs, and
extracted the sysupgrade squashfs. It inspected 1,304 initramfs entries and
919 regular squashfs files. The special device node /dev/console was excluded
from unprivileged extraction; it is not a regular file containing settings.

The older decompressed kernels contained a private build account/hostname and
absolute build paths. Therefore **neither original image is published**.
The account, hostname, paths and raw audit logs are omitted from this report.

A local candidate was rebuilt from a cleaned kernel build with neutral
`builder@buildhost` identity and compiler file/debug/macro prefix mapping.
The same decompression and filesystem checks then reported zero findings for
the new images. An additional production validator verified the boot headers,
DTB, login setup, absence of the private debugging overlay, unset root password,
modem tools, and persistent-root/sysupgrade support. That initial candidate
was not flashed at the time of this audit.

A later CPE rebuild with the USB boot-console correction was installed and
passed main/recovery cold boots without USB. Its runtime kernel identity is
`builder@buildhost`, but a complete new decompressed-content privacy audit was
not completed. The earlier zero-findings result must not be treated as an
audit of this newer build or of a future third-party build.

Neither the rebuilt binaries nor the dependency-source snapshot is distributed.
The project is source-only; local build and audit artifacts remain private.
The source checks do not certify every possible future dependency or build.

The audit distinguishes complete PEM key blocks from mere format strings in
cryptographic libraries, actual random-seed files from the scripts that manage
them, and active UCI secrets from commented configuration examples.

Excluded publication material includes:

- SIM PIN/PUK, saved APN profiles, device passwords and private keys.
- Device identifiers, calibration, EFS/NV and full/partial device backups.
- Saved host keys, access helpers, personal profiles and raw diagnostic logs.
- Original modem firmware and the old, privacy-rejected compiled images.

This was a scoped static review, not a guarantee that arbitrary future builds
are anonymous. Audit new images independently, including compressed content,
before publication. Do not upload the entire private development directory.