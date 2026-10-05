# YeniZil — rules for working on this repo

Wireless doorbell + door opener for a 4-flat building. Six ESP32-C3 Super Mini boards: bell panel (dış1, outside, buttons only), door unit (dış2, inside, door relay, hub of the building network), 4 indoor units. Arduino core 3.3.12, ESP-NOW, AES-128-CCM. Two networks with separate IDs and keys: the bell panel link (dış1 → dış2, direct) and the building network (dış2 + indoor units, flooding). The bell panel never holds the building key.
Design decisions and their rationale live in `docs/ARCHITECTURE.md` (the numbered "Karar" list is the ADR log). This file holds the rules.

## 1. Working agreement

- Talk to the user in Turkish. Code identifiers are English, code comments are Turkish.
- Be direct: if the user (or an earlier decision) is wrong, say so with the correct term and the reason.
- Claude writes code, compiles it, and uploads it **only when the user says "yükle"**. The user tests the boards physically.
- No test sketches, bench tools, helper scripts or debug/serial logging unless the user explicitly asks. A requested test lives on its own branch and is removed when the user says so.
- Commit and push only when asked. Everything is committed, including `bell_panel_config.h` / `door_unit_config.h` / `indoor_config.h` with the network keys. The repo is public on purpose. Do not add `.example.h` templates or `.gitignore` entries for them, and do not raise the key-exposure topic again.
- Before destructive git operations (restore, reset, branch delete), check `git status` and name every file that will lose changes.

## 2. Priority when rules conflict

Security > correctness > simplicity > consistency > performance.

## 3. Scope: what code may exist

Only code that does one of these five jobs:
1. The job itself (buttons, bell/relay outputs, messaging, relaying)
2. Security
3. Power saving
4. Checks (input validation, compile-time config/wiring checks)
5. Optimizations (only when they pay for themselves)

## 4. Choosing what to build on (first match wins)

1. Arduino-ESP32 core API or bundled library (`WiFi`, `ESP_NOW`, `Preferences`, `random`, `pinMode`/`digitalWrite`, `setCpuFrequencyMhz`, `enableLoopWDT`)
2. ESP-IDF / FreeRTOS, only when Arduino has no equivalent (`esp_timer`, `gpio_set_level`, mbedTLS, FreeRTOS queue/notify)
3. A widely used, maintained third-party library
4. Own code, only if none of the above fits or is reliable

For every step-2 or step-4 use, add a row with the reason to `docs/ARCHITECTURE.md` §5.2. If a reliable library could replace own code, tell the user.

## 5. Design principles

**Standards first.** When a problem has an established standard or reference solution, implement it the way the standard says and cite it in a trailing comment. Examples: IEEE 802.15.4 nonce and replay protection, RFC 6347 replay window, OpenThread frame-counter reserve, BLE Mesh network retransmit, NIST SP 800-38C (CCM). Exception: the user's own setup and install flow stays simple even when a standard suggests otherwise. For example, flat IDs are set in a config file before flashing, not by a learn-mode pairing.

**OOP**
- Encapsulation: state is private, the public API is minimal, helpers are private.
- Abstraction: classes speak the domain language (`bell.ring()`, `intercom.requestDoorOpen()`).
- Inheritance only for "is-a" (`Button` is a `PollingComponent`). Use composition for "has-a" (`Bell` has a `PulseOutput`).
- Virtual functions only where heterogeneous objects must share one runtime list (`Component`, `FrameReceiver`) or a library requires it (`ESP_NOW_Peer`).

**SOLID**
- **S**: one reason to change per class. `EspNowRadio` = radio I/O and frame dispatch, `BurstSender` = repetition, `TxCounter` = send counter, `SecureChannel` = crypto + replay for one network, `FloodRouter` = building-network routing, `PanelLink` = bell panel link.
- **O**: extend by adding types or config, not by editing working classes. For example, a new broadcast type is added in `protocol.h` (`isBroadcast`), and the router does not change.
- **L**: every `Component` honours the contract: `begin()` prepares hardware, `update()` never blocks, `nextDeadlineMs()` is accurate.
- **I**: small interfaces. `Component` has three methods.
- **D**: dependencies come in through constructors as references. No class reaches a global. Only the composition roots (`bell_panel.h`, `door_unit.h`, `indoor.h`) create objects.

**DRY, KISS, YAGNI**
- One source of truth per fact or rule (pin levels → `digital_pin.h`, handler invocation → `callIfSet`, frame layout → `frame.h`). The second copy of code becomes a shared helper. Known deliberate duplicate: network ID and key in both unit config files.
- Pick the simpler of two working solutions.
- Write an abstraction only when there are at least two real uses (rule of two). No single-implementation interfaces.

**Also applied**
- Separation of concerns, high cohesion and low coupling (GRASP), Information Expert.
- Tell, don't ask. Law of Demeter: `.ino` talks to `intercom`, never to the router behind it.
- Command–query separation. Where a method both acts and returns a status (`seal`, `open`, `commit`, `send`), that is deliberate. Mark it `[[nodiscard]]` when the result must be checked.
- Fail fast: invalid config or wiring must not compile (`static_assert`).
- Make illegal states unrepresentable: `enum class`, `std::optional`, fixed-extent `std::span`. Ids and counters are strong types in the `std::byte` idiom (`enum class NodeId : uint8_t {}`, `ApartmentId`, `FrameCounter`). Use `toUnderlying()` at byte/storage boundaries only.
- Principle of least astonishment.

## 6. Patterns (use the standard name, only where it truly fits)

| Pattern | Where |
|---|---|
| Facade | `Intercom` builds and hides the building network |
| Template Method | `PollingComponent` (timing) → `poll()` in subclasses |
| Observer (function-pointer callbacks) | `onPress`, `onRing`, `onRingRequest`, `onHeartbeat`, `onTick` |
| Observer (interface) | `EspNowRadio` → `FrameReceiver` (`FloodRouter`, `PanelLink`) |
| Adapter | `BroadcastPeer` over Arduino `ESP_NOW_Peer` |
| State machine (explicit `enum class` states) | `PressDetector` |
| Reactor / cooperative scheduler | `EventLoop` |
| Producer–Consumer | ESP-NOW receive callback → static FreeRTOS queue → loop |
| Pipes and Filters (fixed order) | `SecureChannel::open` |
| Composition Root + Dependency Injection | `bell_panel.h`, `door_unit.h`, `indoor.h` |
| Intrusive registry | `Component` self-registration, `FrameReceiver` list in the radio (no heap) |
| Command | message types |

Not used on purpose: Singleton (use DI), runtime middleware chains, CRTP, policy-based design, template metaprogramming. Decorator is allowed if a cross-cutting need appears (e.g. logging a radio).

## 7. Architecture and file layout

Layers, dependencies only point down: Sketch (`*.ino`) → Unit (`bell_panel.h`/`door_unit.h`/`indoor.h`) → App → Services → Platform → Core → Kernel. `common/config` gives values and settings types to every layer and depends on nothing. Core files (`press_detector`, `replay_window`, `frame`, `protocol`, `byte_order`, `enum_value`, `static_checks`) are pure C++ and must never include Arduino/ESP-IDF. Other layers use Arduino/ESP-IDF directly (ports and adapters: `EspNowRadio`, `BroadcastPeer`, `CounterStore`, `CcmCipher`, `digital_pin`, `clock`, `EventLoop`).

```
bell_panel/ bell_panel.ino · bell_panel.h · bell_panel_config.h · hardware.h
door_unit/  door_unit.ino · door_unit.h · door_unit_config.h · hardware.h
indoor/     indoor.ino · indoor.h · indoor_config.h · hardware.h
common/  app/ config/ io/ kernel/ net/ power/ security/
docs/ARCHITECTURE.md
```

- `*.ino`: only "event → action" bindings plus `using namespace yenizil;`. Never edited to change a setting or behaviour. Different behaviour is solved with OOP.
- `bell_panel.h` / `door_unit.h` / `indoor.h`: composition root plus unit settings (pulse lengths, press rules, `NetworkCredentials`) and their `static_assert`s.
- `*_config.h`: only values changed before flashing: network IDs and keys (building network, bell panel link), and for indoor units `kFlatId`. Nothing else.
- `hardware.h`: only externally wired parts (pins, active levels, wiring notes) and pin checks. On-board parts (e.g. GPIO8 LED) do not belong here.
- `common/config/*_config.h`: product and building values. `*_settings.h`: settings structs.
- All shared code is header-only (`inline`). Arduino IDE does not compile `.cpp` files outside the sketch folder, and its `.ino` preprocessor breaks some modern C++ (e.g. `consteval`), so such code stays in headers.

## 8. Naming and style

- Files and folders `snake_case`. A sketch folder and its `.ino` share the same name.
- Types `PascalCase`, functions and variables `camelCase`, constants `kPascalCase`, private members `name_`, enumerators `kPascalCase`.
- Everything lives in `namespace yenizil` (`yenizil::config`, `::pins`, `::board`, `::frame`). `using namespace` only in `.ino`.
- `#pragma once`, `enum class`, `inline constexpr` constants. No `#define`.
- Standard headers: `<cstdint>`, not `<stdint.h>`.
- Comments: Turkish, end-of-line only, short. No full-line or block comments. Long explanations go to `docs/ARCHITECTURE.md`.
- No magic numbers: name them, and tie related constants together with `static_assert` (see `frame.h`, `counter_store.h`).
- Keep columns aligned in declaration blocks, as in the existing files.

## 9. C++ and embedded rules (C++ Core Guidelines, Google C++ Style, NASA/JPL Power of Ten)

- No heap after `setup()`: no `new`, `String`, `std::string`, `std::vector`, `std::function`, `std::map`. Static storage only (`std::array`, static FreeRTOS queue).
- Lambdas do not capture, so they decay to plain function pointers (`Handler<>`).
- No exceptions, no RTTI. Errors are return values. A component that cannot start stays off and does not crash.
- Check every return value, or discard it explicitly with `static_cast<void>(...)` and a comment saying why it is safe (Power of Ten #7). Mark functions whose result matters `[[nodiscard]]`.
- Simple control flow, bounded loops, short functions, smallest possible scope.
- RAII, Rule of Zero. Hardware-owning classes are non-copyable. Single-argument constructors are `explicit`.
- Const-correctness: `const` methods and `constexpr` / `consteval` wherever possible.
- Constructors only store settings. Hardware is touched in `begin()`, because globals are constructed before Arduino is ready.
- Callbacks from other tasks (ESP-NOW receive) only copy, enqueue and notify. All logic runs on the loop task.
- `update()` never blocks. The loop waits at most 1 s, well under the 5 s watchdog.
- Outputs are fail-safe: driven to their inactive level before `pinMode(OUTPUT)`. External 10k pull-downs on relay and bell pins.
- Compiler warnings should be clean. Run a `--warnings all` build occasionally in a separate build path (see §11).

## 10. Security rules

- Kerckhoffs: only the key is secret. Use standard AES-128-CCM (mbedTLS) with an 8-byte tag.
- Least privilege: the bell panel (outside) holds only the bell panel link key. The door unit accepts only ring requests from that link and re-sends them on the building network. The two networks must have different IDs and keys (`isSeparate`).
- Nonce = sender MAC + persistent counter, never reused (NIST SP 800-38C, 802.15.4). One `TxCounter` per board, shared by all its networks. The TX counter reserve must be saved before use. Stop sending rather than reuse a counter.
- Processing order is fixed: cheap checks → read-only replay check → authenticate/decrypt → advance replay window → action. Allocate peer slots only after authentication.
- Persist the RX counter before acting. If persisting fails, do not act.
- Deny by default: drop unknown versions, apartments, types, destinations and malformed addressing.
- Never flash with "erase all flash": it resets counters and causes nonce reuse.

## 11. Build, upload, verify

Use the PowerShell tool (Git Bash is slower). Persistent build path, separate from the IDE's cache:

```
arduino-cli compile -b esp32:esp32:esp32c3 --build-path "$env:LOCALAPPDATA\arduino\claude-build\<sketch>" C:\Users\Victus\Desktop\YeniZil\<sketch>
arduino-cli board list
arduino-cli upload  -b esp32:esp32:esp32c3 -p <COMx> --input-dir "$env:LOCALAPPDATA\arduino\claude-build\<sketch>" C:\Users\Victus\Desktop\YeniZil\<sketch>
```

- After every code change, compile all three sketches (`bell_panel`, `door_unit`, `indoor`). A cold build takes ~75 s, a cached one ~16 s. If a build passes ~2 min, stop it and investigate.
- Do not change board options or add `--warnings all` on the normal build path: that invalidates the cache. Use a separate path such as `claude-build\<sketch>-warn` for warning checks.
- Upload only on "yükle". Ask which unit is on which port. For an indoor unit, set `kFlatId` first.
- `arduino-cli monitor` only if the user asks.

## 12. Git and process

- Small, single-purpose commits. Conventional Commits prefixes: `feat:`, `fix:`, `refactor:`, `docs:`, `chore:`. End messages with the `Co-Authored-By` line.
- `kProtocolVersion` follows SemVer thinking: bump it on any wire-format change, and remind the user that all six boards must be reflashed.
- Boy Scout rule: leave touched code cleaner. Keep `docs/ARCHITECTURE.md` in sync with every design change, and add a "Karar" entry for new decisions.
- Default branch `master`, remote `origin` (github.com/murat-kacar/YeniZil).

## 13. Hardware facts

- Safe GPIOs on the Super Mini: 0, 1, 3, 4, 5, 6, 7, 10. Avoid 2/8/9 (strapping), 18/19 (USB), 20/21 (UART0).
- Bell panel: buttons GPIO3–6 (flats 1–4, to GND). Door unit: door relay trigger GPIO10 (10k pull-down). Indoor: open-door button GPIO10, bell GPIO0 (≤ 2.5 mA, direct), link LED GPIO1.
- Every LED the user wires gets a **240 Ω series resistor**. A parallel pull-down does not limit current. A resistor-less LED destroyed the old outdoor board.
- A GPIO pin sources ~20 mA safely. Bigger or inductive loads need a transistor/MOSFET plus a flyback diode.
- 5 V / 300 mA adapters. The radio listens continuously (~90–100 mA average). Above 14 dBm TX power, use at least 500 mA adapters.

## 14. Decisions on optional rules

- Null Object handlers were evaluated and **not adopted**: they would move the null check from 5 call sites into ~10 setters. `callIfSet` stays.
