#include "imgui.hpp"

#include <cstddef>
#include <cfloat>
#include <atomic>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <webgpu/webgpu_cpp.h>
#include <aurora/gfx.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_gamepad.h>
#include <dolphin/pad.h>

#include "internal.hpp"
#include "gfx/render_worker.hpp"
#include "webgpu/gpu.hpp"
#include "window.hpp"

#define IMGUI_IMPL_WEBGPU_BACKEND_DAWN
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "backends/imgui_impl_wgpu.h"
#include "tracy/Tracy.hpp"

namespace aurora::imgui {
namespace {
float g_scale;
std::string g_imguiSettings{};
std::string g_imguiLog{};
bool g_useSdlRenderer = false;
bool g_controlsAvailable = false;
bool g_controlsOpen = false;
bool g_graphicsOpen = false;
bool g_graphicsDiagnosticInitialized = false;
std::atomic_bool g_fpsCounterEnabled = false;

std::vector<SDL_Texture*> g_sdlTextures;
std::vector<wgpu::Texture> g_wgpuTextures;

wgpu::Buffer create_texture_upload_buffer(uint32_t width, uint32_t height, const uint8_t* data,
                                          uint32_t copyBytesPerRow) {
  const uint32_t rowBytes = width * 4;
  const uint64_t uploadSize = static_cast<uint64_t>(copyBytesPerRow) * height;
  const wgpu::BufferDescriptor desc{
      .label = "imgui texture upload buffer",
      .usage = wgpu::BufferUsage::CopySrc,
      .size = uploadSize,
      .mappedAtCreation = true,
  };
  auto buffer = webgpu::g_device.CreateBuffer(&desc);
  auto* dst = static_cast<uint8_t*>(buffer.GetMappedRange(0, uploadSize));
  for (uint32_t row = 0; row < height; ++row) {
    std::memcpy(dst, data, rowBytes);
    dst += copyBytesPerRow;
    data += rowBytes;
  }
  buffer.Unmap();
  return buffer;
}

void enqueue_texture_upload(wgpu::Buffer buffer, wgpu::TexelCopyTextureInfo dst, wgpu::TexelCopyBufferLayout layout,
                            wgpu::Extent3D size) {
  gfx::render_worker::enqueue_work([buffer = std::move(buffer), dst = std::move(dst), layout, size] {
    const wgpu::CommandEncoderDescriptor encoderDesc{
        .label = "imgui texture upload encoder",
    };
    auto encoder = webgpu::g_device.CreateCommandEncoder(&encoderDesc);
    const wgpu::TexelCopyBufferInfo src{
        .layout = layout,
        .buffer = buffer,
    };
    encoder.CopyBufferToTexture(&src, &dst, &size);
    constexpr wgpu::CommandBufferDescriptor commandBufferDesc{
        .label = "imgui texture upload command buffer",
    };
    auto commandBuffer = encoder.Finish(&commandBufferDesc);
    webgpu::g_queue.Submit(1, &commandBuffer);
  });
}
} // namespace

struct DrawData::Impl {
  ImDrawData drawData;
  std::vector<std::unique_ptr<ImDrawList>> drawLists;
};

void create_context() noexcept {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  g_fpsCounterEnabled = false;
  ImGuiIO& io = ImGui::GetIO();
  g_imguiSettings = g_config.userPath ? std::string{g_config.userPath} + "/imgui.ini" : std::string{};
  g_imguiLog = std::string{g_config.cachePath} + "/imgui.log";
  io.IniFilename = g_config.sessionOnlyPreferences ? nullptr : g_imguiSettings.c_str();
  io.LogFilename = g_config.sessionOnlyPreferences ? nullptr : g_imguiLog.c_str();
}

void initialize() noexcept {
  ZoneScoped;
  SDL_Renderer* renderer = window::get_sdl_renderer();
  ImGui_ImplSDL3_InitForSDLRenderer(window::get_sdl_window(), renderer);
  g_useSdlRenderer = renderer != nullptr;
  if (g_useSdlRenderer) {
    ImGui_ImplSDLRenderer3_Init(renderer);
  } else {
    ImGui_ImplWGPU_InitInfo info;
    info.Device = webgpu::g_device.Get();
    info.RenderTargetFormat = static_cast<WGPUTextureFormat>(webgpu::g_graphicsConfig.surfaceConfiguration.format);
    ImGui_ImplWGPU_Init(&info);
  }
}

void shutdown() noexcept {
  ZoneScoped;
  if (g_useSdlRenderer) {
    ImGui_ImplSDLRenderer3_Shutdown();
  } else {
    ImGui_ImplWGPU_Shutdown();
  }
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  for (const auto& texture : g_sdlTextures) {
    SDL_DestroyTexture(texture);
  }
  g_sdlTextures.clear();
  g_wgpuTextures.clear();
}

void process_event(const SDL_Event& event) noexcept {
  auto renderEvent = event;
  if (g_useSdlRenderer) {
    if (SDL_Renderer* renderer = window::get_sdl_renderer()) {
      SDL_ConvertEventToRenderCoordinates(renderer, &renderEvent);
    }
  }
  ImGui_ImplSDL3_ProcessEvent(&renderEvent);
}

bool wants_capture_event(const SDL_Event& event) noexcept {
  if (ImGui::GetCurrentContext() == nullptr) {
    return false;
  }

  const ImGuiIO& io = ImGui::GetIO();
  switch (event.type) {
  case SDL_EVENT_MOUSE_MOTION:
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
  case SDL_EVENT_MOUSE_WHEEL:
  case SDL_EVENT_FINGER_DOWN:
  case SDL_EVENT_FINGER_MOTION:
  case SDL_EVENT_FINGER_UP:
  case SDL_EVENT_FINGER_CANCELED:
    return io.WantCaptureMouse;
  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP:
  case SDL_EVENT_TEXT_INPUT:
    return io.WantCaptureKeyboard || io.WantTextInput;
  default:
    return false;
  }
}

namespace {
const char* key_label(int code) {
  switch (code) {
  case PAD_KEY_INVALID: return "Unbound";
  case PAD_KEY_MOUSE_LEFT: return "Mouse left";
  case PAD_KEY_MOUSE_MIDDLE: return "Mouse middle";
  case PAD_KEY_MOUSE_RIGHT: return "Mouse right";
  case PAD_KEY_MOUSE_X1: return "Mouse X1";
  case PAD_KEY_MOUSE_X2: return "Mouse X2";
  default: return SDL_GetScancodeName(static_cast<SDL_Scancode>(code));
  }
}
bool choose_key(const char* label, int& code) {
  bool changed = false;
  if (ImGui::BeginCombo(label, key_label(code))) {
    for (int key = PAD_KEY_MOUSE_X2; key < SDL_SCANCODE_COUNT; ++key) {
      if (key == 0) continue;
      const char* name = key_label(key);
      if (!name || !*name) continue;
      ImGui::PushID(key);
      if (ImGui::Selectable(name, key == code)) { code = key; changed = true; }
      ImGui::PopID();
    }
    ImGui::EndCombo();
  }
  return changed;
}
void show_shared_bindings(const std::vector<std::pair<std::string, std::string>>& bindings) {
  bool heading = false;
  std::string diagnostic;
  for (size_t i = 0; i < bindings.size(); ++i) {
    bool seen = false;
    for (size_t before = 0; before < i; ++before) if (bindings[before].first == bindings[i].first) seen = true;
    if (seen) continue;
    std::string outputs = bindings[i].second;
    bool shared = false;
    for (size_t after = i + 1; after < bindings.size(); ++after) {
      if (bindings[after].first == bindings[i].first) { outputs += ", " + bindings[after].second; shared = true; }
    }
    if (!shared) continue;
    if (!heading) { ImGui::Separator(); ImGui::TextWrapped("Shared inputs (allowed): all listed actions respond together."); heading = true; }
    ImGui::TextWrapped("%s: %s", bindings[i].first.c_str(), outputs.c_str());
    diagnostic += "NATIVE_CONTROLS_SHARED source=" + bindings[i].first + " actions=" + outputs + "\n";
  }
  if (!heading) ImGui::TextUnformatted("No shared inputs on this port.");
  if (std::getenv("AURORA_DIAGNOSTIC_CONTROLS_OPEN")) {
    static std::string previous;
    if (previous != diagnostic) { std::fputs(diagnostic.c_str(), stderr); previous = diagnostic; }
  }
}
void keyboard_shared_bindings(u32 port) {
  std::vector<std::pair<std::string, std::string>> bindings;
  u32 count = 0;
  if (const auto* buttons = PADGetKeyButtonBindings(port, &count)) {
    for (u32 i = 0; i < count; ++i) if (buttons[i].scancode != PAD_KEY_INVALID) {
      const char* key = key_label(buttons[i].scancode);
      const char* output = PADGetButtonName(buttons[i].padButton);
      if (key && *key && output) bindings.emplace_back(key, output);
    }
  }
  if (const auto* axes = PADGetKeyAxisBindings(port, &count)) {
    for (u32 i = 0; i < count; ++i) if (axes[i].scancode != PAD_KEY_INVALID) {
      const char* key = key_label(axes[i].scancode);
      const char* output = PADGetAxisName(axes[i].padAxis);
      if (key && *key && output) bindings.emplace_back(key, output);
    }
  }
  show_shared_bindings(bindings);
}
void controller_shared_bindings(u32 port) {
  std::vector<std::pair<std::string, std::string>> bindings;
  u32 count = 0;
  if (const auto* buttons = PADGetButtonMappings(port, &count)) {
    for (u32 i = 0; i < count; ++i) {
      const char* source = PADGetNativeButtonName(buttons[i].nativeButton);
      const char* output = PADGetButtonName(buttons[i].padButton);
      if (source && output) bindings.emplace_back(source, output);
    }
  }
  if (const auto* axes = PADGetAxisMappings(port, &count)) {
    for (u32 i = 0; i < count; ++i) {
      const auto& binding = axes[i];
      const char* source = binding.nativeAxis.nativeAxis == -1 ? PADGetNativeButtonName(binding.nativeButton) : PADGetNativeAxisName(binding.nativeAxis);
      const char* output = PADGetAxisName(binding.padAxis);
      if (source && output) {
        std::string name = source;
        if (binding.nativeAxis.nativeAxis != -1) name += binding.nativeAxis.sign == AXIS_SIGN_NEGATIVE ? " negative" : " positive";
        bindings.emplace_back(std::move(name), output);
      }
    }
  }
  show_shared_bindings(bindings);
}
void controls_panel() {
  if (!g_config.sessionOnlyPreferences) return;
  // Normal control editing belongs to the native game menu. Keep the old
  // host editor only for explicitly requested development diagnostics.
  if (!std::getenv("AURORA_DIAGNOSTIC_CONTROLS_OPEN") &&
      !std::getenv("AURORA_DIAGNOSTIC_GRAPHICS_OPEN")) return;
  // Lets native-menu tests inspect the game's actual composition without the
  // separate host-side options dock obscuring the letterbox area.
  if (std::getenv("AURORA_DIAGNOSTIC_HIDE_NATIVE_OPTIONS")) return;
  static bool diagnosticInitialized = false;
  if (!diagnosticInitialized) { g_controlsOpen = std::getenv("AURORA_DIAGNOSTIC_CONTROLS_OPEN") != nullptr; diagnosticInitialized = true; }
  if (!g_graphicsDiagnosticInitialized) {
    g_graphicsOpen = std::getenv("AURORA_DIAGNOSTIC_GRAPHICS_OPEN") != nullptr;
    g_graphicsDiagnosticInitialized = true;
  }
  if (!g_controlsAvailable && !std::getenv("AURORA_DIAGNOSTIC_CONTROLS_OPEN")) return;
  auto& open = g_controlsOpen;
  static bool reported = false;
  static int port = 0;
  static const char* feedback = "";
  if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) open = !open;
  ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Input setup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (ImGui::Button("Configure controls (F10)")) open = !open;
  }
  ImGui::End();
  if (open) {
    ImGui::SetNextWindowSize(ImVec2(520, 430), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Controls", &open)) {
      if (!reported && std::getenv("AURORA_DIAGNOSTIC_CONTROLS_OPEN")) {
        std::fputs("NATIVE_CONTROLS_PANEL drawn=1 session_only=1 input_capture=1\n", stderr); reported = true;
      }
      ImGui::TextUnformatted("Changes last for this session. Game input is blocked while editing.");
      ImGui::Combo("Player port", &port, "Player 1\0Player 2\0Player 3\0Player 4\0");
      if (ImGui::BeginTabBar("Binding type")) {
        if (ImGui::BeginTabItem("Keyboard")) {
          if (ImGui::Button("Enable standard keyboard layout")) {
            feedback = PADInstallKeyboardPreset(port) ? "Keyboard enabled." : "Could not enable keyboard.";
          }
          ImGui::SameLine();
          if (ImGui::Button("Disable keyboard")) { PADSetKeyboardActive(port, FALSE); feedback = "Keyboard disabled."; }
          u32 count = 0;
          if (auto* buttons = PADGetKeyButtonBindings(port, &count)) {
            for (u32 i = 0; i < count; ++i) {
              ImGui::PushID(static_cast<int>(i));
              auto binding = buttons[i];
              if (choose_key(PADGetButtonName(binding.padButton), binding.scancode))
                feedback = PADTrySetKeyButtonBinding(port, binding) ? "Binding updated." : "Unsupported key; binding unchanged.";
              ImGui::PopID();
            }
          }
          if (auto* axes = PADGetKeyAxisBindings(port, &count)) {
            for (u32 i = 0; i < count; ++i) {
              ImGui::PushID(static_cast<int>(i + PAD_BUTTON_COUNT));
              auto binding = axes[i];
              if (choose_key(PADGetAxisName(binding.padAxis), binding.scancode)) {
                binding.influence = 100;
                feedback = PADTrySetKeyAxisBinding(port, binding) ? "Binding updated." : "Unsupported key; binding unchanged.";
              }
              ImGui::PopID();
            }
          }
          keyboard_shared_bindings(port);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Controller")) {
          const char* assignedName = PADGetName(port);
          if (ImGui::BeginCombo("Assigned controller", assignedName ? assignedName : "None")) {
            if (ImGui::Selectable("None", assignedName == nullptr)) { PADClearPort(port); feedback = "Controller unassigned."; }
            for (u32 index = 0; index < PADCount(); ++index) {
              const char* device = PADGetNameForControllerIndex(index);
              ImGui::PushID(static_cast<int>(index));
              if (device && ImGui::Selectable(device, PADGetIndexForPort(port) == static_cast<s32>(index)))
                feedback = PADTrySetPortForIndex(index, port) ? "Controller assigned. Previous device on this port is unassigned." : "Could not assign controller; unchanged.";
              ImGui::PopID();
            }
            ImGui::EndCombo();
          }
          const char* name = PADGetName(port);
          ImGui::Text("Device: %s", name ? name : "No controller assigned");
          u32 count = 0;
          if (auto* buttons = PADGetButtonMappings(port, &count)) {
            for (u32 i = 0; i < count; ++i) {
              auto binding = buttons[i];
              ImGui::PushID(static_cast<int>(i));
              if (ImGui::BeginCombo(PADGetButtonName(binding.padButton), PADGetNativeButtonName(binding.nativeButton))) {
                for (int button = -1; button < SDL_GAMEPAD_BUTTON_COUNT; ++button) {
                  const char* label = button == -1 ? "Unbound" : SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
                  if (label && ImGui::Selectable(label, binding.nativeButton == static_cast<u32>(button))) {
                    binding.nativeButton = button;
                    feedback = PADTrySetButtonMapping(port, binding) ? "Binding updated." : "Device does not support that button; unchanged.";
                  }
                }
                ImGui::EndCombo();
              }
              ImGui::PopID();
            }
          }
          if (auto* axes = PADGetAxisMappings(port, &count)) {
            for (u32 i = 0; i < count; ++i) {
              auto binding = axes[i];
              ImGui::PushID(static_cast<int>(i + PAD_BUTTON_COUNT));
              char axisPreview[80];
              std::snprintf(axisPreview, sizeof(axisPreview), "%s %s", PADGetNativeAxisName(binding.nativeAxis) ? PADGetNativeAxisName(binding.nativeAxis) : "Unknown axis", binding.nativeAxis.sign == AXIS_SIGN_NEGATIVE ? "negative" : "positive");
              const char* preview = binding.nativeAxis.nativeAxis == -1 ? (binding.nativeButton == -1 ? "Unbound" : PADGetNativeButtonName(binding.nativeButton)) : axisPreview;
              if (ImGui::BeginCombo(PADGetAxisName(binding.padAxis), preview)) {
                for (int axis = -1; axis < SDL_GAMEPAD_AXIS_COUNT; ++axis) {
                  for (int direction = 0; direction < (axis == -1 ? 1 : 2); ++direction) {
                    PADAxisMapping edit = binding;
                    edit.nativeAxis.nativeAxis = axis;
                    edit.nativeAxis.sign = direction ? AXIS_SIGN_NEGATIVE : AXIS_SIGN_POSITIVE;
                    edit.nativeButton = -1;
                    ImGui::PushID(axis * 2 + direction);
                    char option[80];
                    std::snprintf(option, sizeof(option), "%s %s", axis == -1 ? "Unbound" : PADGetNativeAxisName(edit.nativeAxis), axis == -1 ? "" : (direction ? "negative" : "positive"));
                    if (ImGui::Selectable(option))
                      feedback = PADTrySetAxisMapping(port, edit) ? "Binding updated." : "Device does not support that axis; unchanged.";
                    ImGui::PopID();
                  }
                }
                for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; ++button) {
                  PADAxisMapping edit = binding;
                  edit.nativeAxis.nativeAxis = -1; edit.nativeAxis.sign = AXIS_SIGN_POSITIVE; edit.nativeButton = button;
                  const char* label = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
                  if (label && ImGui::Selectable(label)) feedback = PADTrySetAxisMapping(port, edit) ? "Binding updated." : "Device does not support that button; unchanged.";
                }
                ImGui::EndCombo();
              }
              ImGui::PopID();
            }
          }
          if (name) controller_shared_bindings(port);
          if (name && ImGui::Button("Restore controller defaults")) { PADRestoreDefaultMapping(port); feedback = "Controller defaults restored."; }
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }
      ImGui::TextWrapped("%s", feedback);
    }
    ImGui::End();
  }
  if (g_graphicsOpen) {
    const uint32_t selectedHeight = window::render_height();
    static const char* graphicsFeedback = "";
    static bool graphicsReported = false;
    if (ImGui::Begin("Graphics", &g_graphicsOpen)) {
      if (!graphicsReported && (std::getenv("AURORA_DIAGNOSTIC_GRAPHICS_OPEN") ||
                                std::getenv("AURORA_DIAGNOSTIC_GRAPHICS_PROBE"))) {
        std::fputs("NATIVE_GRAPHICS_PANEL drawn=1 session_only=1\n", stderr);
        graphicsReported = true;
      }
      ImGui::TextUnformatted("Changes last for this session.");
      ImGui::TextUnformatted("Display aspect ratio");
      const char* aspectLabels[]={"4:3","16:9 widescreen","16:10 widescreen","21:9 widescreen"};
      for(uint32_t i=0;i<4;++i)
        if(ImGui::RadioButton(aspectLabels[i],aurora_get_display_preset()==i))
          graphicsFeedback=aurora_set_display_preset(i)
              ? "Display preset updated." : "Unable to update display preset.";
      ImGui::TextUnformatted("Model detail and lighting");
      bool lighting=aurora_get_lighting_enabled()!=0;
      bool lowPoly=aurora_get_low_poly_fighters()!=0;
      if(ImGui::Checkbox("Low-poly fighter models",&lowPoly))
        graphicsFeedback=aurora_set_low_poly_fighters(lowPoly)
            ? "Fighter model detail updated." : "Unable to update fighter model detail.";
      if(ImGui::Checkbox("Lighting",&lighting))
        graphicsFeedback=aurora_set_lighting_enabled(lighting)
            ? "Lighting setting updated." : "Unable to update lighting.";
      ImGui::TextUnformatted("Game texture resolution");
      const uint32_t texturePercent[] = {100,50,25};
      const char* textureLabels[] = {"Original (100%)","Half dimensions (50%)","Quarter dimensions (25%)"};
      for(size_t i=0;i<3;++i)
        if(ImGui::RadioButton(textureLabels[i],aurora_get_texture_quality()==texturePercent[i]))
          graphicsFeedback=aurora_set_texture_quality(texturePercent[i])
              ? "Texture resolution updated." : "Unable to update texture resolution.";
      bool shadows = aurora_get_fighter_shadows();
      if (ImGui::Checkbox("Fighter shadows", &shadows)) {
        graphicsFeedback = aurora_set_fighter_shadows(shadows)
            ? "Fighter shadow setting updated." : "Unable to update fighter shadows.";
      }
      ImGui::TextUnformatted("Fighter shadow resolution");
      const uint32_t shadowSizes[] = {64,128,256};
      const char* shadowLabels[] = {"Low (64)","Medium (128)","Original (256)"};
      for(size_t i=0;i<3;++i) {
        if(ImGui::RadioButton(shadowLabels[i],aurora_get_fighter_shadow_resolution()==shadowSizes[i]))
          graphicsFeedback=aurora_set_fighter_shadow_resolution(shadowSizes[i])
              ? "Fighter shadow resolution updated." : "Unable to update fighter shadow resolution.";
      }
      ImGui::TextUnformatted("Lower render resolution can improve performance.");
      const uint32_t heights[] = {0, 240, 360, 480, 720, 1080, 1440, 2160, 4320};
      const char* labels[] = {"Window resolution", "240p", "360p", "480p", "720p", "1080p", "1440p", "4K (2160p)", "8K (4320p)"};
      for (size_t i = 0; i < sizeof(heights)/sizeof(heights[0]); ++i) {
        if (ImGui::RadioButton(labels[i], selectedHeight == heights[i])) {
          if (aurora_set_render_height(heights[i])) {
            graphicsFeedback = "Render resolution updated.";
          } else graphicsFeedback = "This resolution is unavailable on this device.";
        }
      }
      ImGui::TextUnformatted(graphicsFeedback);
    }
    ImGui::End();
  }
  PADSetControlsEditing(open || g_graphicsOpen);
}
} // namespace

void set_controls_available(const bool available) noexcept {
  g_controlsAvailable = available;
  if (!available) { g_controlsOpen = false; g_graphicsOpen = false; PADSetControlsEditing(false); }
}

void set_fps_counter_enabled(bool enabled) noexcept { g_fpsCounterEnabled.store(enabled); }
bool fps_counter_enabled() noexcept { return g_fpsCounterEnabled.load(); }

static void fps_counter() noexcept {
  if (!g_fpsCounterEnabled) return;
  char label[32];
  const float fps = aurora_get_fps();
  std::snprintf(label, sizeof(label), "FPS: %.1f", std::isfinite(fps) ? fps : 0.f);
  const float scale = g_scale > 0.f ? g_scale : 1.f;
  const float fontSize = 18.f * scale;
  const ImVec2 pos{10.f * scale, 10.f * scale};
  auto* draw = ImGui::GetForegroundDrawList();
  const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.f, label);
  draw->AddRectFilled({pos.x - 4.f * scale, pos.y - 2.f * scale},
                      {pos.x + extent.x + 4.f * scale, pos.y + extent.y + 2.f * scale},
                      IM_COL32(5, 10, 23, 210), 3.f * scale);
  draw->AddText(ImGui::GetFont(), fontSize, pos, IM_COL32(232, 226, 210, 255), label);
  if (std::getenv("AURORA_DIAGNOSTIC_FPS_COUNTER")) {
    static unsigned samples;
    if ((samples++ % 120) == 0)
      std::fprintf(stderr, "NATIVE_FPS_COUNTER drawn=1 fps=%.3f vertices=%d session_only=1 input_capture=0\n",
                   fps, draw->VtxBuffer.Size);
  }
}

void new_frame(const AuroraWindowSize& size) noexcept {
  ZoneScoped;
  ImVec2 framebufferScale{
      size.width > 0 ? static_cast<float>(size.native_fb_width) / static_cast<float>(size.width) : 1.0f,
      size.height > 0 ? static_cast<float>(size.native_fb_height) / static_cast<float>(size.height) : 1.0f,
  };
  ImVec2 displaySize{static_cast<float>(size.width), static_cast<float>(size.height)};

  if (g_useSdlRenderer) {
    if (SDL_Renderer* renderer = window::get_sdl_renderer()) {
      float renderScaleX = 1.0f;
      float renderScaleY = 1.0f;
      SDL_GetRenderScale(renderer, &renderScaleX, &renderScaleY);
      if (renderScaleX > 0.0f && renderScaleY > 0.0f &&
          (std::fabs(renderScaleX - 1.0f) > 0.0001f || std::fabs(renderScaleY - 1.0f) > 0.0001f)) {
        int outputWidth = static_cast<int>(size.native_fb_width);
        int outputHeight = static_cast<int>(size.native_fb_height);
        SDL_GetRenderOutputSize(renderer, &outputWidth, &outputHeight);
        displaySize = {
            static_cast<float>(outputWidth) / renderScaleX,
            static_cast<float>(outputHeight) / renderScaleY,
        };
        framebufferScale = {renderScaleX, renderScaleY};
      }
    }
    ImGui_ImplSDLRenderer3_NewFrame();
    g_scale = size.scale;
  } else {
    if (g_scale != size.scale) {
      if (g_scale > 0.f) {
        ImGui_ImplWGPU_CreateDeviceObjects();
      }
      g_scale = size.scale;
    }
    if (!ImGui::GetIO().Fonts->IsBuilt()) {
      ImGui_ImplWGPU_CreateDeviceObjects();
    }
    ImGui_ImplWGPU_NewFrame();
  }
  ImGui_ImplSDL3_NewFrame();

  ImGuiIO& io = ImGui::GetIO();
  io.DisplayFramebufferScale = framebufferScale;
  ImGui::GetIO().DisplaySize = displaySize;
  ImGui::NewFrame();
  controls_panel();
  fps_counter();
}

DrawData freeze() noexcept {
  ZoneScoped;
  ImGui::Render();

  auto* data = ImGui::GetDrawData();
  data->FramebufferScale = ImGui::GetIO().DisplayFramebufferScale;
  auto frozen = std::make_shared<DrawData::Impl>();
  frozen->drawData = *data;
  frozen->drawLists.reserve(data->CmdListsCount);
  frozen->drawData.CmdLists.resize(data->CmdListsCount);
  for (int i = 0; i < data->CmdListsCount; ++i) {
    frozen->drawLists.emplace_back(data->CmdLists[i]->CloneOutput());
    frozen->drawData.CmdLists[i] = frozen->drawLists.back().get();
  }
  return DrawData{std::move(frozen)};
}

void render(const wgpu::RenderPassEncoder& pass, const DrawData& drawData) noexcept {
  ZoneScoped;

  if (!drawData.m_impl) {
    return;
  }
  auto* data = &drawData.m_impl->drawData;
  if (data->CmdListsCount == 0) {
    return;
  }
  if (g_useSdlRenderer) {
    SDL_Renderer* renderer = window::get_sdl_renderer();
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(data, renderer);
    SDL_RenderPresent(renderer);
  } else {
    pass.PushDebugGroup("Aurora: Dear Imgui");
    ImGui_ImplWGPU_RenderDrawData(data, pass.Get());
    pass.PopDebugGroup();
  }
}

ImTextureID add_texture(uint32_t width, uint32_t height, const uint8_t* data) noexcept {
  if (SDL_Renderer* renderer = window::get_sdl_renderer()) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    SDL_UpdateTexture(texture, nullptr, data, width * 4);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    g_sdlTextures.push_back(texture);
    return reinterpret_cast<ImTextureID>(texture);
  }
  const wgpu::Extent3D size{
      .width = width,
      .height = height,
      .depthOrArrayLayers = 1,
  };
  const wgpu::TextureDescriptor textureDescriptor{
      .label = "imgui texture",
      .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
      .dimension = wgpu::TextureDimension::e2D,
      .size = size,
      .format = wgpu::TextureFormat::RGBA8Unorm,
      .mipLevelCount = 1,
      .sampleCount = 1,
  };
  const wgpu::TextureViewDescriptor textureViewDescriptor{
      .label = "imgui texture view",
      .format = wgpu::TextureFormat::RGBA8Unorm,
      .dimension = wgpu::TextureViewDimension::e2D,
      .mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED,
      .arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED,
  };
  auto texture = webgpu::g_device.CreateTexture(&textureDescriptor);
  auto textureView = texture.CreateView(&textureViewDescriptor);
  {
    const wgpu::TexelCopyTextureInfo dstView{
        .texture = texture,
    };
    const uint32_t copyBytesPerRow = AURORA_ALIGN(width * 4, 256);
    const wgpu::TexelCopyBufferLayout dataLayout{
        .bytesPerRow = copyBytesPerRow,
        .rowsPerImage = height,
    };
    enqueue_texture_upload(create_texture_upload_buffer(width, height, data, copyBytesPerRow), dstView, dataLayout,
                           size);
  }
  g_wgpuTextures.push_back(texture);
  return reinterpret_cast<ImTextureID>(textureView.MoveToCHandle());
}
} // namespace aurora::imgui

// C bindings
extern "C" {
ImTextureID aurora_imgui_add_texture(uint32_t width, uint32_t height, const void* rgba8) {
  return aurora::imgui::add_texture(width, height, static_cast<const uint8_t*>(rgba8));
}
}
