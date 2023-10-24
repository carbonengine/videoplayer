import logging
import re
import urllib.parse

import blue
import trinity
import videoplayer
import uthread2

_error_handlers = set()
_state_change_handlers = set()


class _VideoController(object):
    def __init__(self):
        self.video = None
        self.weak_texture = None
        self.generate_mips = False
        self._deleted = False
        self.constructor_params = {}

    def init(self, video_local=None, video_remote=None, generate_mips=0, **kwargs):
        if not video_local and not video_remote:
            raise ValueError()

        self.generate_mips = bool(generate_mips)

        def texture_destroyed():
            self._destroy()

        texture = trinity.TriTextureRes()
        self.weak_texture = blue.BluePythonWeakRef(texture)
        self.weak_texture.callback = texture_destroyed

        self.constructor_params = dict(kwargs)
        self.constructor_params.update({'video_local': video_local, 'video_remote': video_remote,
                                        'generate_mips': generate_mips})

        uthread2.start_tasklet(self._init, video_local, video_remote)
        return texture

    def _init(self, video_local=None, video_remote=None):
        if self._deleted:
            return
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
        self.video.bgra_texture = self.weak_texture.object
        self.video.on_state_change = self._on_state_change
        self.video.on_create_textures = self._on_video_info_ready
        self.video.on_error = self._on_error

    def _on_state_change(self, player):
        logging.debug('Video player state changed to %s', videoplayer.State.GetNameFromValue(player.state))
        for each in _state_change_handlers:
            each(player, self.constructor_params, self.weak_texture.object)

    def _on_error(self, player):
        try:
            player.validate()
        except RuntimeError as e:
            logging.exception('Video player error')
            for each in _error_handlers:
                each(player, e, self.constructor_params, self.weak_texture.object)

    def _on_video_info_ready(self, _, width, height):
        if self.weak_texture.object:
            self.weak_texture.object.CreateEmptyTexture(width, height, 1, trinity.PIXEL_FORMAT.B8G8R8A8_UNORM)

    def _destroy(self):
        self._deleted = True
        if self.video:
            self.video.bgra_texture = None
            self.video.on_state_change = None
            self.video.on_create_textures = None
            self.video.on_error = None
        self.video = None
        if self.weak_texture is not None:
            self.weak_texture.callback = None


def _url_to_dict(param_string):
    params = {}
    expr = re.compile(r'\?((\w+)=([^&]*))(&?(\w+)=([^&]*))*')
    match = expr.match(param_string)
    if match:
        for i in range(1, len(match.groups()), 3):
            if match.group(i) is not None:
                params[match.group(i + 1)] = str(urllib.parse.unquote(match.group(i + 2)))
    return params


def play_video(param_string):
    # noinspection PyBroadException
    try:
        rj = _VideoController()
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
