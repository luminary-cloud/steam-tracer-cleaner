# steam-tracer-cleaner

Cleans Steam account residue and installs autoexec / video configs. Single-file Windows executable, no installer, no telemetry.

## Download

Grab the latest `steam-tracer-cleaner.exe` from the [Releases](../../releases) page. The binary is statically linked, so it runs on any 64-bit Windows 10 or 11 install with no Visual C++ Redistributable required.

On first launch Windows SmartScreen may show "Windows protected your PC" because the binary is unsigned. Click **More info** then **Run anyway**. Code-signing certificates are not free; this is normal for small open-source Windows tools.

## What it does

Three things, picked one at a time from the left-nav.

**Cleaner.** Walks a known catalog of Steam artifacts and removes the ones not on your preserve list:

- `loginusers.vdf` entries, `ssfn*` files, `HKCU\Software\Valve\Steam\Users\<SteamID64>` subkeys
- `AutoLoginUser` registry value (only if the current value is not on the preserve list)
- `htmlcache/` (Chromium cache for the in-client web view, leaks logged-in cookies)
- `dumps/`, `logs/`, `appcache/`, `depotcache/`, `shadercache/` on every Steam library drive
- `userdata/<AccountID>/` per game or wholesale, depending on the profile
- Steam-related minidumps in `C:\Windows\Minidump`, `%LOCALAPPDATA%\CrashDumps`
- Steam web sessions in Chrome / Edge / Brave / Firefox cookie databases

**Configs.** Two modes:

- *Autoexec*: pick a `.cfg`, the tool writes it to the game's cfg directory (default CS2 path: `steamapps/common/Counter-Strike Global Offensive/game/csgo/cfg/autoexec.cfg`). Existing file is backed up first.
- *Video config*: pick a `cs2_video.txt`, multi-select target accounts, the tool writes it to each `userdata/<AccountID>/730/local/cfg/cs2_video.txt`.

**Audit.** Read-only. Inventories what's on disk, lists every Steam account it sees, displays MachineGuid / MachineId / HwProfileGuid, exports a text report. No writes, no registry changes.

## What it doesn't do

- No HWID spoofing. The audit page shows the values, that's it.
- No bypassing of VAC, Easy Anti-Cheat, BattlEye, or any anti-cheat. This is a file cleaner.
- No bundled cheats, configs, or scripts beyond what you supply.
- No network calls. No update check. No analytics.

## First run

1. Launch `steam-tracer-cleaner.exe`. The window opens on the **Cleaner** screen.
2. Open **Settings** in the left-nav. Add any SteamID64s you want to keep across cleanups under *Preserved account ids*. Tick *Preserve all ssfn (Steam Guard sentry) files* if you want to skip the 2FA prompt on next login. Click **Save settings**.
3. Back on **Cleaner**, pick a profile from the dropdown:
   - **Quick Clean**: caches, logs, dumps. Safe daily use.
   - **Account Reset**: Quick + account residue (loginusers, AutoLogin, ssfn, htmlcache, browser cookies). Preserved accounts survive.
   - **Full Wipe**: Account Reset + userdata folders + controller bindings. Preserved accounts still survive.
   - **Game Reset**: pick AppIDs to wipe just those games' userdata.
4. Click **Dry run** to see the list of files / registry values that would be touched. Nothing is deleted.
5. Click **Backup and clean** when you're ready. Steam is auto-closed first; affected files and registry keys are mirrored to `%LOCALAPPDATA%/steam-tracer-cleaner/backups/<timestamp>/`; then the deletes run.
6. If something went wrong, open the **Backups** screen and click **Restore** on the most recent timestamped entry.

## Why an ignore list

If you keep more than one Steam account on a machine and only want to scrub some of them, a wholesale wipe of `HKCU\Software\Valve\Steam\Users` and `AutoLoginUser` will log you out of the accounts you wanted to keep. The cleaner reads `ignore.json` first and only acts on entries whose SteamID64 is not on the preserve list.

`ignore.json` lives at `%APPDATA%/steam-tracer-cleaner/ignore.json` (or next to the .exe in portable mode):

```json
{
    "preserved_account_ids": ["76561198000000000"],
    "preserved_ssfn_files": true,
    "preserved_paths": [],
    "preserved_registry_values": []
}
```

A SteamID64 in `preserved_account_ids` survives across:

- The matching `loginusers.vdf` entry
- The `userdata\<account_id>\` folder
- The `HKCU\Software\Valve\Steam\Users\<id>` registry subtree
- Controller bindings in that account's userdata
- `AutoLoginUser`, `RememberPassword`, and `LastGameNameUsed` (when AutoLoginUser currently points at this account)
- `remoteclients.vdf` and `coplay_*` files (preserved when any account is in the list, since they can't be cleanly split per-account)

## Build from source

The project is self-contained. Every dependency is vendored under `third_party/`, so there's no vcpkg, Conan, or git submodule to set up.

### Visual Studio (recommended)

1. Install Visual Studio 2022 17.x or 2026 18.x with the **Desktop development with C++** workload.
2. Open `steam-tracer-cleaner.sln`.
3. Set the configuration to `Release | x64` and hit Build (F7).

The binary lands at `build/Release/steam-tracer-cleaner.exe`.

### CMake / Ninja

For headless / CI builds:

```
cmake --preset release
cmake --build --preset release
```

The binary lands at `build/release/bin/steam-tracer-cleaner.exe`. The CMake build also compiles the `tests/` directory; run `ctest --preset debug` to execute the unit tests.

## Project layout

```
app/         WinMain, D3D11 device, message loop
core/        cleaning logic, profiles, ignore list, VDF helpers, autoexec
platform/    Win32 wrappers (registry, fs, process)
ui/          ImGui screens and widgets
tests/       doctest unit tests (CMake only)
third_party/ vendored deps: imgui, spdlog, nlohmann_json, sqlite, doctest
assets/      icon (compiled into the .exe via app.rc)
cmake/       shared compile options
```

The `core/ <-> platform/` split means cleaning rules can be unit-tested without a Win32 message pump.

## Safety

Every destructive action is gated by:

1. A dry-run preview that shows the exact list of files / keys / values it would touch.
2. A backup step that mirrors the affected paths to `%LOCALAPPDATA%/steam-tracer-cleaner/backups/<timestamp>/` before deleting. One-click restore from the Backups screen. Old backups beyond the configured keep-count are auto-pruned after each successful run.
3. An auto-close step. Steam is sent WM_CLOSE before any write, with a TerminateProcess fallback after a 5-second timeout, so it can't rewrite `loginusers.vdf` or `localconfig.vdf` mid-clean.
4. A rotating log at `%LOCALAPPDATA%/steam-tracer-cleaner/logs/<name>.log` recording every action with timestamp, target id, result, and Win32 error code if applicable.

## Portable mode

Drop a file named `portable.flag` next to the .exe. Settings, profiles, ignore list, backups, and logs all live in the same folder as the binary instead of `%APPDATA%` and `%LOCALAPPDATA%`. Useful from a USB stick or a sandbox.

## Vendored libraries

| Library | License | Used for |
|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) (docking) | MIT | UI |
| [spdlog](https://github.com/gabime/spdlog) | MIT | logging (header-only mode) |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT | config files |
| [SQLite amalgamation](https://www.sqlite.org/) | Public domain | browser cookie DBs |
| [doctest](https://github.com/doctest/doctest) | MIT | unit tests |

## License

[MIT](LICENSE).

## Contributing

Pull requests welcome. Code style: `clang-format` config is at the repo root, `clang-tidy` is set up for `bugprone-*` / `cert-*` / `cppcoreguidelines-*` / `modernize-*`. Tests run via `ctest` after a CMake build.
