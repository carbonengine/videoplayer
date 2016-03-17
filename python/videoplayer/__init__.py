import blue
from _videoplayer import *
import trinity as _trinity


def create_textures(player, y_size, uv_size, alpha=False):
    player.y_channel = _trinity.TriTextureRes(y_size[0], y_size[1], 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    player.cu_channel = _trinity.TriTextureRes(uv_size[0], uv_size[1], 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    player.cv_channel = _trinity.TriTextureRes(uv_size[0], uv_size[1], 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    if alpha:
        player.alpha_channel = _trinity.TriTextureRes(y_size[0], y_size[1], 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    player.clear_textures()


def _create_tex_param(name, tex):
    p = _trinity.TriTextureParameter()
    p.name = name
    p.SetResource(tex)
    return p


def set_up_decode_render_job(player, rj, generate_mips=True, rt=None):
    effect = _trinity.Tr2Effect()
    effect.effectFilePath = 'res:/graphics/effect/managed/space/system/decodeycucv.fx'
    effect.resources.append(_create_tex_param('YSource', player.y_channel))
    effect.resources.append(_create_tex_param('CuSource', player.cu_channel))
    effect.resources.append(_create_tex_param('CvSource', player.cv_channel))
    if player.alpha_channel:
        effect.resources.append(_create_tex_param('AlphaSource', player.alpha_channel))
        has_alpha = _trinity.Tr2FloatParameter()
        has_alpha.name = 'HasAlpha'
        has_alpha.value = 1
        effect.parameters.append(has_alpha)
    if rt is None:
        mip_count = 0 if generate_mips else 1
        rt = _trinity.Tr2RenderTarget(player.y_channel.width, player.y_channel.height, mip_count,
                                      _trinity.PIXEL_FORMAT.B8G8R8A8_UNORM)

    rj.steps.append(_trinity.TriStepPushRenderTarget(rt))
    rj.steps.append(_trinity.TriStepPushDepthStencil(None))
    rj.steps.append(_trinity.TriStepSetStdRndStates(_trinity.RM_FULLSCREEN))
    rj.steps.append(_trinity.TriStepClear((0, 0, 0, 0)))
    rj.steps.append(_trinity.TriStepRenderEffect(effect))
    rj.steps.append(_trinity.TriStepPopDepthStencil())
    rj.steps.append(_trinity.TriStepPopRenderTarget())
    if generate_mips:
        rj.steps.append(_trinity.TriStepGenerateMipMaps(rt))
    return rt