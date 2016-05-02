"""
Test player application that uses videoplayer to play webm videos
"""

import os
import sys
import wx


def get_path(wildcard):
    app = wx.App(None)
    style = wx.FD_OPEN | wx.FD_FILE_MUST_EXIST
    dialog = wx.FileDialog(None, 'Open', wildcard=wildcard, style=style)
    try:
        if dialog.ShowModal() == wx.ID_OK:
            return dialog.GetPath()
        else:
            return None
    finally:
        dialog.Destroy()

if len(sys.argv) > 1:
    path = sys.argv[1]
else:
    path = get_path('*.webm')
if not path:
    sys.exit()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import binbootstrapper
binbootstrapper.update_binaries(__file__, binbootstrapper.DLL_VIDEOPLAYER, binbootstrapper.DLL_AUDIO, *binbootstrapper.DLLS_GRAPHICS)

import blue
import trinity
from videoplayer.resource import register_resource_constructor, register_state_change_handler
from binbootstrapper.trinityapp import TrinityApp


def resizer(player, *args, **kwargs):
    try:
        info = player.get_video_info()
    except RuntimeError:
        return
    trinity.app.width = info['width']
    trinity.app.height = info['height']
    trinity.app.MoveWindow(trinity.app.left, trinity.app.top, trinity.app.width, trinity.app.height)


app = TrinityApp()
register_resource_constructor()
register_state_change_handler(resizer)
texture = blue.resMan.GetResource('dynamic:/video/?video_local=%s&test_audio=1' % path)
rj = trinity.TriRenderJob()
rj.steps.append(trinity.TriStepSetStdRndStates(trinity.RM_ALPHA))
rj.steps.append(trinity.TriStepClear((0.3, 0.3, 0.3, 1), 1.0))
rj.steps.append(trinity.TriStepSetRenderState(27, 1))
rj.steps.append(trinity.TriStepRenderTexture(texture))
trinity.renderJobs.recurring.append(rj)
app.exec_()