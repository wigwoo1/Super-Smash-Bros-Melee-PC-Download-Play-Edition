#include <sysdolphin/baselib/initialize.h>
/* Camera queries share the state written by native HSD_StartRender. */
HSD_RenderPass MeleePc_CurrentRenderPass;
HSD_RenderPass HSD_GetCurrentRenderPass(void)
{
    return MeleePc_CurrentRenderPass;
}
