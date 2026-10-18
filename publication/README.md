# Publication policy

The public repository contains the current reviewed code. Development releases pin their exact source commit and include matching source archives and SHA-256 checksums. Newer commits on main do not imply that an older binary was built from them.

Original game content, private executables containing original fonts, generated font includes, images/captures, disc files, saves, private logs, secrets and host-specific paths are excluded. The executable template contains zero-filled font regions; local preparation checks the user's disc and restored executable hashes. Third-party DLLs and Microsoft's CRT are obtained separately through the documented runtime build and Microsoft redistributable.

Latest release validation includes a local reconstruction hash comparison, refusal to overwrite an existing output, and a muted 600-frame native smoke. This does not accept the complete port or resolve the separate projectile-hit and intermittent visibility faults.

A local daily publishing check updates reviewed source changes. New binary releases require explicit matching build provenance and verification. It creates no empty daily commits and never force-pushes. The local machine, Codex automation, network and GitHub authorization must be available for publication.
