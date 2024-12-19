import logging
import random
import re
import weakref

import audio2
import blue
import trinity
import videoplayer
import uthread2
import evegraphics.settings as gfxsettings

_error_handlers = set()
_state_change_handlers = set()
_play_list_finished_handlers = set()


def _is_low_quality():
    return trinity.GetShaderModel() == 'SM_3_0_LO'


class _VideoPlaylistController(object):
    __notifyevents__ = [
        'OnGraphicSettingsChanged'
    ]

    def __init__(self):
        self.video = None
        self.weak_texture = None
        self.generate_mips = False
        self.constructor_params = {}
        self.playlist = None
        self.low_quality_texture_path = None
        self.destroyed = False
        self.current_path = None

        try:
            sm.RegisterForNotifyEvent(self, "OnGraphicSettingsChanged")
        except NameError:
            pass  # sm doesn't exist in evemark

    def init(self, width, height, playlist, low_quality_texture_path=None, **kwargs):
        self.destroyed = False
        self.low_quality_texture_path = low_quality_texture_path
        self.generate_mips = kwargs.pop('generate_mips', False)
        self.constructor_params = kwargs
        self.playlist = playlist(**kwargs)

        def texture_destroyed():
            self._destroy()
        texture = trinity.TriTextureRes()
        self.weak_texture = blue.BluePythonWeakRef(texture)
        self.weak_texture.callback = texture_destroyed

        if not self.low_quality_texture_path or not _is_low_quality():
            uthread2.start_tasklet(self.play_next)
        else:
            self._create_low_quality_render_job()

        def check_focus():
            while not self.destroyed:
                if self.video:
                    if trinity.app.IsHidden():
                        if not self.video.is_paused:
                            logging.debug('Pausing videos as the window is minimized')
                            self.video.pause()
                    else:
                        if self.video.is_paused:
                            logging.debug('Resuming videos as the window is not minimized anymore')
                            self.video.resume()
                blue.synchro.SleepWallclock(300)

        uthread2.start_tasklet(check_focus)

        return texture

    def _create_low_quality_render_job(self):
        self.current_path = None
        lq = blue.resMan.GetResource(self.low_quality_texture_path)

        def check():
            while True:
                if self.destroyed or not self.low_quality_texture_path or not _is_low_quality():
                    return
                if lq.isGood:
                    try:
                        self.weak_texture.object.CreateFromTexture(lq)
                    except:
                        pass
                    return
                blue.synchro.SleepWallclock(100)

        uthread2.start_tasklet(check)

    def play_next(self):
        if self.destroyed:
            return
        try:
            item = next(self.playlist)
        except StopIteration:
            self.current_path = None
            for each in _play_list_finished_handlers:
                each(self.constructor_params, self.weak_texture.object)
            self._destroy()
            return
        if item.lower().startswith('http'):
            stream = blue.BlueNetworkStream(item)
        else:
            if blue.remoteFileCache.FileExists(item) and not blue.paths.FileExistsLocally(item):
                blue.paths.GetFileContentsWithYield(item)
            if self.destroyed:
                return
            stream = blue.paths.open(item, 'rb')

        self.video = videoplayer.VideoPlayer(stream, None)
        self.video.bgra_texture = self.weak_texture.object
        self.video.on_state_change = self._on_state_change
        self.video.on_create_textures = self._on_video_info_ready
        self.video.on_error = self._on_error
        self.current_path = item

    def _on_state_change(self, player):
        if self.destroyed:
            return
        logging.debug('Video player state changed to %s', videoplayer.State.GetNameFromValue(player.state))
        for each in _state_change_handlers:
            each(player, self.constructor_params, self.weak_texture.object)
        if player.state == videoplayer.State.DONE:
            uthread2.start_tasklet(self.play_next)

    def _on_error(self, player):
        try:
            player.validate()
        except RuntimeError as e:
            logging.exception('Video player error')
            for each in _error_handlers:
                each(player, e, self.constructor_params, self.weak_texture.object)
            uthread2.start_tasklet(self.play_next)

    def _on_video_info_ready(self, _, width, height):
        if self.weak_texture.object:
            self.weak_texture.object.CreateEmptyTexture(width, height, 1, trinity.PIXEL_FORMAT.B8G8R8A8_UNORM)

    def _destroy(self):
        if self.video:
            self.video.bgra_texture = None
            self.video.on_state_change = None
            self.video.on_create_textures = None
            self.video.on_error = None
        self.video = None
        self.destroyed = True
        if self.weak_texture is not None:
            self.weak_texture.callback = None

    def OnGraphicSettingsChanged(self, changes):
        changed = gfxsettings.GFX_TEXTURE_QUALITY in changes or gfxsettings.GFX_SHADER_QUALITY in changes
        if changed:
            uthread2.start_tasklet(self.play_next)


class _VideoPlaylistControllerWithSound(_VideoPlaylistController):
    def play_next(self):
        if self.destroyed:
            return
        try:
            item = next(self.playlist)
        except StopIteration:
            self.current_path = None
            for each in _play_list_finished_handlers:
                each(self.constructor_params, self.weak_texture.object)
            self._destroy()
            return
        if item.lower().startswith('http'):
            stream = blue.BlueNetworkStream(item)
        else:
            if blue.remoteFileCache.FileExists(item) and not blue.paths.FileExistsLocally(item):
                blue.paths.GetFileContentsWithYield(item)
            if self.destroyed:
                return
            stream = blue.paths.open(item, 'rb')

        inputMgr = audio2.AudioInputMgr()
        sink = videoplayer.WwiseAudioSink(inputMgr)
        self.video = videoplayer.VideoPlayer(stream, sink, 0, True)

        self.video.bgra_texture = self.weak_texture.object
        self.video.on_state_change = self._on_state_change
        self.video.on_create_textures = self._on_video_info_ready
        self.video.on_error = self._on_error
        self.current_path = item


_hexdig = '0123456789ABCDEFabcdef'
_hextochr = dict((a + b, chr(int(a + b, 16)))
                 for a in _hexdig for b in _hexdig)


def _unquote(s):
    """unquote('abc%20def') -> 'abc def'."""
    res = s.split('%')
    # fastpath
    if len(res) == 1:
        return s
    s = res[0]
    for item in res[1:]:
        try:
            s += _hextochr[item[:2]] + item[2:]
        except KeyError:
            s += '%' + item
        except UnicodeDecodeError:
            s += unichr(int(item[:2], 16)) + item[2:]
    return s


def _url_to_dict(param_string):
    params = {}
    expr = re.compile(r'\?((\w+)=([^&]*))(&?(\w+)=([^&]*))*')
    match = expr.match(param_string)
    if match:
        for i in range(1, len(match.groups()), 3):
            if match.group(i) is not None:
                params[match.group(i + 1)] = str(_unquote(match.group(i + 2)))
    return params


_video_controllers = weakref.WeakValueDictionary()


def register_resource_constructor(name, width, height, playlist, low_quality_texture_path=None, withSound=False):
    """
    Registers a dynamic resource handler to play videos from a playlist

    :param name: name of dynamic resource, after registering resources can be requested using dynamic:/name path
    :param width: width of the resulting texture in pixels
    :param height: height of the resulting texture in pixels
    :param playlist: function returning a generator yielding paths to videos, called with dynamic resource keyword
        arguments
    :param low_quality_texture_path: fallback texture path to display with SM_LO, use None to have no fallback
    """

    def play(param_string):
        # noinspection PyBroadException
        try:
            if withSound:
                rj = _VideoPlaylistControllerWithSound()
            else:
                rj = _VideoPlaylistController()
            _video_controllers['dynamic:/%s%s' % (name, param_string)] = rj
            return rj.init(width, height, playlist, low_quality_texture_path, **_url_to_dict(param_string))
        except:
            logging.exception('Exception in video playlist resource constructor')
    blue.resMan.RegisterResourceConstructor(name, play)


def unregister_resource_constructor(name):
    blue.resMan.UnregisterResourceConstructor(name)


def register_error_handler(error_handler):
    """
    Registers an error handler that is called when there is an error playing video
    :param error_handler: function that is called when there's an error during video playback, called with arguments
        video player instance, exception object, dynamic constructor parameters, resulting texture object
    """
    _error_handlers.add(error_handler)


def unregister_error_handler(error_handler):
    """
    Unregisters error handler registered with register_error_handler
    :param error_handler: callback passed to register_error_handler
    """
    _error_handlers.remove(error_handler)


def register_state_change_handler(state_change_handler):
    """
    Registers function called when video player state changes. Function is called with arguments: video player instance,
        exception object, dynamic constructor parameters, resulting texture object
    :param state_change_handler: function called when video player state changes
    """
    _state_change_handlers.add(state_change_handler)


def unregister_state_change_handler(state_change_handler):
    """
    Unregisters state change handler registered with register_state_change_handler
    :param state_change_handler: callback passed to register_state_change_handler
    """
    _state_change_handlers.remove(state_change_handler)


def register_playlist_finished_handler(playlist_finished_handler):
    """
    Registers function called when playlist has finished playing. Function is called with arguments: dynamic constructor
        parameters, resulting texture object
    :param playlist_finished_handler: function called when video player state changes
    """
    _play_list_finished_handlers.add(playlist_finished_handler)


def unregister_playlist_finished_handler(playlist_finished_handler):
    """
    Unregisters on-finish handler registered with register_playlist_finished_handler
    :param playlist_finished_handler: callback passed to register_playlist_finished_handler
    """
    _play_list_finished_handlers.remove(playlist_finished_handler)


def get_currently_played_video(res_path):
    """
    Returns currently played video res path for the given playlist res path.
    :param res_path: dynamic res path for the playlist
    :type res_path: str
    :return: res path for the currently played video or None if the video is not playing
    :rtype: str
    :raises KeyError: if the playlist path is not currently used
    """
    video = _video_controllers.get(res_path, None)
    if video is None:
        return None
    return video.current_path


def shuffled_videos(*res_path):
    """
    Sample playlist generator: yields shuffled list of video paths

    :param res_path: res paths to webm videos or directories containing videos
    :return: an infinite generator yielding paths to videos in random order
    """
    paths = []
    for path in res_path:
        if not path.lower().startswith('http') and blue.paths.isdir(path):
            for each in blue.paths.listdir(path):
                if each.lower().endswith('.webm'):
                    paths.append('%s/%s' % (path, each))
        elif path.lower().startswith('http') or blue.paths.exists(path):
            paths.append(path)

    # noinspection PyUnusedLocal
    def inner(**kwargs):
        random.shuffle(paths)
        index = 0
        while True:
            yield paths[index]
            index = (index + 1) % len(paths)
    return inner
