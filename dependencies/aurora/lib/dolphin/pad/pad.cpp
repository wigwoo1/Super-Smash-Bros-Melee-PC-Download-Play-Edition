#include "../../input.hpp"
#include "../../device.hpp"
#include "../../internal.hpp"
#include "../../io.hpp"
#include "../../keyboard_state.hpp"
#include "../../keyboard_press_latch.hpp"
#include "../../pad_axis_merge.hpp"
#include "../../pad_diagnostic_isolation.hpp"
#include <dolphin/pad.h>
#include <dolphin/si.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>

#include <array>
#include <filesystem>
#include <limits>
#include <ranges>
#include <sys/stat.h>

namespace {
constexpr aurora::Module Log{"aurora::input"};

constexpr int32_t k_mappingsFileVersion = 4;
constexpr int32_t k_minMappingsFileVersion = 3;

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsStandard{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADStatus, PAD_CHANMAX> g_virtualPadStatus{};
std::array<bool, PAD_CHANMAX> g_virtualPadActive{};
SDL_JoystickID g_diagnosticSdlInstance = 0;

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsXBox360{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsXBoxOne{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsPS3{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsPS4{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsPS5{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsGamecube{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {SDL_GAMEPAD_BUTTON_MISC3, PAD_TRIGGER_L},
    {SDL_GAMEPAD_BUTTON_MISC4, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsNSOGamecube{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_BACK, PAD_TRIGGER_Z},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, PAD_TRIGGER_L},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsProCon{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsJoyConRight{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsJoyConLeft{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADButtonMapping, PAD_BUTTON_COUNT> g_defaultButtonsJoyPair{{
    {SDL_GAMEPAD_BUTTON_SOUTH, PAD_BUTTON_A},
    {SDL_GAMEPAD_BUTTON_EAST, PAD_BUTTON_B},
    {SDL_GAMEPAD_BUTTON_WEST, PAD_BUTTON_X},
    {SDL_GAMEPAD_BUTTON_NORTH, PAD_BUTTON_Y},
    {SDL_GAMEPAD_BUTTON_START, PAD_BUTTON_START},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_TRIGGER_Z},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L},
    {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_R},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_BUTTON_UP},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_BUTTON_DOWN},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_BUTTON_LEFT},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_BUTTON_RIGHT},
}};

std::array<PADKeyButtonBinding, PAD_BUTTON_COUNT> g_defaultKeys{{
    {PAD_KEY_INVALID, PAD_BUTTON_A},
    {PAD_KEY_INVALID, PAD_BUTTON_B},
    {PAD_KEY_INVALID, PAD_BUTTON_X},
    {PAD_KEY_INVALID, PAD_BUTTON_Y},
    {PAD_KEY_INVALID, PAD_BUTTON_START},
    {PAD_KEY_INVALID, PAD_TRIGGER_Z},
    {PAD_KEY_INVALID, PAD_TRIGGER_L},
    {PAD_KEY_INVALID, PAD_TRIGGER_R},
    {PAD_KEY_INVALID, PAD_BUTTON_UP},
    {PAD_KEY_INVALID, PAD_BUTTON_DOWN},
    {PAD_KEY_INVALID, PAD_BUTTON_LEFT},
    {PAD_KEY_INVALID, PAD_BUTTON_RIGHT},
}};

std::array<PADKeyAxisBinding, PAD_AXIS_COUNT> g_defaultKeyAxis{{
    {PAD_KEY_INVALID, PAD_AXIS_LEFT_X_POS, 0},
    {PAD_KEY_INVALID, PAD_AXIS_LEFT_X_NEG, 0},
    {PAD_KEY_INVALID, PAD_AXIS_LEFT_Y_POS, 0},
    {PAD_KEY_INVALID, PAD_AXIS_LEFT_Y_NEG, 0},
    {PAD_KEY_INVALID, PAD_AXIS_RIGHT_X_POS, 0},
    {PAD_KEY_INVALID, PAD_AXIS_RIGHT_X_NEG, 0},
    {PAD_KEY_INVALID, PAD_AXIS_RIGHT_Y_POS, 0},
    {PAD_KEY_INVALID, PAD_AXIS_RIGHT_Y_NEG, 0},
    {PAD_KEY_INVALID, PAD_AXIS_TRIGGER_L, 0},
    {PAD_KEY_INVALID, PAD_AXIS_TRIGGER_R, 0},
}};

std::array<PADAxisMapping, PAD_AXIS_COUNT> g_defaultAxes{{
    {{SDL_GAMEPAD_AXIS_LEFTX, AXIS_SIGN_POSITIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_LEFT_X_POS},
    {{SDL_GAMEPAD_AXIS_LEFTX, AXIS_SIGN_NEGATIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_LEFT_X_NEG},
    // SDL's gamepad y-axis is inverted from GC's
    {{SDL_GAMEPAD_AXIS_LEFTY, AXIS_SIGN_NEGATIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_LEFT_Y_POS},
    {{SDL_GAMEPAD_AXIS_LEFTY, AXIS_SIGN_POSITIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_LEFT_Y_NEG},
    {{SDL_GAMEPAD_AXIS_RIGHTX, AXIS_SIGN_POSITIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_X_POS},
    {{SDL_GAMEPAD_AXIS_RIGHTX, AXIS_SIGN_NEGATIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_X_NEG},
    // see above
    {{SDL_GAMEPAD_AXIS_RIGHTY, AXIS_SIGN_NEGATIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_Y_POS},
    {{SDL_GAMEPAD_AXIS_RIGHTY, AXIS_SIGN_POSITIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_Y_NEG},
    {{SDL_GAMEPAD_AXIS_LEFT_TRIGGER, AXIS_SIGN_POSITIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_TRIGGER_L},
    {{SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, AXIS_SIGN_POSITIVE}, SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_TRIGGER_R},
}};

template <typename T, size_t N>
constexpr const std::array<T, N>& toStdArray(const T (&array)[N]) {
  static_assert(sizeof(array) == sizeof(std::array<T, N>));
  return reinterpret_cast<const std::array<T, N>&>(array);
}

struct PADKeyboardState {
  std::array<PADKeyButtonBinding, PAD_BUTTON_COUNT> m_buttonMapping{};
  std::array<PADKeyAxisBinding, PAD_AXIS_COUNT> m_axisMapping{};
  bool m_mappingsSet = false;
};

std::array<PADKeyboardState, PAD_MAX_CONTROLLERS> g_keyboardBindings;

struct PADCLampRegion {
  uint8_t minTrigger;
  uint8_t maxTrigger;
  int8_t minStick;
  int8_t maxStick;
  int8_t xyStick;
  int8_t minSubstick;
  int8_t maxSubstick;
  int8_t xySubstick;
  int8_t radStick;
  int8_t radSubstick;
};

constexpr PADCLampRegion ClampRegion{
    // Triggers
    30,
    180,

    // Left stick
    15,
    72,
    40,

    // Right stick
    15,
    59,
    31,

    // Stick radii
    56,
    44,
};

bool g_initialized;
bool g_keyboardBindingsLoaded = false;
bool g_blockPAD = false;
bool g_controlsEditing = false;
struct NativeBindingCapture {
  bool active = false, armed = false, keyboard = true, axis = false;
  u32 port = 0;
  u16 output = 0;
  s32 key = PAD_KEY_INVALID;
  Uint64 deadline = 0;
} g_bindingCapture;
bool g_nativeControlsPage = false, g_controlsNavigationReady = false;
u16 g_controlsNavigationLast = 0;
unsigned g_controlsNavigationFrames = 0;
aurora::input::KeyboardPressLatch<SDL_SCANCODE_COUNT> g_controlsNavigationKeys;

void reset_controls_navigation() {
  g_controlsNavigationReady = false;
  g_controlsNavigationLast = 0;
  g_controlsNavigationFrames = 0;
  g_controlsNavigationKeys.clear();
}
bool g_windowFocused = true;
bool g_suppressHeldOnRead = false;
aurora::input::KeyboardPressLatch<SDL_SCANCODE_COUNT> g_pendingKeyPresses;
bool key_tap_diagnostic() {
  static const bool enabled=std::getenv("AURORA_DIAGNOSTIC_KEY_TAPS") != nullptr;
  return enabled;
}
std::array<PADButton, PAD_CHANMAX> g_suppressedButtons{};
std::array<PADExtButton, PAD_CHANMAX> g_suppressedExtButtons{};
std::array<bool, PAD_CHANMAX> g_suppressLeftTrigger{};
std::array<bool, PAD_CHANMAX> g_suppressRightTrigger{};
std::array<std::array<bool, 4>, PAD_CHANMAX> g_suppressedSticks{};
std::atomic_bool g_diagnosticPrizeTrace{false};
std::atomic_bool g_diagnosticPrizeTraceLastA{false};
std::atomic_uint g_diagnosticPrizeTraceWrites{0};
std::atomic_uint g_diagnosticPrizeTraceReads{0};
std::atomic<long long> g_diagnosticPrizeTraceWindowEndNs{0};

static long long diagnostic_prize_trace_now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool is_mouse_scancode(const s32 scancode) { return scancode < PAD_KEY_INVALID; }
bool is_mouse_button_pressed(const s32 scancode) {
  const int32_t buttonNum = -(scancode + 1);
  if (buttonNum < 1 || buttonNum > 5) {
    return false;
  }
  float x, y;
  const auto buttons = SDL_GetMouseState(&x, &y);
  return (buttons & 1u << (buttonNum - 1)) != 0u;
}
} // namespace

void PADSetSpec(u32 spec [[maybe_unused]]) {}

static void load_keyboard_bindings();
static void save_keyboard_bindings();

static bool seek_aligned(SDL_IOStream* file, Sint64& dataStart) {
  const Sint64 position = SDL_TellIO(file);
  if (position < 0 || position > std::numeric_limits<Sint64>::max() - 31) {
    return false;
  }
  dataStart = (position + 31) & ~Sint64{31};
  return SDL_SeekIO(file, dataStart, SDL_IO_SEEK_SET) == dataStart;
}

static bool device_rumble_available_for_port(const u32 port) {
  return port == PAD_CHAN0 && aurora::device::rumble_available();
}

static bool should_use_device_rumble(const u32 port, const aurora::input::GameController* controller) {
  return device_rumble_available_for_port(port) &&
         (controller == nullptr ||
          (!controller->m_isGameCube && (!controller->m_hasRumble || controller->m_forceDeviceRumble)));
}

static bool device_gyro_available_for_port(const u32 port) {
  return port == PAD_CHAN0 && aurora::device::gyro_available();
}

static bool device_accel_available_for_port(const u32 port) {
  return port == PAD_CHAN0 && aurora::device::accel_available();
}

static bool device_sensor_available_for_port(const u32 port, const PADSensorType sensor) {
  switch (sensor) {
  case PAD_SENSOR_ACCEL:
    return device_accel_available_for_port(port);
  case PAD_SENSOR_GYRO:
    return device_gyro_available_for_port(port);
  default:
    return false;
  }
}

static bool get_device_sensor_data(const PADSensorType sensor, f32* data, const int nValues) {
  switch (sensor) {
  case PAD_SENSOR_ACCEL:
    return aurora::device::accel(data, nValues);
  case PAD_SENSOR_GYRO:
    return aurora::device::gyro(data, nValues);
  default:
    return false;
  }
}

static bool controller_has_sensor(const aurora::input::GameController* controller, const PADSensorType sensor) {
  return controller != nullptr && SDL_GamepadHasSensor(controller->m_controller, static_cast<SDL_SensorType>(sensor));
}

static bool should_use_device_sensor(const u32 port, const aurora::input::GameController* controller,
                                     const PADSensorType sensor) {
  return device_sensor_available_for_port(port, sensor) && !controller_has_sensor(controller, sensor);
}

// ReSharper disable once CppDFAConstantFunctionResult
BOOL PADInit() {
  if (g_initialized) {
    return true;
  }
  g_initialized = true;

  std::ranges::for_each(g_keyboardBindings, [](auto& state) {
    state.m_buttonMapping = g_defaultKeys;
    state.m_axisMapping = g_defaultKeyAxis;
  });

  if (!g_keyboardBindingsLoaded) {
    g_keyboardBindingsLoaded = true;
    load_keyboard_bindings();
  }

  return true;
}

BOOL PADRecalibrate(u32 mask [[maybe_unused]]) { return true; }

BOOL PADReset(u32 mask [[maybe_unused]]) { return true; }

void PADSetAnalogMode(u32 mode [[maybe_unused]]) {}

aurora::input::GameController* __PADGetControllerForIndex(const u32 idx) /*  NOLINT(*-reserved-identifier) */
{
  if (idx >= aurora::input::g_GameControllers.size()) {
    return nullptr;
  }

  uint32_t tmp = 0;
  auto iter = aurora::input::g_GameControllers.begin();
  while (tmp < idx) {
    ++iter;
    ++tmp;
  }
  if (iter == aurora::input::g_GameControllers.end()) {
    return nullptr;
  }

  return &iter->second;
}

u32 PADCount() { return aurora::input::g_GameControllers.size(); }

const char* PADGetNameForControllerIndex(const u32 idx) {
  const auto* ctrl = __PADGetControllerForIndex(idx);
  if (ctrl == nullptr) {
    return nullptr;
  }

  return SDL_GetGamepadName(ctrl->m_controller);
}

BOOL PADTrySetPortForIndex(const u32 idx, const u32 port) {
  if (port >= PAD_CHANMAX) return FALSE;
  const auto* ctrl = __PADGetControllerForIndex(idx);
  if (!ctrl) return FALSE;
  const int32_t oldPort = SDL_GetGamepadPlayerIndex(ctrl->m_controller);
  const auto* dest = aurora::input::get_controller_for_player(port);
  if (!SDL_SetGamepadPlayerIndex(ctrl->m_controller, static_cast<Sint32>(port))) return FALSE;
  if (dest && dest != ctrl && !SDL_SetGamepadPlayerIndex(dest->m_controller, -1)) {
    SDL_SetGamepadPlayerIndex(ctrl->m_controller, oldPort);
    return FALSE;
  }
  if (oldPort >= 0 && oldPort != port) aurora::input::persist_controller_for_player(oldPort, nullptr);
  aurora::input::persist_controller_for_player(port, ctrl);
  return TRUE;
}

void PADSetPortForIndex(const u32 idx, const u32 port) {
  (void)PADTrySetPortForIndex(idx, port);
}

int32_t PADGetIndexForPort(const u32 port) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return -1;
  }
  int32_t index = 0;
  for (auto iter = aurora::input::g_GameControllers.begin(); iter != aurora::input::g_GameControllers.end();
       ++iter, ++index) {
    if (&iter->second == ctrl) {
      break;
    }
  }

  return index;
}

void PADClearPort(const u32 port) {
  aurora::input::persist_controller_for_player(port, nullptr);
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return;
  }
  SDL_SetGamepadPlayerIndex(ctrl->m_controller, -1);
}

void __PADSetDefaultMapping(aurora::input::GameController* controller) /*  NOLINT(*-reserved-identifier) */
{
  switch (SDL_GetGamepadType(controller->m_controller)) {
  case SDL_GAMEPAD_TYPE_XBOX360:
    controller->m_buttonMapping = g_defaultButtonsXBox360;
    break;
  case SDL_GAMEPAD_TYPE_XBOXONE:
    controller->m_buttonMapping = g_defaultButtonsXBoxOne;
    break;
  case SDL_GAMEPAD_TYPE_STANDARD:
    controller->m_buttonMapping = g_defaultButtonsStandard;
    break;
  case SDL_GAMEPAD_TYPE_PS3:
    controller->m_buttonMapping = g_defaultButtonsPS3;
    break;
  case SDL_GAMEPAD_TYPE_PS4:
    controller->m_buttonMapping = g_defaultButtonsPS4;
    break;
  case SDL_GAMEPAD_TYPE_PS5:
    controller->m_buttonMapping = g_defaultButtonsPS5;
    break;
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    if (controller->m_pid == 0x2073) {
      controller->m_buttonMapping = g_defaultButtonsNSOGamecube;
    } else {
      controller->m_buttonMapping = g_defaultButtonsProCon;
    }
    break;
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    controller->m_buttonMapping = g_defaultButtonsJoyConRight;
    break;
  case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    controller->m_buttonMapping = g_defaultButtonsJoyConLeft;
    break;
  case SDL_GAMEPAD_TYPE_GAMECUBE:
    controller->m_buttonMapping = g_defaultButtonsGamecube;
    break;
  default:
    controller->m_buttonMapping = g_defaultButtonsStandard;
    break;
  }
}

void __PADLoadMapping(aurora::input::GameController* controller) /*  NOLINT(*-reserved-identifier) */ {
  int32_t playerIndex = SDL_GetGamepadPlayerIndex(controller->m_controller);
  if (playerIndex < 0) {
    return;
  }

  if (!controller->m_mappingLoaded) {
    __PADSetDefaultMapping(controller);
    controller->m_axisMapping = g_defaultAxes;
  }

  controller->m_mappingLoaded = true;
  // Session-only mode still requires usable defaults; only disk loading stops.
  if (aurora::g_config.userPath == nullptr) return;

  const auto path = aurora::io::fs_path_from_string(aurora::g_config.userPath) /
                    fmt::format("{}_{:04X}_{:04X}.controller", PADGetName(playerIndex), controller->m_vid,
                                controller->m_pid);
  auto file = aurora::io::open_file(path, "rb");
  if (!file) {
    return;
  }

  uint32_t magic = 0;
  if (!SDL_ReadU32LE(file.get(), &magic) || magic != SBIG('CTRL')) {
    Log.warn("Invalid controller mapping magic!");
    return;
  }

  uint32_t version = 0;
  if (!SDL_ReadU32LE(file.get(), &version) || version < k_minMappingsFileVersion ||
      version > k_mappingsFileVersion) {
    Log.warn("Invalid controller mapping version! (Expected {0}..{1}, found {2})", k_minMappingsFileVersion,
             k_mappingsFileVersion, version);
    return;
  }

  uint8_t isGameCubeValue = 0;
  Sint64 dataStart = 0;
  if (!SDL_ReadU8(file.get(), &isGameCubeValue) || !seek_aligned(file.get(), dataStart)) {
    Log.warn("Unable to read controller bindings header! Path: \"{}\"", aurora::io::fs_path_to_string(path));
    return;
  }
  const bool isGameCube = isGameCubeValue != 0;
  if (isGameCube) {
    if (playerIndex >= PAD_CHANMAX) {
      return;
    }
    constexpr uint32_t dzSecLen = sizeof(PADDeadZones);
    constexpr uint32_t btnSecLen = sizeof(PADButtonMapping) * PAD_BUTTON_COUNT;
    constexpr uint32_t axisSecLen = sizeof(PADAxisMapping) * PAD_AXIS_COUNT;
    const Sint64 portOffset = dataStart + (dzSecLen + btnSecLen + axisSecLen) * playerIndex;
    if (SDL_SeekIO(file.get(), portOffset, SDL_IO_SEEK_SET) != portOffset) {
      Log.warn("Unable to seek in controller bindings! Path: \"{}\"", aurora::io::fs_path_to_string(path));
      return;
    }
  }

  auto deadZones = controller->m_deadZones;
  auto buttonMapping = controller->m_buttonMapping;
  auto axisMapping = controller->m_axisMapping;
  auto rumbleIntensityLow = controller->m_rumbleIntensityLow;
  auto rumbleIntensityHigh = controller->m_rumbleIntensityHigh;
  auto forceDeviceRumble = controller->m_forceDeviceRumble;
  bool ok = aurora::io::read_exact(file.get(), &deadZones, sizeof(deadZones)) &&
            aurora::io::read_exact(file.get(), buttonMapping.data(), sizeof(buttonMapping)) &&
            aurora::io::read_exact(file.get(), axisMapping.data(), sizeof(axisMapping));
  if (!isGameCube) {
    ok = ok && SDL_ReadU16LE(file.get(), &rumbleIntensityLow) && SDL_ReadU16LE(file.get(), &rumbleIntensityHigh);
    if (version >= k_mappingsFileVersion) {
      uint8_t forceDeviceRumbleValue = 0;
      ok = ok && SDL_ReadU8(file.get(), &forceDeviceRumbleValue);
      forceDeviceRumble = forceDeviceRumbleValue != 0;
    }
  }
  if (!ok) {
    Log.warn("Unable to read controller bindings! Path: \"{}\"", aurora::io::fs_path_to_string(path));
    return;
  }
  controller->m_deadZones = deadZones;
  controller->m_buttonMapping = buttonMapping;
  controller->m_axisMapping = axisMapping;
  controller->m_rumbleIntensityLow = rumbleIntensityLow;
  controller->m_rumbleIntensityHigh = rumbleIntensityHigh;
  controller->m_forceDeviceRumble = forceDeviceRumble;

  bool axisCorrupt = false;
  for (uint32_t i = 0; i < PAD_AXIS_COUNT; ++i) {
    if (controller->m_axisMapping[i].padAxis != static_cast<PADAxis>(i)) {
      axisCorrupt = true;
      break;
    }
  }
  if (axisCorrupt) {
    Log.warn("__PADLoadMapping port={}: corrupt axis data in file, resetting axes to defaults", playerIndex);
    controller->m_axisMapping = g_defaultAxes;
  }

  bool buttonCorrupt = false;
  for (uint32_t i = 0; i < PAD_BUTTON_COUNT; ++i) {
    if (controller->m_buttonMapping[i].padButton == 0) {
      buttonCorrupt = true;
      break;
    }
  }
  if (buttonCorrupt) {
    Log.warn("__PADLoadMapping port={}: corrupt button data in file, resetting buttons to defaults", playerIndex);
    __PADSetDefaultMapping(controller);
  }
}

static void EnsureMappingLoaded(aurora::input::GameController* controller) {
  if (!controller->m_mappingLoaded) {
    __PADLoadMapping(controller);
  }
}

static Sint16 _get_axis_value(const aurora::input::GameController* controller, //  NOLINT(*-reserved-identifier)
                              PADAxis axis) {
  const auto iter =
      std::ranges::find_if(controller->m_axisMapping, [axis](const auto& pair) { return pair.padAxis == axis; });
  if (iter == controller->m_axisMapping.end()) {
    return 0;
  }

  if (iter->nativeAxis.nativeAxis != -1) {
    const auto [nativeAxis, sign] = iter->nativeAxis;
    if(nativeAxis<0 || nativeAxis>=SDL_GAMEPAD_AXIS_COUNT ||
       (sign!=AXIS_SIGN_POSITIVE && sign!=AXIS_SIGN_NEGATIVE)) return 0;
    // clamp value to avoid overflow when casting to Sint16 if -32768 is negated
    return static_cast<Sint16>(
        std::min(SDL_GetGamepadAxis(controller->m_controller, static_cast<SDL_GamepadAxis>(nativeAxis)) * sign,
                 SDL_JOYSTICK_AXIS_MAX));
  }

  if(iter->nativeButton<0 || iter->nativeButton>=SDL_GAMEPAD_BUTTON_COUNT) return 0;
  if (SDL_GetGamepadButton(controller->m_controller, static_cast<SDL_GamepadButton>(iter->nativeButton))) {
    return SDL_JOYSTICK_AXIS_MAX;
  }
  return 0;
}

static void neutralize_status(PADStatus& status) {
  status.button = 0;
  status.extButton = 0;
  status.stickX = 0;
  status.stickY = 0;
  status.substickX = 0;
  status.substickY = 0;
  status.triggerLeft = 0;
  status.triggerRight = 0;
  status.analogA = 0;
  status.analogB = 0;
}

static void apply_unblock_suppression(PADStatus& status, const u32 port, const bool captureHeldInput) {
  s8* sticks[] = {&status.stickX, &status.stickY, &status.substickX, &status.substickY};
  for (unsigned i = 0; i < 4; ++i) {
    if (captureHeldInput && std::abs(int(*sticks[i])) > 15) g_suppressedSticks[port][i] = true;
    if (g_suppressedSticks[port][i]) {
      if (std::abs(int(*sticks[i])) <= 15) g_suppressedSticks[port][i] = false;
      else *sticks[i] = 0;
    }
  }
  if (captureHeldInput) {
    g_suppressedButtons[port] |= status.button;
    g_suppressedExtButtons[port] |= status.extButton;
    g_suppressLeftTrigger[port] = g_suppressLeftTrigger[port] || status.triggerLeft > ClampRegion.minTrigger;
    g_suppressRightTrigger[port] = g_suppressRightTrigger[port] || status.triggerRight > ClampRegion.minTrigger;
  }

  g_suppressedButtons[port] &= status.button;
  status.button &= ~g_suppressedButtons[port];
  g_suppressedExtButtons[port] &= status.extButton;
  status.extButton &= ~g_suppressedExtButtons[port];

  if (g_suppressLeftTrigger[port]) {
    if (status.triggerLeft <= ClampRegion.minTrigger) {
      g_suppressLeftTrigger[port] = false;
    } else {
      status.triggerLeft = 0;
    }
  }

  if (g_suppressRightTrigger[port]) {
    if (status.triggerRight <= ClampRegion.minTrigger) {
      g_suppressRightTrigger[port] = false;
    } else {
      status.triggerRight = 0;
    }
  }
}

static int dominant_axis_value(const int physical, const int virtualValue, const int min, const int max) {
  return aurora::input::dominant_pad_axis(physical,virtualValue,min,max);
}

static void merge_virtual_status(PADStatus& status, const PADStatus& virtualStatus) {
  status.button |= virtualStatus.button;
  status.extButton |= virtualStatus.extButton;
  status.stickX = static_cast<s8>(dominant_axis_value(status.stickX, virtualStatus.stickX, -127, 127));
  status.stickY = static_cast<s8>(dominant_axis_value(status.stickY, virtualStatus.stickY, -127, 127));
  status.substickX = static_cast<s8>(dominant_axis_value(status.substickX, virtualStatus.substickX, -127, 127));
  status.substickY = static_cast<s8>(dominant_axis_value(status.substickY, virtualStatus.substickY, -127, 127));
  status.triggerLeft = std::max(status.triggerLeft, virtualStatus.triggerLeft);
  status.triggerRight = std::max(status.triggerRight, virtualStatus.triggerRight);
  status.analogA = std::max(status.analogA, virtualStatus.analogA);
  status.analogB = std::max(status.analogB, virtualStatus.analogB);
}

u32 PADRead(PADStatus* status) {
  if (!g_initialized) {
    Log.fatal("PADRead called before PADInit()!");
  }

  static const auto diagnosticIsolation = aurora::input::PadDiagnosticIsolation::configured(
      std::getenv("AURORA_DIAGNOSTIC_SDL_GAMEPAD") != nullptr,
      std::getenv("AURORA_DIAGNOSTIC_INPUT_ISOLATION") != nullptr);
  if (diagnosticIsolation.enabled) {
    static bool logged = false;
    if (!logged) {
      std::fprintf(stderr,"NATIVE_DIAGNOSTIC_INPUT_ISOLATION enabled=1 keyboard=blocked hardware=owned-sdl-only other_virtual=retained normal_play_changed=0\n");
      logged = true;
    }
  }

  int numKeys = 0;
  const bool* kbState = SDL_GetKeyboardState(&numKeys);
  const bool captureHeldInput = g_suppressHeldOnRead && !g_blockPAD && !g_controlsEditing && !g_bindingCapture.active && !g_nativeControlsPage && g_windowFocused;
  auto keyPresses=g_pendingKeyPresses.take();
  if(captureHeldInput)keyPresses.reset();
  g_suppressHeldOnRead = false;

  uint32_t rumbleSupport = 0;
  for (uint32_t i = 0; i < PAD_CHANMAX; ++i) {
    memset(&status[i], 0, sizeof(PADStatus));
    // Support device rumble on port 0 regardless of whether a controller is connected.
    if (device_rumble_available_for_port(i)) {
      rumbleSupport |= PAD_CHAN0_BIT;
    }
    auto controller = aurora::input::get_controller_for_player(i);
    if (controller == nullptr && !g_keyboardBindings[i].m_mappingsSet && !g_virtualPadActive[i]) {
      status[i].err = PAD_ERR_NO_CONTROLLER;
      g_suppressedButtons[i] = 0;
      g_suppressLeftTrigger[i] = false;
      g_suppressRightTrigger[i] = false;
      if (i == 1 && std::getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT")) {
        static unsigned missingReads = 0;
        if (missingReads < 12000 && missingReads % 30 == 0) {
          std::fprintf(stderr,
                       "NATIVE_KIRBY_MEWTWO_PAD_MISSING read=%u port=1 err=%d virtual=0\n",
                       missingReads, int(status[i].err));
        }
        if (missingReads < 12000) ++missingReads;
      }
      continue;
    }

    status[i].err = PAD_ERR_NONE;
    if (g_keyboardBindings[i].m_mappingsSet && diagnosticIsolation.accepts_keyboard()) {
      std::ranges::for_each(
          g_keyboardBindings[i].m_buttonMapping, [&kbState, &numKeys, &i, &status, &keyPresses](const PADKeyButtonBinding& mapping) {
            if (aurora::input::keyboard_sample_pressed(kbState,numKeys,mapping.scancode,keyPresses)) {
              status[i].button |= mapping.padButton;
            } else if (is_mouse_scancode(mapping.scancode) && is_mouse_button_pressed(mapping.scancode)) {
              status[i].button |= mapping.padButton;
            }
          });

      int lx = 0, ly = 0, rx = 0, ry = 0, tl = 0, tr = 0;
      for (const auto& binding : g_keyboardBindings[i].m_axisMapping) {
        bool pressed = false;
        if (binding.scancode > PAD_KEY_INVALID) {
          pressed = aurora::input::keyboard_sample_pressed(kbState,numKeys,binding.scancode,keyPresses);
        } else if (is_mouse_scancode(binding.scancode)) {
          pressed = is_mouse_button_pressed(binding.scancode);
        }
        if (!pressed) {
          continue;
        }
        switch (binding.padAxis) {
        case PAD_AXIS_LEFT_X_POS:
          lx += 127;
          break;
        case PAD_AXIS_LEFT_X_NEG:
          lx -= 127;
          break;
        case PAD_AXIS_LEFT_Y_POS:
          ly += 127;
          break;
        case PAD_AXIS_LEFT_Y_NEG:
          ly -= 127;
          break;
        case PAD_AXIS_RIGHT_X_POS:
          rx += 127;
          break;
        case PAD_AXIS_RIGHT_X_NEG:
          rx -= 127;
          break;
        case PAD_AXIS_RIGHT_Y_POS:
          ry += 127;
          break;
        case PAD_AXIS_RIGHT_Y_NEG:
          ry -= 127;
          break;
        case PAD_AXIS_TRIGGER_L:
          tl += 255;
          break;
        case PAD_AXIS_TRIGGER_R:
          tr += 255;
          break;
        default:
          break;
        }
      }
      status[i].stickX = static_cast<s8>(std::clamp(static_cast<int>(status[i].stickX) + lx, -127, 127));
      status[i].stickY = static_cast<s8>(std::clamp(static_cast<int>(status[i].stickY) + ly, -127, 127));
      status[i].substickX = static_cast<s8>(std::clamp(static_cast<int>(status[i].substickX) + rx, -127, 127));
      status[i].substickY = static_cast<s8>(std::clamp(static_cast<int>(status[i].substickY) + ry, -127, 127));
      status[i].triggerLeft = static_cast<u8>(std::min(static_cast<int>(status[i].triggerLeft) + tl, 255));
      status[i].triggerRight = static_cast<u8>(std::min(static_cast<int>(status[i].triggerRight) + tr, 255));
    }

    if (controller) {
      EnsureMappingLoaded(controller);
      bool leftTriggerSet = false;
      bool rightTriggerSet = false;
      std::ranges::for_each(controller->m_buttonMapping, [&controller, &i, &status, &leftTriggerSet,
                                                          &rightTriggerSet](const auto& mapping) {
        if (SDL_GetGamepadButton(controller->m_controller, static_cast<SDL_GamepadButton>(mapping.nativeButton))) {
          status[i].button |= mapping.padButton;
        }

        if (mapping.padButton == PAD_TRIGGER_L && mapping.nativeButton != PAD_NATIVE_BUTTON_INVALID) {
          leftTriggerSet = true;
        }
        if (mapping.padButton == PAD_TRIGGER_R && mapping.nativeButton != PAD_NATIVE_BUTTON_INVALID) {
          rightTriggerSet = true;
        }
      });

      // TODO: Add serializable mappings for these (probably not necessary)?
      static constexpr std::array<std::pair<SDL_GamepadButton, PADExtButton>, PAD_EXT_BUTTON_COUNT> kExtButtonMappings{{
          {SDL_GAMEPAD_BUTTON_BACK, PAD_BUTTON_BACK},
          {SDL_GAMEPAD_BUTTON_GUIDE, PAD_BUTTON_GUIDE},
          {SDL_GAMEPAD_BUTTON_MISC1, PAD_BUTTON_MISC1},
          {SDL_GAMEPAD_BUTTON_MISC2, PAD_BUTTON_MISC2},
          {SDL_GAMEPAD_BUTTON_MISC3, PAD_BUTTON_MISC3},
          {SDL_GAMEPAD_BUTTON_MISC4, PAD_BUTTON_MISC4},
          {SDL_GAMEPAD_BUTTON_MISC5, PAD_BUTTON_MISC5},
          {SDL_GAMEPAD_BUTTON_MISC6, PAD_BUTTON_MISC6},
          {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, PAD_BUTTON_RIGHT_PADDLE1},
          {SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, PAD_BUTTON_LEFT_PADDLE1},
          {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, PAD_BUTTON_RIGHT_PADDLE2},
          {SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, PAD_BUTTON_LEFT_PADDLE2},
          {SDL_GAMEPAD_BUTTON_RIGHT_STICK, PAD_BUTTON_RIGHT_STICK},
          {SDL_GAMEPAD_BUTTON_LEFT_STICK, PAD_BUTTON_LEFT_STICK},
          {SDL_GAMEPAD_BUTTON_TOUCHPAD, PAD_BUTTON_TOUCHPAD},
      }};

      for (const auto& [native, button] : kExtButtonMappings) {
        if (SDL_GetGamepadButton(controller->m_controller, native)) {
          status[i].extButton |= button;
        }
      }

      const auto xlPos = _get_axis_value(controller, PAD_AXIS_LEFT_X_POS);
      const auto xlNeg = _get_axis_value(controller, PAD_AXIS_LEFT_X_NEG);
      const auto ylPos = _get_axis_value(controller, PAD_AXIS_LEFT_Y_POS);
      const auto ylNeg = _get_axis_value(controller, PAD_AXIS_LEFT_Y_NEG);

      auto xl = static_cast<Sint16>((xlPos + -xlNeg) / 2);
      // Default Y bindings already invert SDL direction. Keep scaling signed.
      auto yl = static_cast<Sint16>((ylPos - ylNeg) / 2);
      if (controller->m_deadZones.useDeadzones) {
        if (std::abs(xl) > controller->m_deadZones.stickDeadZone) {
          xl /= 256;
        } else {
          xl = 0;
        }
        if (std::abs(yl) > controller->m_deadZones.stickDeadZone) {
          yl /= 256;
        } else {
          yl = 0;
        }
      } else {
        xl /= 256;
        yl /= 256;
      }

      status[i].stickX = static_cast<int8_t>(dominant_axis_value(xl,status[i].stickX,-128,127));
      status[i].stickY = static_cast<int8_t>(dominant_axis_value(yl,status[i].stickY,-128,127));

      const auto xrPos = _get_axis_value(controller, PAD_AXIS_RIGHT_X_POS);
      const auto xrNeg = _get_axis_value(controller, PAD_AXIS_RIGHT_X_NEG);
      const auto yrPos = _get_axis_value(controller, PAD_AXIS_RIGHT_Y_POS);
      const auto yrNeg = _get_axis_value(controller, PAD_AXIS_RIGHT_Y_NEG);

      auto xr = static_cast<Sint16>((xrPos + -xrNeg) / 2);
      // Default Y bindings already invert SDL direction. Keep scaling signed.
      auto yr = static_cast<Sint16>((yrPos - yrNeg) / 2);
      if (controller->m_deadZones.useDeadzones) {
        if (std::abs(xr) > controller->m_deadZones.substickDeadZone) {
          xr /= 256;
        } else {
          xr = 0;
        }

        if (std::abs(yr) > controller->m_deadZones.substickDeadZone) {
          yr /= 256;
        } else {
          yr = 0;
        }
      } else {
        xr /= 256;
        yr /= 256;
      }

      status[i].substickX = static_cast<int8_t>(dominant_axis_value(xr,status[i].substickX,-128,127));
      status[i].substickY = static_cast<int8_t>(dominant_axis_value(yr,status[i].substickY,-128,127));

      Sint16 tl = std::max(static_cast<Sint16>(0), _get_axis_value(controller, PAD_AXIS_TRIGGER_L));
      Sint16 tr = std::max(static_cast<Sint16>(0), _get_axis_value(controller, PAD_AXIS_TRIGGER_R));

      if (controller->m_deadZones.emulateTriggers) {
        if (!leftTriggerSet && tl > controller->m_deadZones.leftTriggerActivationZone) {
          status[i].button |= PAD_TRIGGER_L;
        }
        if (!rightTriggerSet && tr > controller->m_deadZones.rightTriggerActivationZone) {
          status[i].button |= PAD_TRIGGER_R;
        }
      }
      tl /= 128;
      tr /= 128;

      status[i].triggerLeft = static_cast<u8>(std::max(int(status[i].triggerLeft),int(tl)));
      status[i].triggerRight = static_cast<u8>(std::max(int(status[i].triggerRight),int(tr)));

      // If the digital button is activated, set the analog value to max.
      if (status[i].button & PAD_TRIGGER_L) {
        status[i].triggerLeft = std::max<u8>(status[i].triggerLeft,180);
      }
      if (status[i].button & PAD_TRIGGER_R) {
        status[i].triggerRight = std::max<u8>(status[i].triggerRight,180);
      }

      if (controller->m_hasRumble) {
        rumbleSupport |= PAD_CHAN0_BIT >> i;
      }

      // Update the LED colors when they exist and the controller is read (which should happen once per frame in most
      // games)
      if (controller->m_hasRgbLed && controller->m_isColorDirty) {
        SDL_SetGamepadLED(controller->m_controller, controller->m_ledRed, controller->m_ledGreen,
                          controller->m_ledBlue);
        controller->m_isColorDirty = false;
      }
    }

    // Keep real controller discovery/mapping intact. Only an explicitly isolated
    // SDL diagnostic drops unrelated live input before merging staged other ports.
    if (diagnosticIsolation.enabled) {
      const auto instance = controller ? SDL_GetGamepadID(controller->m_controller) : 0;
      if (!diagnosticIsolation.accepts_controller(instance, g_diagnosticSdlInstance)) {
        const auto rejected = status[i];
        neutralize_status(status[i]);
        status[i].err = g_virtualPadActive[i] ? PAD_ERR_NONE : PAD_ERR_NO_CONTROLLER;
        static std::array<unsigned, PAD_CHANMAX> rejectedSamples{};
        if (controller && rejectedSamples[i] < 8) {
          ++rejectedSamples[i];
          std::fprintf(stderr,"NATIVE_DIAGNOSTIC_INPUT_REJECTED port=%u instance=%u owned_instance=%u raw_button=%u raw_x=%d raw_y=%d isolated_button=%u isolated_x=%d isolated_y=%d isolated_err=%d virtual_merge=%u\n",
                       i, unsigned(instance), unsigned(g_diagnosticSdlInstance), unsigned(rejected.button),
                       int(rejected.stickX), int(rejected.stickY), unsigned(status[i].button),
                       int(status[i].stickX), int(status[i].stickY), int(status[i].err), unsigned(g_virtualPadActive[i]));
        }
      }
    }

    const bool diagnosticVirtualGamepad =
        std::getenv("AURORA_DIAGNOSTIC_SDL_GAMEPAD") != nullptr;
    // Keep ordinary background-input suppression; only the opt-in diagnostic
    // virtual device may exercise PADRead while this test window is unfocused.
    if (g_blockPAD || g_controlsEditing || g_bindingCapture.active || g_nativeControlsPage || (!g_windowFocused && !diagnosticVirtualGamepad)) {
      neutralize_status(status[i]);
    } else {
      apply_unblock_suppression(status[i], i, captureHeldInput);
      if (g_virtualPadActive[i]) {
        merge_virtual_status(status[i], g_virtualPadStatus[i]);
      }
    }
#ifdef _WIN32
    if (i == PAD_CHAN0 && g_diagnosticPrizeTrace.load(std::memory_order_relaxed) &&
        diagnostic_prize_trace_now_ns() <= g_diagnosticPrizeTraceWindowEndNs.load(std::memory_order_relaxed)) {
      const unsigned seq = g_diagnosticPrizeTraceReads.fetch_add(1, std::memory_order_relaxed);
      if (seq < 20000) {
        std::fprintf(stderr,
                     "NATIVE_PRIZE_PAD_SAMPLE seq=%u time_ns=%lld virtual_active=%u virtual_button=%u sampled_button=%u focus=%u block=%u editing=%u\n",
                     seq, diagnostic_prize_trace_now_ns(), unsigned(g_virtualPadActive[i]),
                     unsigned(g_virtualPadStatus[i].button), unsigned(status[i].button),
                     unsigned(g_windowFocused), unsigned(g_blockPAD), unsigned(g_controlsEditing));
      }
    }
#endif
    // Opt-in readback only. Sequence counts PAD reads, not simulation ticks.
    if(i==0 && keyPresses.any() && key_tap_diagnostic())
      std::fprintf(stderr,"NATIVE_KEY_TAP_SAMPLE count=%zu button=%x stick=%d,%d substick=%d,%d return_held=%u focus=%u block=%u editing=%u\n",
          keyPresses.count(),unsigned(status[i].button),int(status[i].stickX),int(status[i].stickY),
          int(status[i].substickX),int(status[i].substickY),
          unsigned(aurora::input::keyboard_key_pressed(kbState,numKeys,SDL_SCANCODE_RETURN)),
          unsigned(g_windowFocused),unsigned(g_blockPAD),unsigned(g_controlsEditing));
    if (i == 1 && std::getenv("MELEE_DIAGNOSTIC_KIRBY_MEWTWO_COPY_INPUT")) {
      static unsigned readSequence = 0;
      if (readSequence < 12000 && readSequence % 30 == 0) {
        std::fprintf(stderr,
                     "NATIVE_KIRBY_MEWTWO_PAD_RAW read=%u port=1 virtual=%d requested_x=%d err=%d output_x=%d block=%d editing=%d focus=%d background_bypass=%d\n",
                     readSequence, int(g_virtualPadActive[i]),
                     int(g_virtualPadStatus[i].stickX), int(status[i].err),
                     int(status[i].stickX), int(g_blockPAD),
                     int(g_controlsEditing), int(g_windowFocused),
                     int(diagnosticVirtualGamepad));
      }
      if (readSequence < 12000) ++readSequence;
    }
  }
  return rumbleSupport;
}

// Opt-in developer route: exercise the actual SDL gamepad/PAD path instead
// of merging synthetic PADStatus. Port1 and other diagnostics are unaffected.
static void diagnostic_sdl_noise() {
  static const bool enabled = std::getenv("AURORA_DIAGNOSTIC_SDL_NOISE") != nullptr;
  static SDL_Joystick* joystick = nullptr;
  if (!enabled || joystick) return;
  SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
  desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=6;desc.nbuttons=15;
  desc.axis_mask=63;desc.button_mask=32767;desc.name="Melee unowned SDL input interference test";
  const auto id=SDL_AttachVirtualJoystick(&desc);joystick=SDL_OpenJoystick(id);
  AURORA_ASSERT(id && joystick,"SDL interference test device creation failed");
  AURORA_ASSERT(aurora::input::add_controller(id)==id,"SDL interference test discovery failed");
  aurora::input::set_player_index(id,3);
  AURORA_ASSERT(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_EAST,true),"SDL interference B button failed");
  AURORA_ASSERT(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_DPAD_UP,true),"SDL interference Up button failed");
  for(int axis=0;axis<6;++axis) {
    const Sint16 value=axis==0?32767:axis==1||axis>=4?-32768:0;
    AURORA_ASSERT(SDL_SetJoystickVirtualAxis(joystick,axis,value),"SDL interference axis failed");
  }
  std::fprintf(stderr,"NATIVE_SDL_NOISE port=3 assigned_player=%d instance=%u buttons=B+Up stick=full physical_acceptance=0\n",
               aurora::input::player_index(id),unsigned(id));
}

static void diagnostic_sdl_status(const PADStatus& value) {
  static SDL_Joystick* joystick=nullptr;
  static SDL_JoystickID id=0;
  if (!joystick) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=6;desc.nbuttons=15;
    desc.axis_mask=63;desc.button_mask=32767;desc.name="Melee original-game SDL input diagnostic";
    id=SDL_AttachVirtualJoystick(&desc);joystick=SDL_OpenJoystick(id);
    AURORA_ASSERT(id && joystick,"SDL diagnostic gamepad creation failed");
    AURORA_ASSERT(aurora::input::add_controller(id)==id,"SDL diagnostic discovery failed");
    aurora::input::set_player_index(id,0);
    g_diagnosticSdlInstance = id;
    std::fprintf(stderr,
                 "NATIVE_SDL_DEVICE port=0 assigned_player=%d controller_count=%u virtual_pad_merge=0 physical_acceptance=0\n",
                 aurora::input::player_index(id), aurora::input::controller_count());
  }
  const int axes[]={value.stickX*256,-value.stickY*256,value.substickX*256,-value.substickY*256,
    (value.button&PAD_TRIGGER_L)?32767:int(value.triggerLeft)*257-32768,
    (value.button&PAD_TRIGGER_R)?32767:int(value.triggerRight)*257-32768};
  for(int axis=0;axis<6;++axis)AURORA_ASSERT(SDL_SetJoystickVirtualAxis(joystick,axis,static_cast<Sint16>(std::clamp(axes[axis],-32768,32767))),"SDL diagnostic axis failed");
  for(const auto& mapping:g_defaultButtonsStandard) {
    if(mapping.nativeButton<15)AURORA_ASSERT(SDL_SetJoystickVirtualButton(joystick,mapping.nativeButton,(value.button&mapping.padButton)!=0),"SDL diagnostic button failed");
  }
  diagnostic_sdl_noise();
  SDL_UpdateJoysticks();SDL_UpdateGamepads();
}

void PADSetVirtualStatus(const u32 port, const PADStatus* virtualStatus) {
  if (port >= PAD_CHANMAX || virtualStatus == nullptr) {
    return;
  }

  if(port==0 && std::getenv("AURORA_DIAGNOSTIC_SDL_GAMEPAD")) {
    diagnostic_sdl_status(*virtualStatus);g_virtualPadActive[port]=false;return;
  }
  g_virtualPadStatus[port] = *virtualStatus;
  g_virtualPadStatus[port].err = PAD_ERR_NONE;
  g_virtualPadActive[port] = true;
#ifdef _WIN32
  if (port == PAD_CHAN0 && g_diagnosticPrizeTrace.load(std::memory_order_relaxed)) {
    const bool pressed = (virtualStatus->button & PAD_BUTTON_A) != 0;
    const bool previous = g_diagnosticPrizeTraceLastA.exchange(pressed, std::memory_order_relaxed);
    if (pressed != previous) {
      const long long now = diagnostic_prize_trace_now_ns();
      g_diagnosticPrizeTraceWindowEndNs.store(now + 250000000, std::memory_order_relaxed);
      const unsigned seq = g_diagnosticPrizeTraceWrites.fetch_add(1, std::memory_order_relaxed);
      if (seq < 4096)
        std::fprintf(stderr, "NATIVE_PRIZE_PAD_INJECT seq=%u time_ns=%lld pressed=%u button=%u\n",
                     seq, now, unsigned(pressed), unsigned(virtualStatus->button));
    }
  }
#endif
}

void PADSetDiagnosticPrizeTrace(bool enabled) {
  if (enabled) {
    g_diagnosticPrizeTraceWrites.store(0, std::memory_order_relaxed);
    g_diagnosticPrizeTraceReads.store(0, std::memory_order_relaxed);
    g_diagnosticPrizeTraceLastA.store(false, std::memory_order_relaxed);
    g_diagnosticPrizeTraceWindowEndNs.store(0, std::memory_order_relaxed);
  }
  g_diagnosticPrizeTrace.store(enabled, std::memory_order_release);
  std::fprintf(stderr,
               "NATIVE_PRIZE_PAD_TRACE enabled=%u writes=%u samples=%u\n",
               unsigned(enabled), g_diagnosticPrizeTraceWrites.load(std::memory_order_relaxed),
               g_diagnosticPrizeTraceReads.load(std::memory_order_relaxed));
}

bool PADDiagnosticPrizeTraceActive(void) {
  if (!g_diagnosticPrizeTrace.load(std::memory_order_acquire)) return false;
  return diagnostic_prize_trace_now_ns() <=
         g_diagnosticPrizeTraceWindowEndNs.load(std::memory_order_relaxed);
}

void PADClearVirtualStatus(const u32 port) {
  if (port >= PAD_CHANMAX) {
    return;
  }

  if(port==0 && std::getenv("AURORA_DIAGNOSTIC_SDL_GAMEPAD")) diagnostic_sdl_status(PADStatus{});
  g_virtualPadStatus[port] = {};
  g_virtualPadActive[port] = false;
}

void PADClearAllVirtualStatus() {
  g_virtualPadStatus.fill({});
  g_virtualPadActive.fill(false);
}

void PADControlMotor(const u32 chan, const u32 cmd) {
  const auto controller = aurora::input::get_controller_for_player(chan);
  if (should_use_device_rumble(chan, controller)) {
    u16 low = 0;
    u16 high = 0;
    if (controller != nullptr) {
      EnsureMappingLoaded(controller);
      low = controller->m_rumbleIntensityLow;
      high = controller->m_rumbleIntensityHigh;
    } else {
      aurora::input::get_device_rumble_intensity(&low, &high);
    }
    if (cmd == PAD_MOTOR_STOP || cmd == PAD_MOTOR_STOP_HARD) {
      aurora::device::rumble(0, 0, 0);
    } else if (cmd == PAD_MOTOR_RUMBLE) {
      aurora::device::rumble(low, high, 0);
    }
    return;
  }

  if (controller == nullptr) {
    return;
  }

  const auto instance = aurora::input::get_instance_for_player(chan);
  if (controller->m_isGameCube) {
    if (cmd == PAD_MOTOR_STOP) {
      aurora::input::controller_rumble(instance, 0, 1, 0);
    } else if (cmd == PAD_MOTOR_RUMBLE) {
      aurora::input::controller_rumble(instance, 1, 1, 0);
    } else if (cmd == PAD_MOTOR_STOP_HARD) {
      aurora::input::controller_rumble(instance, 0, 0, 0);
    }
  } else {
    if (cmd == PAD_MOTOR_STOP) {
      aurora::input::controller_rumble(instance, 0, 0, 1);
    } else if (cmd == PAD_MOTOR_RUMBLE) {
      aurora::input::controller_rumble(instance, controller->m_rumbleIntensityLow, controller->m_rumbleIntensityHigh,
                                       0);
    } else if (cmd == PAD_MOTOR_STOP_HARD) {
      aurora::input::controller_rumble(instance, 0, 0, 0);
    }
  }
}

void PADControlAllMotors(const u32* cmdArr) {
  for (u32 i = 0; i < PAD_CHANMAX; ++i) {
    PADControlMotor(i, cmdArr[i]);
  }
}

void ClampTrigger(u8* trigger, const u8 min, const u8 max) {
  if (*trigger <= min) {
    *trigger = 0;
  } else {
    if (*trigger > max) {
      *trigger = max;
    }
    *trigger -= min;
  }
}

void ClampCircle(s8* px, s8* py, const s8 radius, const s8 min) {
  int x = *px; // NOLINT(*-str34-c)
  int y = *py; // NOLINT(*-str34-c)

  if (-min < x && x < min) {
    x = 0;
  } else if (0 < x) {
    x -= min;
  } else {
    x += min;
  }

  if (-min < y && y < min) {
    y = 0;
  } else if (0 < y) {
    y -= min;
  } else {
    y += min;
  }

  if (const int squared = x * x + y * y; radius * radius < squared) {
    const auto length = static_cast<int32_t>(std::sqrt(squared));
    x = x * radius / length;
    y = y * radius / length;
  }

  *px = static_cast<int8_t>(x);
  *py = static_cast<int8_t>(y);
}

void ClampStick(s8* px, s8* py, const s8 max, const s8 xy, const s8 min) {
  int32_t x = *px; // NOLINT(*-str34-c)
  int32_t y = *py; // NOLINT(*-str34-c)

  int32_t signX = 0;
  if (0 <= x) {
    signX = 1;
  } else {
    signX = -1;
    x = -x;
  }

  int8_t signY = 0;
  if (0 <= y) {
    signY = 1;
  } else {
    signY = -1;
    y = -y;
  }

  if (x <= min) {
    x = 0;
  } else {
    x -= min;
  }
  if (y <= min) {
    y = 0;
  } else {
    y -= min;
  }

  if (x == 0 && y == 0) {
    *px = *py = 0;
    return;
  }

  x = x * max / (INT8_MAX - min);
  y = y * max / (INT8_MAX - min);

  if (xy * y <= xy * x) {
    if (const int32_t d = xy * x + (max - xy) * y; xy * max < d) {
      x = xy * max * x / d;
      y = xy * max * y / d;
    }
  } else {
    if (const int32_t d = xy * y + (max - xy) * x; xy * max < d) {
      x = xy * max * x / d;
      y = xy * max * y / d;
    }
  }

  *px = static_cast<s8>(signX * x);
  *py = static_cast<s8>(signY * y);
}

void PADClamp(PADStatus* status) {
  for (uint32_t i = 0; i < PAD_CHANMAX; ++i) {
    if (status[i].err != PAD_ERR_NONE) {
      continue;
    }

    ClampStick(&status[i].stickX, &status[i].stickY, ClampRegion.maxStick, ClampRegion.xyStick, ClampRegion.minStick);
    ClampStick(&status[i].substickX, &status[i].substickY, ClampRegion.maxSubstick, ClampRegion.xySubstick,
               ClampRegion.minSubstick);
    ClampTrigger(&status[i].triggerLeft, ClampRegion.minTrigger, ClampRegion.maxTrigger);
    ClampTrigger(&status[i].triggerRight, ClampRegion.minTrigger, ClampRegion.maxTrigger);
  }
}

void PADClampCircle(PADStatus* status) {
  for (uint32_t i = 0; i < PAD_CHANMAX; ++i) {
    if (status[i].err != PAD_ERR_NONE) {
      continue;
    }

    ClampCircle(&status[i].stickX, &status[i].stickY, ClampRegion.radStick, ClampRegion.minStick);
    ClampCircle(&status[i].substickX, &status[i].substickY, ClampRegion.radSubstick, ClampRegion.minSubstick);
    ClampTrigger(&status[i].triggerLeft, ClampRegion.minTrigger, ClampRegion.maxTrigger);
    ClampTrigger(&status[i].triggerRight, ClampRegion.minTrigger, ClampRegion.maxTrigger);
  }
}

void PADGetVidPid(const u32 port, u32* vid, u32* pid) {
  *vid = 0;
  *pid = 0;
  const auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return;
  }

  *vid = controller->m_vid;
  *pid = controller->m_pid;
}

const char* PADGetName(const u32 port) {
  const auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return nullptr;
  }

  return SDL_GetGamepadName(controller->m_controller);
}

BOOL PADTrySetButtonMapping(const u32 port, const PADButtonMapping mapping) {
  if (port >= PAD_CHANMAX) return FALSE;
  auto* controller = aurora::input::get_controller_for_player(port);
  if (!controller) return FALSE;
  if (mapping.nativeButton != SDL_GAMEPAD_BUTTON_INVALID &&
      (mapping.nativeButton < 0 || mapping.nativeButton >= SDL_GAMEPAD_BUTTON_COUNT ||
       !SDL_GamepadHasButton(controller->m_controller, static_cast<SDL_GamepadButton>(mapping.nativeButton))))
    return FALSE;
  EnsureMappingLoaded(controller);
  const auto iter = std::ranges::find_if(controller->m_buttonMapping,
      [mapping](const auto& item) { return item.padButton == mapping.padButton; });
  if (iter == controller->m_buttonMapping.end()) return FALSE;
  *iter = mapping;
  return TRUE;
}

void PADSetButtonMapping(const u32 port, const PADButtonMapping mapping) {
  auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return;
  }

  const auto iter = std::ranges::find_if(controller->m_buttonMapping,
                                         [mapping](const auto& pair) { return mapping.padButton == pair.padButton; });
  if (iter == controller->m_buttonMapping.end()) {
    return;
  }

  *iter = mapping;
}

void PADSetAllButtonMappings(const u32 port, const PADButtonMapping buttons[PAD_BUTTON_COUNT]) {
  for (uint32_t i = 0; i < PAD_BUTTON_COUNT; ++i) {
    PADSetButtonMapping(port, buttons[i]);
  }
}

PADButtonMapping* PADGetButtonMappings(const u32 port, u32* buttonCount) {
  auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    *buttonCount = 0;
    return nullptr;
  }

  EnsureMappingLoaded(controller);

  *buttonCount = PAD_BUTTON_COUNT;
  return controller->m_buttonMapping.data();
}

BOOL PADTrySetAxisMapping(const u32 port,const PADAxisMapping mapping) {
  if(port>=PAD_CHANMAX || mapping.padAxis>=PAD_AXIS_COUNT) return FALSE;
  auto* controller=aurora::input::get_controller_for_player(port);
  if(!controller) return FALSE;
  const auto source=mapping.nativeAxis;
  if(source.sign!=AXIS_SIGN_POSITIVE && source.sign!=AXIS_SIGN_NEGATIVE) return FALSE;
  if(source.nativeAxis==-1) {
    if(mapping.nativeButton!=-1 &&
       (mapping.nativeButton<0 || mapping.nativeButton>=SDL_GAMEPAD_BUTTON_COUNT ||
        !SDL_GamepadHasButton(controller->m_controller,static_cast<SDL_GamepadButton>(mapping.nativeButton)))) return FALSE;
  } else if(source.nativeAxis<0 || source.nativeAxis>=SDL_GAMEPAD_AXIS_COUNT ||
            mapping.nativeButton!=-1 ||
            !SDL_GamepadHasAxis(controller->m_controller,static_cast<SDL_GamepadAxis>(source.nativeAxis))) return FALSE;
  EnsureMappingLoaded(controller);
  const auto iter=std::ranges::find_if(controller->m_axisMapping,
    [mapping](const auto& item){return item.padAxis==mapping.padAxis;});
  if(iter==controller->m_axisMapping.end()) return FALSE;
  *iter=mapping;return TRUE;
}

void PADSetAxisMapping(const u32 port, const PADAxisMapping mapping) {
  auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return;
  }

  const auto iter = std::ranges::find_if(controller->m_axisMapping,
                                         [mapping](const auto& pair) { return mapping.padAxis == pair.padAxis; });
  if (iter == controller->m_axisMapping.end()) {
    return;
  }

  *iter = mapping;
}

void PADSetAllAxisMappings(const u32 port, const PADAxisMapping axes[PAD_AXIS_COUNT]) {
  for (uint32_t i = 0; i < PAD_AXIS_COUNT; ++i) {
    PADSetAxisMapping(port, axes[i]);
  }
}

PADAxisMapping* PADGetAxisMappings(const u32 port, u32* axisCount) {
  auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    *axisCount = 0;
    return nullptr;
  }

  EnsureMappingLoaded(controller);
  *axisCount = PAD_AXIS_COUNT;
  return controller->m_axisMapping.data();
}

static bool valid_edit_scancode(int scancode) {
  if(scancode>=PAD_KEY_MOUSE_X2 && scancode<=PAD_KEY_INVALID) return true;
  int count=0;const bool* states=SDL_GetKeyboardState(&count);
  if(!states || scancode<=0 || scancode>=count) return false;
  const char* name=SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
  return name && *name;
}
BOOL PADTrySetKeyboardProfile(const u32 port,const PADKeyButtonBinding* buttons,const PADKeyAxisBinding* axes) {
  if(port>=PAD_MAX_CONTROLLERS || !buttons || !axes) return FALSE;
  auto candidate=g_keyboardBindings[port];
  for(unsigned i=0;i<PAD_BUTTON_COUNT;++i) {
    if(!valid_edit_scancode(buttons[i].scancode)) return FALSE;
    for(unsigned j=0;j<i;++j)if(buttons[j].padButton==buttons[i].padButton)return FALSE;
    auto found=std::ranges::find_if(candidate.m_buttonMapping,[&](const auto& item){return item.padButton==buttons[i].padButton;});
    if(found==candidate.m_buttonMapping.end())return FALSE;
    *found=buttons[i];
  }
  for(unsigned i=0;i<PAD_AXIS_COUNT;++i) {
    if(!valid_edit_scancode(axes[i].scancode) ||
       (axes[i].scancode!=PAD_KEY_INVALID && axes[i].influence!=100))return FALSE;
    for(unsigned j=0;j<i;++j)if(axes[j].padAxis==axes[i].padAxis)return FALSE;
    auto found=std::ranges::find_if(candidate.m_axisMapping,[&](const auto& item){return item.padAxis==axes[i].padAxis;});
    if(found==candidate.m_axisMapping.end())return FALSE;
    *found=axes[i];
  }
  g_keyboardBindings[port]=candidate;return TRUE;
}

BOOL PADTrySetKeyButtonBinding(const u32 port,const PADKeyButtonBinding binding) {
  if(!valid_edit_scancode(binding.scancode)) return FALSE;
  return PADSetKeyButtonBinding(port,binding);
}
BOOL PADTrySetKeyAxisBinding(const u32 port,const PADKeyAxisBinding binding) {
  if(!valid_edit_scancode(binding.scancode) ||
     (binding.scancode!=PAD_KEY_INVALID && binding.influence!=100)) return FALSE;
  return PADSetKeyAxisBinding(port,binding);
}

BOOL PADSetKeyButtonBinding(const u32 port, const PADKeyButtonBinding binding) {
  if (port >= PAD_MAX_CONTROLLERS) {
    return FALSE;
  }

  for (auto& state = g_keyboardBindings[port]; auto& [scancode, padButton] : state.m_buttonMapping) {
    if (padButton == binding.padButton) {
      scancode = binding.scancode;
      g_pendingKeyPresses.clear();
      return TRUE;
    }
  }

  return FALSE;
}

BOOL PADSetKeyButtonBindings(const u32 port, PADKeyButtonBinding bindings[PAD_BUTTON_COUNT]) {
  if(port>=PAD_MAX_CONTROLLERS || !bindings) return FALSE;
  auto candidate=g_keyboardBindings[port].m_buttonMapping;
  for(uint32_t i=0;i<PAD_BUTTON_COUNT;++i) {
    for(uint32_t j=0;j<i;++j)if(bindings[j].padButton==bindings[i].padButton)return FALSE;
    auto found=std::ranges::find_if(candidate,[&](const auto& item){return item.padButton==bindings[i].padButton;});
    if(found==candidate.end())return FALSE;
    found->scancode=bindings[i].scancode;
  }
  g_pendingKeyPresses.clear();
  g_keyboardBindings[port].m_buttonMapping=candidate;return TRUE;
}

PADKeyButtonBinding* PADGetKeyButtonBindings(const u32 port, u32* buttonCount) {
  if (port >= PAD_MAX_CONTROLLERS || !g_keyboardBindings[port].m_mappingsSet) {
    return nullptr;
  }
  auto& state = g_keyboardBindings[port];
  *buttonCount = PAD_BUTTON_COUNT;
  return state.m_buttonMapping.data();
}

BOOL PADSetKeyAxisBinding(const u32 port, const PADKeyAxisBinding binding) {
  if (port >= PAD_MAX_CONTROLLERS) {
    return FALSE;
  }

  for (auto& state = g_keyboardBindings[port]; auto& b : state.m_axisMapping) {
    if (b.padAxis == binding.padAxis) {
      b.scancode = binding.scancode;
      b.influence = binding.influence;
      return TRUE;
    }
  }

  return FALSE;
}
BOOL PADSetKeyAxisBindings(const u32 port, PADKeyAxisBinding bindings[PAD_AXIS_COUNT]) {
  if(port>=PAD_MAX_CONTROLLERS || !bindings) return FALSE;
  auto candidate=g_keyboardBindings[port].m_axisMapping;
  for(uint32_t i=0;i<PAD_AXIS_COUNT;++i) {
    for(uint32_t j=0;j<i;++j)if(bindings[j].padAxis==bindings[i].padAxis)return FALSE;
    auto found=std::ranges::find_if(candidate,[&](const auto& item){return item.padAxis==bindings[i].padAxis;});
    if(found==candidate.end())return FALSE;
    found->scancode=bindings[i].scancode;
    found->influence=bindings[i].influence;
  }
  g_keyboardBindings[port].m_axisMapping=candidate;return TRUE;
}

PADKeyAxisBinding* PADGetKeyAxisBindings(const u32 port, u32* axisCount) {
  if (port >= PAD_MAX_CONTROLLERS || !g_keyboardBindings[port].m_mappingsSet) {
    return nullptr;
  }
  auto& state = g_keyboardBindings[port];
  *axisCount = PAD_AXIS_COUNT;
  return state.m_axisMapping.data();
}

void PADSetKeyboardActive(const u32 port, const BOOL active) {
  if (port >= PAD_MAX_CONTROLLERS) {
    return;
  }
  g_pendingKeyPresses.clear();
  g_keyboardBindings[port].m_mappingsSet = active != FALSE;
}

BOOL PADInstallKeyboardPreset(const u32 port) {
  if (port >= PAD_CHANMAX || !PADInit()) return FALSE;
  PADKeyButtonBinding buttons[PAD_BUTTON_COUNT] = {
      {SDL_SCANCODE_J, PAD_BUTTON_A}, {SDL_SCANCODE_K, PAD_BUTTON_B},
      {SDL_SCANCODE_U, PAD_BUTTON_X}, {SDL_SCANCODE_I, PAD_BUTTON_Y},
      {SDL_SCANCODE_RETURN, PAD_BUTTON_START}, {SDL_SCANCODE_O, PAD_TRIGGER_Z},
      {SDL_SCANCODE_Q, PAD_TRIGGER_L}, {SDL_SCANCODE_E, PAD_TRIGGER_R},
      {SDL_SCANCODE_UP, PAD_BUTTON_UP}, {SDL_SCANCODE_DOWN, PAD_BUTTON_DOWN},
      {SDL_SCANCODE_LEFT, PAD_BUTTON_LEFT}, {SDL_SCANCODE_RIGHT, PAD_BUTTON_RIGHT}};
  PADKeyAxisBinding axes[PAD_AXIS_COUNT] = {
      {SDL_SCANCODE_D, PAD_AXIS_LEFT_X_POS, 100}, {SDL_SCANCODE_A, PAD_AXIS_LEFT_X_NEG, 100},
      {SDL_SCANCODE_W, PAD_AXIS_LEFT_Y_POS, 100}, {SDL_SCANCODE_S, PAD_AXIS_LEFT_Y_NEG, 100},
      {SDL_SCANCODE_L, PAD_AXIS_RIGHT_X_POS, 100}, {SDL_SCANCODE_H, PAD_AXIS_RIGHT_X_NEG, 100},
      {SDL_SCANCODE_Y, PAD_AXIS_RIGHT_Y_POS, 100}, {SDL_SCANCODE_N, PAD_AXIS_RIGHT_Y_NEG, 100},
      {SDL_SCANCODE_Q, PAD_AXIS_TRIGGER_L, 100}, {SDL_SCANCODE_E, PAD_AXIS_TRIGGER_R, 100}};
  if (!PADSetKeyButtonBindings(port, buttons) || !PADSetKeyAxisBindings(port, axes)) return FALSE;
  PADSetKeyboardActive(port, TRUE);
  return TRUE;
}

void PADClearKeyBindings(const u32 port) {
  if (port >= PAD_MAX_CONTROLLERS) {
    return;
  }
  g_pendingKeyPresses.clear();
  g_keyboardBindings[port].m_buttonMapping = g_defaultKeys;
  g_keyboardBindings[port].m_axisMapping = g_defaultKeyAxis;
  g_keyboardBindings[port].m_mappingsSet = false;
}

constexpr uint32_t k_keyboardMagic = SBIG('KBND');
constexpr int32_t k_keyboardVersion = 3;

static void load_keyboard_bindings() {
  if (aurora::g_config.userPath == nullptr) {
    return;
  }
  const auto filePath = aurora::io::fs_path_from_string(aurora::g_config.userPath) / "keyboard_bindings.dat";
  auto file = aurora::io::open_file(filePath, "rb");
  if (!file) {
    return;
  }

  uint32_t magic = 0;
  if (!SDL_ReadU32LE(file.get(), &magic) || magic != k_keyboardMagic) {
    Log.warn("keyboard_bindings.dat: invalid magic");
    return;
  }

  uint32_t version = 0;
  if (!SDL_ReadU32LE(file.get(), &version) || version != k_keyboardVersion) {
    Log.warn("keyboard_bindings.dat: version mismatch (expected {}, got {})", k_keyboardVersion, version);
    return;
  }

  Sint64 dataStart = 0;
  if (!seek_aligned(file.get(), dataStart)) {
    Log.warn("keyboard_bindings.dat: unable to seek to bindings");
    return;
  }

  auto bindings = g_keyboardBindings;
  bool ok = true;
  for (uint32_t port = 0; port < bindings.size(); ++port) {
    auto& [buttonMapping, axisMapping, mappingsSet] = bindings[port];
    uint8_t mappingsSetValue = 0;
    ok = ok && SDL_ReadU8(file.get(), &mappingsSetValue) &&
         aurora::io::read_exact(file.get(), buttonMapping.data(), sizeof(buttonMapping)) &&
         aurora::io::read_exact(file.get(), axisMapping.data(), sizeof(axisMapping));
    if (!ok) {
      Log.warn("keyboard_bindings.dat: truncated bindings for port {}", port);
      return;
    }
    mappingsSet = mappingsSetValue != 0;

    bool kbButtonCorrupt = false;
    for (uint32_t i = 0; i < PAD_BUTTON_COUNT; ++i) {
      if (buttonMapping[i].padButton != g_defaultKeys[i].padButton) {
        kbButtonCorrupt = true;
        break;
      }
    }
    if (kbButtonCorrupt) {
      Log.warn("keyboard_bindings.dat port={}: corrupt button identifiers, resetting to defaults", port);
      buttonMapping = g_defaultKeys;
    }

    bool kbAxisCorrupt = false;
    for (uint32_t i = 0; i < PAD_AXIS_COUNT; ++i) {
      if (axisMapping[i].padAxis != g_defaultKeyAxis[i].padAxis) {
        kbAxisCorrupt = true;
        break;
      }
    }
    if (kbAxisCorrupt) {
      Log.warn("keyboard_bindings.dat port={}: corrupt axis identifiers, resetting to defaults", port);
      axisMapping = g_defaultKeyAxis;
    }

    if (mappingsSet) {
      const bool anyBound =
          std::ranges::any_of(buttonMapping,
                              [](const PADKeyButtonBinding& b) { return b.scancode != PAD_KEY_INVALID; }) ||
          std::ranges::any_of(axisMapping, [](const PADKeyAxisBinding& b) { return b.scancode != PAD_KEY_INVALID; });
      if (!anyBound) {
        mappingsSet = false;
      }
    }
  }
  g_keyboardBindings = std::move(bindings);
}

static void save_keyboard_bindings() {
  if (aurora::g_config.userPath == nullptr) {
    return;
  }
  const auto filePath = aurora::io::fs_path_from_string(aurora::g_config.userPath) / "keyboard_bindings.dat";
  const auto pathString = aurora::io::fs_path_to_string(filePath);
  auto file = aurora::io::open_atomic_file(filePath);
  if (!file) {
    Log.warn("save_keyboard_bindings: failed to open {} for writing: {}", pathString, SDL_GetError());
    return;
  }

  bool ok = SDL_WriteU32LE(file.get(), k_keyboardMagic) && SDL_WriteS32LE(file.get(), k_keyboardVersion);
  Sint64 dataStart = 0;
  ok = ok && seek_aligned(file.get(), dataStart);

  for (const auto& [buttonMapping, axisMapping, mappingsSet] : g_keyboardBindings) {
    ok = ok && SDL_WriteU8(file.get(), mappingsSet) &&
         aurora::io::write_exact(file.get(), buttonMapping.data(), sizeof(buttonMapping)) &&
         aurora::io::write_exact(file.get(), axisMapping.data(), sizeof(axisMapping));
  }
  if (!ok || !file.commit()) {
    Log.warn("save_keyboard_bindings: failed to write {}: {}", pathString, SDL_GetError());
  }
}

void PADSerializeMappings() {
  if (aurora::g_config.userPath == nullptr) {
    return;
  }
  const auto basePath = aurora::io::fs_path_from_string(aurora::g_config.userPath);

  for (auto& controller : aurora::input::g_GameControllers | std::views::values) {
    EnsureMappingLoaded(&controller);
    const auto filePath =
        basePath / fmt::format("{}_{:04X}_{:04X}.controller", aurora::input::controller_name(controller.m_index),
                               controller.m_vid, controller.m_pid);
    const auto filePathStr = aurora::io::fs_path_to_string(filePath);

    auto file = aurora::io::open_atomic_file(filePath, aurora::io::AtomicFileMode::Preserve);
    if (!file) {
      Log.warn("Unable to open controller bindings! Path: \"{}\": {}", filePathStr, SDL_GetError());
      continue;
    }

    // write header
    constexpr uint32_t magic = SBIG('CTRL');
    bool ok = SDL_SeekIO(file.get(), 0, SDL_IO_SEEK_SET) == 0 && SDL_WriteU32LE(file.get(), magic) &&
              SDL_WriteU32LE(file.get(), k_mappingsFileVersion) &&
              SDL_WriteU8(file.get(), controller.m_isGameCube);

    // start writing data at next 32-byte aligned offset
    Sint64 dataStart = 0;
    ok = ok && seek_aligned(file.get(), dataStart);
    if (controller.m_isGameCube) {
      // GameCube adapters expose 4 input devices with the same vid/pid, we store all 4 in the same file
      const auto port = aurora::input::player_index(controller.m_index);
      if (port < 0 || port >= PAD_CHANMAX) {
        Log.warn("Unable to write controller bindings for invalid port {}! Path: \"{}\"", port, filePathStr);
        continue;
      }
      constexpr int64_t dzSecLen = sizeof(PADDeadZones);
      constexpr int64_t btnSecLen = sizeof(PADButtonMapping) * PAD_BUTTON_COUNT;
      constexpr int64_t axisSecLen = sizeof(PADAxisMapping) * PAD_AXIS_COUNT;
      // skip to offset in file for this particular port
      const Sint64 portOffset = dataStart + (dzSecLen + btnSecLen + axisSecLen) * port;
      ok = ok && SDL_SeekIO(file.get(), portOffset, SDL_IO_SEEK_SET) == portOffset;
    }
    ok = ok && aurora::io::write_exact(file.get(), &controller.m_deadZones, sizeof(controller.m_deadZones)) &&
         aurora::io::write_exact(file.get(), controller.m_buttonMapping.data(), sizeof(controller.m_buttonMapping)) &&
         aurora::io::write_exact(file.get(), controller.m_axisMapping.data(), sizeof(controller.m_axisMapping));

    if (!controller.m_isGameCube) {
      ok = ok && SDL_WriteU16LE(file.get(), controller.m_rumbleIntensityLow) &&
           SDL_WriteU16LE(file.get(), controller.m_rumbleIntensityHigh) &&
           SDL_WriteU8(file.get(), controller.m_forceDeviceRumble);
    }
    if (!ok || !file.commit()) {
      Log.warn("Unable to write controller bindings! Path: \"{}\": {}", filePathStr, SDL_GetError());
    }
  }

  save_keyboard_bindings();
}

PADDeadZones* PADGetDeadZones(const u32 port) {
  auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return nullptr;
  }
  return &controller->m_deadZones;
}

static constexpr std::array<std::pair<PADButton, std::string_view>, PAD_BUTTON_COUNT> skButtonNames = {{
    {PAD_BUTTON_LEFT, "Left"sv},
    {PAD_BUTTON_RIGHT, "Right"sv},
    {PAD_BUTTON_DOWN, "Down"sv},
    {PAD_BUTTON_UP, "Up"sv},
    {PAD_TRIGGER_Z, "Z"sv},
    {PAD_TRIGGER_R, "R"sv},
    {PAD_TRIGGER_L, "L"sv},
    {PAD_BUTTON_A, "A"sv},
    {PAD_BUTTON_B, "B"sv},
    {PAD_BUTTON_X, "X"sv},
    {PAD_BUTTON_Y, "Y"sv},
    {PAD_BUTTON_START, "Start"sv},
}};

static constexpr std::array<std::pair<PADButton, std::string_view>, PAD_AXIS_COUNT> skAxisNames = {{
    {PAD_AXIS_LEFT_X_POS, "Left X+"sv},
    {PAD_AXIS_LEFT_X_NEG, "Left X-"sv},
    {PAD_AXIS_LEFT_Y_POS, "Left Y+"sv},
    {PAD_AXIS_LEFT_Y_NEG, "Left Y-"sv},
    {PAD_AXIS_RIGHT_X_POS, "Right X+"sv},
    {PAD_AXIS_RIGHT_X_NEG, "Right X-"sv},
    {PAD_AXIS_RIGHT_Y_POS, "Right Y+"sv},
    {PAD_AXIS_RIGHT_Y_NEG, "Right Y-"sv},
    {PAD_AXIS_TRIGGER_L, "Trigger L"sv},
    {PAD_AXIS_TRIGGER_R, "Trigger R"sv},
}};

static constexpr std::array<std::pair<PADButton, std::string_view>, PAD_AXIS_COUNT> skAxisDirLabels = {{
    {PAD_AXIS_LEFT_X_POS, "Right"sv},
    {PAD_AXIS_LEFT_X_NEG, "Left"sv},
    {PAD_AXIS_LEFT_Y_POS, "Up"sv},
    {PAD_AXIS_LEFT_Y_NEG, "Down"sv},
    {PAD_AXIS_RIGHT_X_POS, "Right"sv},
    {PAD_AXIS_RIGHT_X_NEG, "Left"sv},
    {PAD_AXIS_RIGHT_Y_POS, "Up"sv},
    {PAD_AXIS_RIGHT_Y_NEG, "Down"sv},
    {PAD_AXIS_TRIGGER_L, "N/A"sv},
    {PAD_AXIS_TRIGGER_R, "N/A"sv},
}};

const char* PADGetButtonName(const PADButton button) {

  if (const auto iter =
          std::ranges::find_if(skButtonNames, [&button](const auto& pair) { return button == pair.first; });
      iter != skButtonNames.end()) {
    return iter->second.data();
  }

  return nullptr;
}

const char* PADGetNativeButtonName(u32 button) {
  return SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
}

const char* PADGetAxisName(const PADAxis axis) {
  if (const auto it = std::ranges::find_if(skAxisNames, [&axis](const auto& pair) { return axis == pair.first; });
      it != skAxisNames.end()) {
    return it->second.data();
  }

  return nullptr;
}

const char* PADGetAxisDirectionLabel(const PADAxis axis) {
  if (const auto it = std::ranges::find_if(skAxisDirLabels, [&axis](const auto& pair) { return axis == pair.first; });
      it != skAxisDirLabels.end()) {
    return it->second.data();
  }

  return nullptr;
}

const char* PADGetNativeAxisName(PADSignedNativeAxis axis) {
  return SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(axis.nativeAxis));
}

int32_t PADGetNativeButtonPressed(const u32 port) {
  const auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return -1;
  }

  for (int32_t i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i) {
    if (SDL_GetGamepadButton(controller->m_controller, static_cast<SDL_GamepadButton>(i)) != 0u) {
      return i;
    }
  }
  return -1;
}

PADSignedNativeAxis PADGetNativeAxisPulled(const u32 port) {
  const auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return {-1, AXIS_SIGN_POSITIVE};
  }

  for (int32_t i = 0; i < SDL_GAMEPAD_AXIS_COUNT; ++i) {
    const auto axisVal = SDL_GetGamepadAxis(controller->m_controller, static_cast<SDL_GamepadAxis>(i));
    if (axisVal >= 16384) {
      return {i, AXIS_SIGN_POSITIVE};
    }

    if (axisVal <= -16384) {
      // SDL3 triggers rest at -32768, so skip their negative direction.
      if (i == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || i == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
        continue;
      }
      return {i, AXIS_SIGN_NEGATIVE};
    }
  }
  return {-1, AXIS_SIGN_POSITIVE};
}

void PADRestoreDefaultMapping(const u32 port) {
  auto* controller = aurora::input::get_controller_for_player(port);
  if (controller == nullptr) {
    return;
  }
  __PADSetDefaultMapping(controller);
  controller->m_axisMapping = g_defaultAxes;
}

void PADSetControlsEditing(const bool editing) {
  if (g_controlsEditing != editing) reset_controls_navigation();
  if(editing)g_pendingKeyPresses.clear();
  if (g_controlsEditing && !editing) g_suppressHeldOnRead = true;
  g_controlsEditing = editing;
}

void PADSetWindowFocused(const bool focused) {
  if (g_windowFocused != focused) {
    g_pendingKeyPresses.clear();
    reset_controls_navigation();
  }
  if (!g_windowFocused && focused) g_suppressHeldOnRead = true;
  g_windowFocused = focused;
}

void PADBlockInput(const bool block) {
  if (g_blockPAD != block) reset_controls_navigation();
  if(block)g_pendingKeyPresses.clear();
  if (g_blockPAD && !block) {
    g_suppressHeldOnRead = true;
  }
  g_blockPAD = block;
}

void PADHandleKeyPress(const s32 scancode,const bool repeat) {
  if (g_nativeControlsPage && !g_bindingCapture.active && g_controlsNavigationReady &&
      g_windowFocused && !g_blockPAD && !g_controlsEditing)
    g_controlsNavigationKeys.press(scancode, repeat);
  if (g_bindingCapture.active && g_windowFocused && !repeat &&
      (scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_BACKSPACE ||
       (g_bindingCapture.armed && g_bindingCapture.keyboard)) &&
      g_bindingCapture.key == PAD_KEY_INVALID)
    g_bindingCapture.key = scancode;
  const bool accepted=g_windowFocused && !g_blockPAD && !g_controlsEditing && !g_bindingCapture.active && !g_nativeControlsPage;
  if(accepted)g_pendingKeyPresses.press(scancode,repeat);
  if(key_tap_diagnostic() && !repeat)
    std::fprintf(stderr,"NATIVE_KEY_TAP_EVENT scancode=%d accepted=%u focus=%u block=%u editing=%u\n",
        int(scancode),unsigned(accepted),unsigned(g_windowFocused),unsigned(g_blockPAD),unsigned(g_controlsEditing));
}

const char* PADGetKeyName(const s32 scancode) {
  if (scancode == PAD_KEY_INVALID) return "Unbound";
  if (scancode < 0) return "Mouse";
  const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
  return name && *name ? name : "Unknown";
}

void PADCancelBindingCapture() {
  if (g_bindingCapture.active) g_suppressHeldOnRead = true;
  g_bindingCapture.active = false;
  reset_controls_navigation();
  g_pendingKeyPresses.clear();
}

void PADSetControlsPageActive(const bool active) {
  if (g_nativeControlsPage == active) return;
  PADCancelBindingCapture();
  g_nativeControlsPage = active;
  g_suppressHeldOnRead = true;
}

u16 PADReadControlsNavigation() {
  if (!g_nativeControlsPage || g_bindingCapture.active || !g_windowFocused || g_blockPAD || g_controlsEditing) return 0;
  int count = 0;
  const bool* keys = SDL_GetKeyboardState(&count);
  auto taps = g_controlsNavigationKeys.take();
  const auto key = [&](const int code) { return aurora::input::keyboard_sample_pressed(keys, count, code, taps); };
  u16 held = 0;
  if (key(SDL_SCANCODE_UP) || key(SDL_SCANCODE_W)) held |= PAD_BUTTON_UP;
  if (key(SDL_SCANCODE_DOWN) || key(SDL_SCANCODE_S)) held |= PAD_BUTTON_DOWN;
  if (key(SDL_SCANCODE_LEFT) || key(SDL_SCANCODE_A)) held |= PAD_BUTTON_LEFT;
  if (key(SDL_SCANCODE_RIGHT) || key(SDL_SCANCODE_D)) held |= PAD_BUTTON_RIGHT;
  if (key(SDL_SCANCODE_RETURN) || key(SDL_SCANCODE_KP_ENTER)) held |= PAD_BUTTON_A;
  if (key(SDL_SCANCODE_ESCAPE)) held |= PAD_BUTTON_B;
  for (u32 port = 0; port < PAD_CHANMAX; ++port) {
    const auto* controller = aurora::input::get_controller_for_player(port);
    if (!controller) continue;
    SDL_Gamepad* pad = controller->m_controller;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP) || SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) < -16384) held |= PAD_BUTTON_UP;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) || SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) > 16384) held |= PAD_BUTTON_DOWN;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) || SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) < -16384) held |= PAD_BUTTON_LEFT;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) > 16384) held |= PAD_BUTTON_RIGHT;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH)) held |= PAD_BUTTON_A;
    if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST)) held |= PAD_BUTTON_B;
  }
  if (!g_controlsNavigationReady) {
    if (!held) g_controlsNavigationReady = true;
    return 0;
  }
  const u16 pressed = held & ~g_controlsNavigationLast;
  g_controlsNavigationFrames = held && held == g_controlsNavigationLast ? g_controlsNavigationFrames + 1 : 0;
  g_controlsNavigationLast = held;
  const u16 directions = PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT;
  return pressed | (g_controlsNavigationFrames >= 24 && (g_controlsNavigationFrames % 6) == 0 ? held & directions : 0);
}

static s32 g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_NONE;

s32 PADGetBindingCaptureRefusal() { return g_bindingCaptureRefusal; }

BOOL PADBeginBindingCapture(const u32 port, const bool keyboard, const bool axis, const u16 output) {
  g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_NONE;
  if (port >= PAD_CHANMAX) g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_PORT;
  else if (g_bindingCapture.active) g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_ACTIVE;
  else if (g_controlsEditing) g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_EDITOR;
  else if (g_blockPAD) g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_BLOCKED;
  else if (!g_windowFocused) g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_FOCUS;
  const bool trace = std::getenv("AURORA_DIAGNOSTIC_BINDING_CAPTURE") != nullptr;
  if (trace)
    std::fprintf(stderr, "NATIVE_BINDING_BEGIN port=%u keyboard=%u axis=%u output=%u active=%u editing=%u block=%u focus=%u native_page=%u\n",
                 port, unsigned(keyboard), unsigned(axis), unsigned(output), unsigned(g_bindingCapture.active),
                 unsigned(g_controlsEditing), unsigned(g_blockPAD), unsigned(g_windowFocused), unsigned(g_nativeControlsPage));
  if (g_bindingCaptureRefusal != PAD_BINDING_REFUSAL_NONE) {
    if (trace) std::fprintf(stderr, "NATIVE_BINDING_BEGIN refused=guard reason=%d\n", g_bindingCaptureRefusal);
    return FALSE;
  }
  u32 count = 0;
  bool found = false;
  if (keyboard && axis) {
    const auto* mappings = PADGetKeyAxisBindings(port, &count);
    for (u32 i = 0; mappings && i < count; ++i) found |= mappings[i].padAxis == output;
  } else if (keyboard) {
    const auto* mappings = PADGetKeyButtonBindings(port, &count);
    for (u32 i = 0; mappings && i < count; ++i) found |= mappings[i].padButton == output;
  } else if (axis) {
    const auto* mappings = PADGetAxisMappings(port, &count);
    for (u32 i = 0; mappings && i < count; ++i) found |= mappings[i].padAxis == output;
  } else {
    const auto* mappings = PADGetButtonMappings(port, &count);
    for (u32 i = 0; mappings && i < count; ++i) found |= mappings[i].padButton == output;
  }
  if (!found) {
    g_bindingCaptureRefusal = PAD_BINDING_REFUSAL_MAPPING;
    if (trace) std::fprintf(stderr, "NATIVE_BINDING_BEGIN refused=mapping count=%u\n", count);
    return FALSE;
  }
  g_bindingCapture = {true, false, keyboard, axis, port, output, PAD_KEY_INVALID, SDL_GetTicks() + 15000};
  g_pendingKeyPresses.clear();
  if (trace) std::fputs("NATIVE_BINDING_BEGIN accepted=1\n", stderr);
  return TRUE;
}

s32 PADPollBindingCapture() {
  auto& capture = g_bindingCapture;
  if (!capture.active) return 3;
  if (!g_windowFocused || g_blockPAD || SDL_GetTicks() >= capture.deadline ||
      (!capture.keyboard && PADGetIndexForPort(capture.port) < 0) || capture.key == SDL_SCANCODE_ESCAPE) {
    PADCancelBindingCapture();
    return 3;
  }
  if (!capture.armed) {
    int count = 0;
    const bool* keys = SDL_GetKeyboardState(&count);
    for (int i = 0; i < count; ++i) if (keys[i]) return 0;
    for (u32 port = 0; port < PAD_CHANMAX; ++port) {
      if (PADGetNativeButtonPressed(port) >= 0 || PADGetNativeAxisPulled(port).nativeAxis >= 0) return 0;
    }
    capture.armed = true;
    return 1;
  }
  bool unbind = capture.key == SDL_SCANCODE_BACKSPACE;
  BOOL accepted = FALSE;
  if (capture.keyboard) {
    if (capture.key == PAD_KEY_INVALID) return 1;
    const s32 key = unbind ? PAD_KEY_INVALID : capture.key;
    if (capture.axis) accepted = PADTrySetKeyAxisBinding(capture.port, {key, capture.output, 100});
    else accepted = PADTrySetKeyButtonBinding(capture.port, {key, capture.output});
  } else {
    const s32 button = PADGetNativeButtonPressed(capture.port);
    const PADSignedNativeAxis axis = PADGetNativeAxisPulled(capture.port);
    if (!unbind && button < 0 && (!capture.axis || axis.nativeAxis < 0)) return 1;
    if (capture.axis) accepted = PADTrySetAxisMapping(capture.port,
        {unbind || button >= 0 ? PADSignedNativeAxis{-1, AXIS_SIGN_POSITIVE} : axis,
         unbind ? -1 : button, capture.output});
    else accepted = PADTrySetButtonMapping(capture.port, {static_cast<u32>(unbind ? -1 : button), capture.output});
  }
  PADCancelBindingCapture();
  return accepted ? 2 : 4;
}

SDL_Gamepad* PADGetSDLGamepadForIndex(const u32 index) {
  const auto* ctrl = __PADGetControllerForIndex(index);
  if (ctrl == nullptr) {
    return nullptr;
  }

  return ctrl->m_controller;
}

void PADSetDefaultMapping(const PADDefaultMapping* mapping, const PADControllerType type) {
  if (g_initialized) {
    Log.fatal("PADSetDefaultMapping called after PADInit()!");
  }

  switch (type) {
  case PAD_TYPE_STANDARD:
    g_defaultButtonsStandard = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_XBOX360:
    g_defaultButtonsXBox360 = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_XBOXONE:
    g_defaultButtonsXBoxOne = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_PS3:
    g_defaultButtonsPS3 = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_PS4:
    g_defaultButtonsPS4 = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_PS5:
    g_defaultButtonsPS5 = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_SWITCH_PROCON:
    g_defaultButtonsProCon = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_JOYCON_LEFT:
    g_defaultButtonsJoyConLeft = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_JOYCON_RIGHT:
    g_defaultButtonsJoyConRight = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_JOYCON_PAIR:
    g_defaultButtonsJoyPair = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_GAMECUBE:
    g_defaultButtonsGamecube = toStdArray(mapping->buttons);
    break;
  case PAD_TYPE_NSO_GAMECUBE:
    g_defaultButtonsNSOGamecube = toStdArray(mapping->buttons);
    break;
  default:
    break;
  }
  g_defaultAxes = toStdArray(mapping->axes);
}

BOOL PADSetColor(const u32 port, const u8 red, const u8 green, const u8 blue) {
  const auto ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return FALSE;
  }

  ctrl->m_ledRed = red;
  ctrl->m_ledGreen = green;
  ctrl->m_ledBlue = blue;
  ctrl->m_isColorDirty = true;
  return true;
}

BOOL PADGetColor(const u32 port, u8* red, u8* green, u8* blue) {
  const auto ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return FALSE;
  }

  *red = ctrl->m_ledRed;
  *green = ctrl->m_ledGreen;
  *blue = ctrl->m_ledBlue;
  return TRUE;
}

BOOL PADSetSensorEnabled(const u32 port, const PADSensorType sensor, const BOOL enabled) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);

  if (controller_has_sensor(ctrl, sensor)) {
    return SDL_SetGamepadSensorEnabled(ctrl->m_controller, static_cast<SDL_SensorType>(sensor), enabled ? true : false)
               ? TRUE
               : FALSE;
  }

  return should_use_device_sensor(port, ctrl, sensor) ? TRUE : FALSE;
}

BOOL PADHasSensor(const u32 port, const PADSensorType sensor) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (controller_has_sensor(ctrl, sensor)) {
    return TRUE;
  }

  return should_use_device_sensor(port, ctrl, sensor) ? TRUE : FALSE;
}

BOOL PADGetSensorData(const u32 port, const PADSensorType sensor, f32* data, const int nValues) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (controller_has_sensor(ctrl, sensor)) {
    return SDL_GetGamepadSensorData(ctrl->m_controller, static_cast<SDL_SensorType>(sensor), data, nValues);
  }

  if (should_use_device_sensor(port, ctrl, sensor)) {
    return get_device_sensor_data(sensor, data, nValues) ? TRUE : FALSE;
  }

  return FALSE;
}

BOOL PADHasLED(const u32 port) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);

  if (ctrl == nullptr) {
    return FALSE;
  }

  return ctrl->m_hasRgbLed;
}

BOOL PADSetRumbleIntensity(const u32 port, const u16 low, const u16 high) {
  auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl != nullptr) {
    if (ctrl->m_isGameCube || (!ctrl->m_hasRumble && !should_use_device_rumble(port, ctrl))) {
      return FALSE;
    }
    EnsureMappingLoaded(ctrl);
    ctrl->m_rumbleIntensityLow = low;
    ctrl->m_rumbleIntensityHigh = high;
    return TRUE;
  }

  if (!should_use_device_rumble(port, nullptr)) {
    return FALSE;
  }
  aurora::input::set_device_rumble_intensity(low, high);
  return TRUE;
}

BOOL PADGetRumbleIntensity(const u32 port, u16* low, u16* high) {
  auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl != nullptr) {
    if (ctrl->m_isGameCube || (!ctrl->m_hasRumble && !should_use_device_rumble(port, ctrl))) {
      *low = 0;
      *high = 0;
      return FALSE;
    }
    EnsureMappingLoaded(ctrl);
    *low = ctrl->m_rumbleIntensityLow;
    *high = ctrl->m_rumbleIntensityHigh;
    return TRUE;
  }

  if (!should_use_device_rumble(port, nullptr)) {
    *low = 0;
    *high = 0;
    return FALSE;
  }

  aurora::input::get_device_rumble_intensity(low, high);
  return TRUE;
}

BOOL PADSupportsRumbleIntensity(const u32 port) {
  if (const auto* ctrl = aurora::input::get_controller_for_player(port)) {
    if (!ctrl->m_isGameCube && (ctrl->m_hasRumble || should_use_device_rumble(port, ctrl))) {
      return TRUE;
    }
    return FALSE;
  }
  return should_use_device_rumble(port, nullptr) ? TRUE : FALSE;
}

BOOL PADCanForceDeviceRumble(const u32 port) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  return ctrl != nullptr && !ctrl->m_isGameCube && ctrl->m_hasRumble && device_rumble_available_for_port(port) ? TRUE
                                                                                                               : FALSE;
}

BOOL PADGetForceDeviceRumble(const u32 port) {
  auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr || !PADCanForceDeviceRumble(port)) {
    return FALSE;
  }

  EnsureMappingLoaded(ctrl);
  return ctrl->m_forceDeviceRumble ? TRUE : FALSE;
}

BOOL PADSetForceDeviceRumble(const u32 port, const BOOL force) {
  auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr || !PADCanForceDeviceRumble(port)) {
    return FALSE;
  }

  EnsureMappingLoaded(ctrl);
  ctrl->m_forceDeviceRumble = force != FALSE;
  return TRUE;
}

BOOL PADIsGCAdapter(const u32 port) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return FALSE;
  }
  return ctrl->m_isGameCube;
}

PADBatteryState PADGetBatteryState(const u32 port, f32* perc) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return PAD_BATTERYSTATE_ERROR;
  }

  int tmp = 0;
  const auto ret = SDL_GetGamepadPowerInfo(ctrl->m_controller, &tmp);
  if (tmp != -1) {
    *perc = static_cast<float>(tmp) / 100.f;
  } else {
    *perc = static_cast<float>(tmp);
  }
  return static_cast<PADBatteryState>(ret);
}

PADControllerType PADGetControllerType(const u32 port) {
  const auto* ctrl = aurora::input::get_controller_for_player(port);
  if (ctrl == nullptr) {
    return PAD_TYPE_UNKNOWN;
  }

  auto type = SDL_GetGamepadType(ctrl->m_controller);
  return static_cast<PADControllerType>(type);
}

PADControllerType PADGetControllerTypeForIndex(const u32 index) {
  const auto* ctrl = __PADGetControllerForIndex(index);
  if (ctrl == nullptr) {
    return PAD_TYPE_UNKNOWN;
  }

  auto type = SDL_GetGamepadType(ctrl->m_controller);
  if (type == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO && ctrl->m_pid == 0x2073) {
    return PAD_TYPE_NSO_GAMECUBE;
  }
  return static_cast<PADControllerType>(type);
}
