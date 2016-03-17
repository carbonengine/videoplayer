import logging
import re
import urllib

import blue
import trinity
import audio2
import videoplayer
import decometaclass
import uthread2

_error_handlers = set()
_state_change_handlers = set()


def _create_tex_param(name, tex):
    p = trinity.TriTexture2DParameter()
    p.name = name
    p.SetResource(tex)
    return p


class VideoRenderJob(object):
    __cid__ = "trinity.TriRenderJob"
    __metaclass__ = decometaclass.BlueWrappedMetaclass

    def __init__(self):
        self.video = None
        self.rt = None
        self.weak_texture = None
        self.generate_mips = False
        self.audio_emitter = None
        self._deleted = False
        self.constructor_params = {}
        self.name = 'VideoRenderJob'

    def init(self, video_local=None, video_remote=None, generate_mips=0, **kwargs):
        if not video_local and not video_remote:
            raise ValueError()

        self.generate_mips = bool(generate_mips)

        def texture_destroyed():
            self._destroy()

        trinity.renderJobs.recurring.append(self)

        texture = trinity.TriTextureRes()
        self.weak_texture = blue.BluePythonWeakRef(texture)
        self.weak_texture.callback = texture_destroyed

        self.constructor_params = dict(kwargs)
        self.constructor_params.update({'video_local': video_local, 'video_remote': video_remote,
                                        'generate_mips': generate_mips})

        uthread2.start_tasklet(self._init, video_local, video_remote)
        return texture

    def _init(self, video_local=None, video_remote=None):
        if video_local:
            if blue.remoteFileCache.FileExists(video_local) and not blue.paths.FileExistsLocally(video_local):
                blue.paths.GetFileContentsWithYield(video_local)
                if self._deleted:
                    return
            stream = blue.paths.open(video_local, 'rb')
        elif video_remote:
            stream = blue.BlueNetworkStream(video_remote)
        else:
            raise ValueError()
        self.video = videoplayer.VideoPlayer(stream, None)
        self.video.on_state_change = self._on_state_change
        self.video.on_create_textures = self._on_video_info_ready
        self.video.on_error = self._on_error
        self.rt = None

    def _on_state_change(self, player):
        logging.info('Video player state changed to %s', videoplayer.State.GetNameFromValue(player.state))
        for each in _state_change_handlers:
            each(player, self.constructor_params, self.weak_texture.object)

    def _on_error(self, player):
        try:
            player.validate()
        except RuntimeError as e:
            logging.exception('Video player error')
            for each in _error_handlers:
                each(player, e, self.constructor_params, self.weak_texture.object)


    def _on_video_info_ready(self, player, y_size, uv_size, alpha):
        self.steps.removeAt(-1)

        videoplayer.create_textures(self.video, y_size, uv_size, alpha)
        self.rt = videoplayer.set_up_decode_render_job(self.video, self, self.generate_mips)

        def update_texture():
            self.weak_texture.object.SetFromRenderTarget(self.rt)

        self.steps.append(trinity.TriStepPythonCB(update_texture))

    def _destroy(self):
        self._deleted = True
        trinity.renderJobs.recurring.remove(self)
        self.steps.removeAt(-1)
        if self.video:
            self.video.on_state_change = None
            self.video.on_create_textures = None
            self.video.on_error = None
        self.video = None
        self.audio_emitter = None
        self.rt = None
        if self.weak_texture is not None:
            self.weak_texture.callback = None


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


def register_error_handler(error_handler):
    _error_handlers.add(error_handler)


def unregister_error_handler(error_handler):
    _error_handlers.remove(error_handler)


def register_state_change_handler(state_change_handler):
    _state_change_handlers.add(state_change_handler)


def unregister_state_change_handler(state_change_handler):
    _state_change_handlers.remove(state_change_handler)


