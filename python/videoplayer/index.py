import json
import os
from urllib.request import urlopen

import uthread2
from eveprefs import prefs


class VideoIndex(object):
    def __init__(self, url, language_id=None):
        if language_id is None:
            try:
                # noinspection PyUnresolvedReferences
                self._language_id = str(prefs.languageID)
            except (NameError, AttributeError):
                self._language_id = 'EN'
        else:
            self._language_id = language_id
        if self._language_id == 'ZH':
            self._language_id = 'CN'
        self._url = url
        if self._url.lower().startswith(u'http'):
            self._base_url = u'/'.join(self._url.split(u'/')[0:-1])
            if not self._base_url.endswith(u'/'):
                self._base_url += u'/'
        else:
            self._base_url = os.path.dirname(self._url)
            if not self._base_url.endswith(os.path.sep):
                self._base_url += os.path.sep
        self._data = None
        self._videos = {}
        uthread2.start_tasklet(self._load_index)

    def _load_index(self):
        try:
            if self._url.lower().startswith('http'):
                opener = urlopen
            else:
                opener = open
            stream = opener(self._url)
            try:
                self._data = json.load(stream)
            finally:
                stream = None
            if not isinstance(self._data, dict):
                raise TypeError('VideoIndex: data loaded from \"%s\" is not a dict' % self._url)
            for group in self._data.get('groups', []):
                for video in group.get('videos', []):
                    if video.get('id'):
                        self._videos[video['id']] = video
        except:
            self._data = {}
            raise

    def _wait_for_load(self):
        while self._data is None:
            uthread2.yield_()

    def _get_localized_string(self, item):
        if self._language_id in item:
            return item[self._language_id]
        elif 'EN' in item:
            return item['EN']

    def get_groups(self):
        self._wait_for_load()
        result = []
        for i, group in enumerate(self._data.get('groups', [])):
            title = self._get_localized_string(group.get('title', {}))
            result.append((i, title))
        return result

    def _build_url(self, url):
        if not url:
            return None
        return self._base_url + url

    def _get_video_item(self, video):
        return {'id': video.get('id'),
                'title': self._get_localized_string(video.get('title', {})),
                'fullTitle': self._get_localized_string(video.get('full_title', {})),
                'description': self._get_localized_string(video.get('description', {})),
                'url': self._build_url(video.get('url', '')),
                'subtitles': self._build_url(self._get_localized_string(video.get('subtitles', {})))}

    def get_videos_in_group(self, group):
        self._wait_for_load()
        return [self._get_video_item(video) for video in self._data.get('groups', [])[group].get('videos', [])]

    def get_video_by_id(self, video_id):
        self._wait_for_load()
        if video_id in self._videos:
            return self._get_video_item(self._videos[video_id])

    def get_related(self, videoId):
        self._wait_for_load()
        related = []
        if videoId in self._videos:
            for each in self._videos[videoId].get('related', []):
                video = self.get_video_by_id(each)
                if video:
                    related.append(video)
        return related
