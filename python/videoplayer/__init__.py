import blue
from _videoplayer import *
import trinity as _trinity


def create_textures(player):
    info = player.get_video_info()
    width = info['width']
    height = info['height']
    half_width = width / 2
    half_height = height / 2
    player.y_channel = _trinity.TriTextureRes(width, height, 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    player.cu_channel = _trinity.TriTextureRes(half_width, half_height, 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    player.cv_channel = _trinity.TriTextureRes(half_width, half_height, 1, _trinity.PIXEL_FORMAT.R8_UNORM)
    player.clear_textures()


def _create_tex_param(name, tex):
    p = _trinity.TriTextureParameter()
    p.name = name
    p.SetResource(tex)
    return p


def set_up_decode_render_job(player, rj, generate_mips=True):
    effect = _trinity.Tr2Effect()
    effect.effectFilePath = 'res:/graphics/effect/managed/space/system/decodeycucv.fx'
    effect.resources.append(_create_tex_param('YSource', player.y_channel))
    effect.resources.append(_create_tex_param('CuSource', player.cu_channel))
    effect.resources.append(_create_tex_param('CvSource', player.cv_channel))

    mip_count = 0 if generate_mips else 1
    rt = _trinity.Tr2RenderTarget(player.y_channel.width, player.y_channel.height, mip_count,
                                 _trinity.PIXEL_FORMAT.B8G8R8A8_UNORM)

    rj.steps.append(_trinity.TriStepPushRenderTarget(rt))
    rj.steps.append(_trinity.TriStepPushDepthStencil(None))
    rj.steps.append(_trinity.TriStepSetStdRndStates(_trinity.RM_FULLSCREEN))
    rj.steps.append(_trinity.TriStepClear((1, 1, 0, 1)))
    rj.steps.append(_trinity.TriStepRenderEffect(effect))
    rj.steps.append(_trinity.TriStepPopDepthStencil())
    rj.steps.append(_trinity.TriStepPopRenderTarget())
    if generate_mips:
        rj.steps.append(_trinity.TriStepGenerateMipMaps(rt))
    return rt