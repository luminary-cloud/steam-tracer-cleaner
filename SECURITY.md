# Security Policy

## Reporting a vulnerability

Please report security issues privately. **Do not open a public issue for a
vulnerability.**

Email **cloud@luminary.pw** with:

- a description of the issue and its impact,
- steps to reproduce (or a proof of concept),
- the app version (the Settings/footer shows it, or the release tag) and your Windows
  version.

You can expect an acknowledgement within a few days. Please give a reasonable window
to ship a fix before any public disclosure.

## Supported versions

Only the latest release on the [Releases](../../releases) page receives fixes.

## How destructive actions are protected

Cleaning deletes files and registry values, so every destructive run is gated:

- **Dry run.** A preview lists the exact files, registry keys, and values that would
  be touched. Nothing is deleted.
- **Backup before delete.** Affected paths and keys are mirrored to a timestamped
  folder under `%LOCALAPPDATA%\steam-tracer-cleaner\backups\` before the deletes run.
  Restore any run in one click from the Backups screen. Old backups beyond the keep
  count are pruned automatically.
- **Auto-close.** Steam (and, for configs, only the target game) is closed before any
  write, because Steam rewrites its `.vdf` files on exit.
- **Preserve list.** SteamID64s on the preserve list are never touched: their login
  entry, `userdata`, registry subtree, controller bindings, and cached avatar all
  survive every profile.
- **Action log.** A rotating log under `%LOCALAPPDATA%\steam-tracer-cleaner\logs\`
  records every action with a timestamp, target, result, and Win32 error code.

## Data handling

- **Local only.** Settings, profiles, the ignore list, backups, and logs stay on your
  machine (under `%APPDATA%` / `%LOCALAPPDATA%`, or beside the binary in portable mode).
- **Browser cookies.** To clear Steam web sessions, the cleaner reads the cookie
  databases of installed browsers (Chrome, Edge, Brave, Firefox) and removes only the
  Steam-related entries. It does not read or transmit anything else.
- **No telemetry.** The only outbound call is an optional GitHub release check on
  launch, which you can disable in Settings. Nothing else leaves the machine.

## Scope

- **Not a spoofer.** The Audit screen displays MachineGuid / MachineId / HwProfileGuid
  for reference. It never changes them.
- **No anti-cheat bypass.** Nothing here defeats VAC, Easy Anti-Cheat, BattlEye, or any
  other anti-cheat. This is a file cleaner.
- **Administrator elevation.** The app runs elevated because registry edits under
  `HKCU\Software\Valve\Steam` and per-account `userdata` writes require it on some
  setups.
- **No code signing.** Release binaries are unsigned, so SmartScreen warns on first
  run. Verify the SHA-256 published with each release if you want to confirm the
  download.
