# -*- coding: utf-8 -*-
#
# Copyright (c) 2015, Thibault Saunier
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA 02110-1301, USA.

import gi

gi.require_version("Gst", "1.0")
gi.require_version("GES", "1.0")

from gi.repository import Gst  # noqa
from gi.repository import GES  # noqa
import unittest  # noqa

Gst.init(None)
GES.init()


class TestCopyPaste(unittest.TestCase):

    def setUp(self):
        self.timeline = GES.Timeline.new_audio_video()
        self.assertEqual(len(self.timeline.get_tracks()), 2)
        self.layer = self.timeline.append_layer()

    def testCopyClipRemoveAndPaste(self):
        clip1 = GES.TestClip.new()
        clip1.props.duration = 10

        self.layer.add_clip(clip1)

        self.assertEqual(len(clip1.get_children(False)), 2)

        copy = clip1.copy(True)
        self.assertEqual(len(self.layer.get_clips()), 1)

        self.layer.remove_clip(clip1)

        copy.paste(10)
        self.assertEqual(len(self.layer.get_clips()), 1)

    def testCopyPasteTitleClip(self):
        clip1 = GES.TitleClip.new()
        clip1.props.duration = 10

        self.layer.add_clip(clip1)
        self.assertEqual(len(clip1.get_children(False)), 1)

        copy = clip1.copy(True)
        self.assertEqual(len(self.layer.get_clips()), 1)

        copy.paste(10)
        self.assertEqual(len(self.layer.get_clips()), 2)


class TestTitleClip(unittest.TestCase):
    def testGetPropertyNotInTrack(self):
        title_clip = GES.TitleClip.new()
        self.assertEqual(title_clip.props.text, "")
        self.assertEqual(title_clip.props.font_desc, "Serif 36")

class TestTrackElements(unittest.TestCase):
    def test_add_to_layer_with_effect_remove_add(self):
        timeline = GES.Timeline.new_audio_video()
        self.assertEqual(len(timeline.get_tracks()), 2)
        layer = timeline.append_layer()

        test_clip = GES.TestClip()
        self.assertEqual(test_clip.get_children(True), [])
        self.assertTrue(layer.add_clip(test_clip))
        audio_source = test_clip.find_track_element(None, GES.AudioSource)
        self.assertFalse(audio_source is None)

        self.assertTrue(test_clip.set_child_property("volume", 0.0))
        self.assertEqual(audio_source.get_child_property("volume")[1], 0.0)

        effect = GES.Effect.new("agingtv")
        test_clip.add(effect)

        children = test_clip.get_children(True)
        layer.remove_clip(test_clip)
        self.assertEqual(test_clip.get_children(True), children)

        self.assertTrue(layer.add_clip(test_clip))
        self.assertEqual(test_clip.get_children(True), children)

        audio_source = test_clip.find_track_element(None, GES.AudioSource)
        self.assertFalse(audio_source is None)
        self.assertEqual(audio_source.get_child_property("volume")[1], 0.0)
