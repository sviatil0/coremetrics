# Security Policy

## Supported Versions

Only the latest 0.2.x release is supported with security fixes. CoreMetrics is a
desktop monitor with no network listener, no auth surface, and no persistent state
the user does not explicitly produce. The threat surface is small: a malicious
process name could try to inject a control sequence into the terminal, or a
crafted CSV/JSON export consumer could be tricked into a spreadsheet formula
injection (already mitigated in src/Exporter.cpp per CWE-1236).

| Version | Supported          |
| ------- | ------------------ |
| 0.2.x   | :white_check_mark: |
| < 0.2.0 | :x:                |

## Reporting a Vulnerability

Use GitHub's private vulnerability reporting at
<https://github.com/sviatil0/coremetrics/security/advisories/new> or email
soleksiienko1@gmail.com directly.

Expect a first reply within 5 days. Confirmed vulnerabilities will be assigned a
CVE if appropriate, fixed on the dev branch, and shipped in the next 0.2.x patch
release. Coordinated disclosure: please do not file a public issue describing
the vulnerability before a patched release is tagged.

## Past advisories

None to date.
