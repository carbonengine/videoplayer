import logging
import re
import urllib

import blue
import trinity
import audio2
import videoplayer
import decometaclass


def _create_tex_param(name, tex):
    p = trinity.TriTexture2DParameter()
    p.name = name
    p.SetResource(tex)
    return p


def _log_state_change(player):
    logging.log('Video player state changed to %s', videoplayer.State.GetNameFromValue(player.state))


def _log_error(player):
    try:
        player.validate()
    except RuntimeError as e:
        logging.exception('Video player error')


class VideoRenderJob(object):
    __cid__ = "trinity.TriRenderJob"
    __metaclass__ = decometaclass.BlueWrappedMetaclass

    def __init__(self):
        self.video = None
        self.rt = None
        self.weak_texture = None
        self.generate_mips = False
        self.audio_emitter = None

    def init(self, video_local=None, video_remote=None, generate_mips=0, **kwargs):
        self.generate_mips = bool(generate_mips)
        if video_local:
            stream = blue.paths.open(video_local, 'rb')
        elif video_remote:
            stream = blue.BlueNetworkStream(video_remote)
        else:
            raise ValueError()
        audio = sm.GetService('audio')
        self.audio_emitter, channel = audio.GetAudioBus(rate=48000)
        self.video = videoplayer.VideoPlayer(stream, videoplayer.Audio2Sink(audio2.GetDirectSoundPtr(), channel))
        self.video.on_state_change = _log_state_change
        self.video.on_create_textures = self._on_video_info_ready
        self.video.on_error = _log_error
        self.rt = None

        def texture_destroyed():
            self._destroy()

        trinity.renderJobs.recurring.append(self)

        texture = trinity.TriTextureRes()
        self.weak_texture = blue.BluePythonWeakRef(texture)
        self.weak_texture.callback = texture_destroyed
        return texture

    def _on_video_info_ready(self, y_size, uv_size):
        self.steps.removeAt(-1)

        videoplayer.create_textures(self.video, y_size, uv_size)
        self.rt = videoplayer.set_up_decode_render_job(self.video, self, self.generate_mips)

        def update_texture():
            self.weak_texture.object.SetFromRenderTarget(self.rt)

        self.steps.append(trinity.TriStepPythonCB(update_texture))

    def _destroy(self):
        trinity.renderJobs.recurring.remove(self)
        self.video = None
        self.audio_emitter = None
        self.rt = None


def _url_to_dict(param_string):
    params = {}
    expr = re.compile(r'\?((\w+)=([^&]*))(&?(\w+)=([^&]*))*')
    match = expr.match(param_string)
    if match:
        for i in xrange(1, len(match.groups()), 3):
            if match.group(i) is not None:
                params[match.group(i + 1)] = str(urllib.unquote(match.group(i + 2)))
    return params


def play_video(param_string):
    # noinspection PyBroadException
    try:
        rj = VideoRenderJob()
        return rj.init(**_url_to_dict(param_string))
    except:
        logging.exception('Exception in video resource constructor')


def register_resource_constructor(name='video'):
    blue.resMan.RegisterResourceConstructor(name, play_video)
