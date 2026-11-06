#ifndef MELEE_PC_RENDERER_SESSION_H
#define MELEE_PC_RENDERER_SESSION_H

#include "frame_clock.h"
#include "game_prefs_session.h"
/* Same-build private process payload. No pointers, game assets or save state. */
typedef struct {
    unsigned active;
    PADKeyButtonBinding buttons[PAD_BUTTON_COUNT];
    PADKeyAxisBinding axes[PAD_AXIS_COUNT];
} MeleePc_RendererKeyboard;
typedef struct {
    unsigned magic, version, size;
    unsigned display, height, shadows, shadow_size, quality, lighting;
    unsigned low_poly, pack, fps, hz, audio_channels, upmix;
    MeleePc_RendererKeyboard keyboard[PAD_CHANMAX];
    MeleePc_GamePrefsTransfer game_prefs;
} MeleePc_RendererSession;
static unsigned renderer_session_audio_channels, renderer_session_upmix;

static unsigned renderer_session_capture(void* output, unsigned capacity)
{
    MeleePc_RendererSession state;
    unsigned port;
    if(capacity<sizeof(state) || aurora_is_texture_pack_pending()) return 0;
    memset(&state,0,sizeof(state));
    state.magic=0x4d525331;state.version=2;state.size=sizeof(state);
    if(!MeleePc_GamePrefsDiagnosticSeed() ||
       !MeleePc_GamePrefsCapture(&state.game_prefs)) return 0;
    state.display=aurora_get_display_preset();state.height=aurora_get_render_height();
    state.shadows=aurora_get_fighter_shadows();state.shadow_size=aurora_get_fighter_shadow_resolution();
    state.quality=aurora_get_texture_quality();state.lighting=aurora_get_lighting_enabled();
    state.low_poly=aurora_get_low_poly_fighters();state.pack=aurora_get_texture_pack();
    state.fps=aurora_get_fps_counter_enabled();state.hz=MeleePc_GetGameplayPresentationRate();
    state.audio_channels=aurora_ax_requested_output_channels();state.upmix=aurora_ax_stereo_upmix_enabled();
    for(port=0;port<PAD_CHANMAX;port++) {
        u32 buttons=0,axes=0;
        PADKeyButtonBinding* b=PADGetKeyButtonBindings(port,&buttons);
        PADKeyAxisBinding* a=PADGetKeyAxisBindings(port,&axes);
        if(!b && !a) continue;
        if(!b || !a || buttons!=PAD_BUTTON_COUNT || axes!=PAD_AXIS_COUNT) return 0;
        state.keyboard[port].active=1;
        memcpy(state.keyboard[port].buttons,b,sizeof(state.keyboard[port].buttons));
        memcpy(state.keyboard[port].axes,a,sizeof(state.keyboard[port].axes));
    }
    memcpy(output,&state,sizeof(state));
    return sizeof(state);
}

static int renderer_session_restore(const void* input, unsigned size)
{
    MeleePc_RendererSession state;
    unsigned port;
    if(size!=sizeof(state)) return 0;
    memcpy(&state,input,sizeof(state));
    if(state.magic!=0x4d525331 || state.version!=2 || state.size!=sizeof(state) ||
       state.display>3 || state.shadows>1 || state.lighting>1 || state.low_poly>1 ||
       state.pack>1 || state.fps>1 || state.upmix>1 ||
       (state.audio_channels!=0 && state.audio_channels!=2 && state.audio_channels!=6)) return 0;
    for(port=0;port<PAD_CHANMAX;port++) if(state.keyboard[port].active>1) return 0;
    if(!aurora_set_display_preset(state.display) || !aurora_set_render_height(state.height) ||
       !aurora_set_fighter_shadows(state.shadows) || !aurora_set_fighter_shadow_resolution(state.shadow_size) ||
       !aurora_set_texture_quality(state.quality) || !aurora_set_lighting_enabled(state.lighting) ||
       !aurora_set_low_poly_fighters(state.low_poly) || !aurora_set_texture_pack(state.pack) ||
       !aurora_set_fps_counter_enabled(state.fps) || !MeleePc_SetGameplayPresentationRate(state.hz)) return 0;
    for(port=0;port<PAD_CHANMAX;port++) {
        if(!state.keyboard[port].active) PADClearKeyBindings(port);
        else if(!PADInstallKeyboardPreset(port) ||
                !PADTrySetKeyboardProfile(port,state.keyboard[port].buttons,state.keyboard[port].axes)) return 0;
    }
    if(!MeleePc_GamePrefsQueue(&state.game_prefs)) return 0;
    renderer_session_audio_channels=state.audio_channels;
    renderer_session_upmix=state.upmix;
    fprintf(stderr,"NATIVE_RENDERER_SESSION restored=1 bytes=%u aspect=%u render_height=%u shadows=%u shadow_size=%u texture_quality=%u lighting=%u low_poly=%u fps=%u hz=%u keyboard_ports=%u%u%u%u audio_requested=%u\n",
            size,state.display,state.height,state.shadows,state.shadow_size,state.quality,state.lighting,state.low_poly,state.fps,state.hz,
            state.keyboard[0].active,state.keyboard[1].active,state.keyboard[2].active,state.keyboard[3].active,state.audio_channels);
    return 1;
}

/* Defer AX preferences until the original game has initialized the stream.
 * Do not block the initial renderer handshake: the hidden child cannot finish
 * AX startup while window pausing is active. Retry after it becomes visible. */
static int renderer_session_service(void)
{
    if(!renderer_session_audio_channels) return 1;
    if(!aurora_ax_source_rate()) return 0;
    if(!aurora_ax_request_output_channels(renderer_session_audio_channels) ||
       !aurora_ax_set_stereo_upmix(renderer_session_upmix)) return -1;
    fprintf(stderr,"NATIVE_RENDERER_SESSION audio_restored=1 requested=%u upmix=%u\n",
            renderer_session_audio_channels,renderer_session_upmix);
    renderer_session_audio_channels=0;
    return 1;
}
#endif
