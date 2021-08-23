"""
Test player application that uses videoplayer to play webm videos
"""

import argparse
import os
import sys
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'carbon', 'tools', 'lib', 'bin', 'x64')))

import wx

def get_path(wildcard):
    style = wx.FD_OPEN | wx.FD_FILE_MUST_EXIST
    dialog = wx.FileDialog(None, 'Open', wildcard=wildcard, style=style)
    try:
        if dialog.ShowModal() == wx.ID_OK:
            return dialog.GetPath()
        else:
            return None
    finally:
        dialog.Destroy()

wxapp = wx.App()

import audio2
import blue
import eveaudio
import trinity
import uthread2
import videoplayer
from trinity.trinityapp import TrinityApp
from eveaudio.audiomanager import CreateAudioSettings, AudioManager 

WWISE_ARG = "wwise"
WAVE_ARG = "wave"
VALID_ARGS = [WWISE_ARG, WAVE_ARG]

parser = argparse.ArgumentParser(description="Application to test the videoplayer with webm videos.")
parser.add_argument("--videopath", type=str, help="A path to a video. If not given then an explorer window will open.")
parser.add_argument("--audio", required=True, type=str, help="One of {} which will determine how audio is handled when playing the video.".format(VALID_ARGS))

class Controls(wx.Frame):
    def __init__(self, video):
        super(Controls, self).__init__(None, title='Video Controls')
        self.video = video
        self._update_slider = True
        video.on_state_change = self.on_video_state

        layout = wx.BoxSizer(wx.HORIZONTAL)
        self.status = wx.StaticText(self)
        self.status.SetMinSize((100, 10))
        layout.Add(self.status)
        play = wx.Button(self, label='Play')
        play.Bind(wx.EVT_BUTTON, self.on_play)
        layout.Add(play)
        pause = wx.Button(self, label='Pause')
        pause.Bind(wx.EVT_BUTTON, self.on_pause)
        layout.Add(pause)
        self.slider = wx.Slider(self)
        self.slider.Bind(wx.EVT_COMMAND_SCROLL_THUMBTRACK, self.on_slider_tracking)
        self.slider.Bind(wx.EVT_COMMAND_SCROLL_CHANGED, self.on_slider)
        layout.Add(self.slider)
        self.time = wx.StaticText(self)
        self.time.SetMinSize((100, 10))
        layout.Add(self.time)
        layout.Add(wx.StaticText(self, label='/'))
        self.duration = wx.StaticText(self)
        self.duration.SetMinSize((100, 10))
        layout.Add(self.duration)
        self.SetSizerAndFit(layout)

        uthread2.StartTasklet(self.update_time)

    def on_play(self, _):
        self.video.resume()

    def on_pause(self, _):
        self.video.pause()

    def on_video_state(self, player):
        self.status.SetLabelText(videoplayer.State.GetNameFromValue(player.state))
        self.duration.SetLabelText('%s' % (player.duration / 1000000000))

    def on_slider_tracking(self, _):
        self._update_slider = False

    def on_slider(self, evt):
        self._update_slider = True
        if self.video.duration:
            self.video.seek(long(float(self.slider.GetValue()) / 100.0 * self.video.duration))

    def update_time(self):
        while True:
            t = '%s' % (self.video.media_time / 1000000000)
            if self.time.GetLabelText() != t:
                self.time.SetLabelText('%s' % (self.video.media_time / 1000000000))
            if self.video.duration and not wx.GetMouseState().LeftDown():
                x = int(float(self.video.media_time / 1000000) / (self.video.duration / 1000000) * 100)
                if self._update_slider and self.slider.GetValue() != x:
                    self.slider.SetValue(x)
            uthread2.Yield()


def init_wx(video):
    controls = Controls(video)
    controls.Show()

    event_loop = wx.EventLoop()
    wx.EventLoop.SetActive(event_loop)

    def process_tasklet():
        while True:
            while event_loop.Pending():
                event_loop.Dispatch()
                wxapp.ProcessPendingEvents()
            wxapp.ProcessIdle()
            uthread2.Yield()
    uthread2.StartTasklet(process_tasklet)


def _on_video_info_ready(_, width, height):
    texture.__init__(width, height, 1, trinity.PIXEL_FORMAT.B8G8R8A8_UNORM)

args = parser.parse_args()
if args.videopath:
    if os.path.isfile(args.videopath):
        path = args.videopath 
    else:
        print("{} is not a valid path to a .webm file".format(args.videopath))
        sys.exit(1)
else:
    path = get_path('*.webm')
if not path:
    sys.exit(1)
    state = trinity.Tr2MainWindowState()
    state.windowMode = trinity.Tr2WindowMode.WINDOWED
    state.width = width
    state.height = height
    trinity.mainWindow.SetWindowState(state)


video = None 
# If I place the creation of the video player inside a function it doesn't play the video.
if args.audio == WAVE_ARG:
    video = videoplayer.VideoPlayer(blue.paths.open(path, 'rb'), videoplayer.WaveOutAudioSink())
elif args.audio == WWISE_ARG:
    blue.paths.SetSearchPath("res", os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "eve", "client", "res")))
    resAudio = "res:/Audio/"
    settings = CreateAudioSettings(resAudio, "en", "Videoplayer")
    manager = AudioManager()
    manager.Enable(settings, eveaudio.EVE_COMMON_BANKS)
    uiPlayer = audio2.GetUIPlayer()
    inputMgr = audio2.AudioInputMgr()
    sink = videoplayer.WwiseAudioSink(inputMgr)
    video = videoplayer.VideoPlayer(blue.paths.open(path, 'rb'), sink, 0, True)

texture = trinity.TriTextureRes()
video.bgra_texture = texture
video.on_create_textures = _on_video_info_ready

app = TrinityApp(pauseOnDeactivate=False)

init_wx(video)

rj = trinity.TriRenderJob()
rj.steps.append(trinity.TriStepSetStdRndStates(trinity.RM_ALPHA))
rj.steps.append(trinity.TriStepClear((0.3, 0.3, 0.3, 1), 1.0))
rj.steps.append(trinity.TriStepSetRenderState(27, 1))
rj.steps.append(trinity.TriStepRenderTexture(texture))
trinity.renderJobs.recurring.append(rj)
app.exec_()