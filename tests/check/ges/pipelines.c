/* GStreamer Editing Services
 * Copyright (C) 2013 Thibault Saunier <tsaunier@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "test-utils.h"
#include <ges/ges.h>
#include <gst/check/gstcheck.h>
#include <gst/check/gstconsistencychecker.h>

static void
message_received_cb (GstBus * bus, GstMessage * message, GMainLoop * mainloop)
{
  GST_INFO ("bus message from \"%" GST_PTR_FORMAT "\": %" GST_PTR_FORMAT,
      GST_MESSAGE_SRC (message), message);

  switch (message->type) {
    case GST_MESSAGE_EOS:
      g_main_loop_quit (mainloop);
      break;
    case GST_MESSAGE_WARNING:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_warning (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *gerror;
      gchar *debug;

      gst_message_parse_error (message, &gerror, &debug);
      gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);
      g_error_free (gerror);
      g_free (debug);
      g_main_loop_quit (mainloop);
      break;
    }
    default:
      break;
  }
}

GST_START_TEST (test_audio_video_change_in_audio)
{
  GstBus *bus;
  GstPad *tmppad;
  GMainLoop *mainloop;
  GESClip *clip1, *clip2, *clip3;
  GstElement *audio_sink, *video_sink;
  GstStreamConsistency *check_audio, *check_video;

  GESLayer *layer = ges_layer_new ();
  GESTimeline *timeline = ges_timeline_new_audio_video ();
  GESTimelinePipeline *pipeline = ges_timeline_pipeline_new ();
  GESAsset *testsrc_asset = ges_asset_request (GES_TYPE_TEST_CLIP, NULL, NULL);

  ges_timeline_add_layer (timeline, layer);
  ges_timeline_pipeline_add_timeline (pipeline, timeline);
  bus = gst_element_get_bus (GST_ELEMENT (pipeline));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_DEFAULT);

  /**
   * timeline:
   * ------------
   *
   * Track video:
   * ______________________
   * --------  ----------
   * | clip1 | | clip2   |
   * 0------- 1 ---------2
   * Track audio:
   * ______________________
   * ----------------------
   * |     clip3          |
   * 0--------------------2
   */
  clip1 = ges_layer_add_asset (layer, testsrc_asset, 0, 0, 1 * GST_SECOND,
      GES_TRACK_TYPE_VIDEO);
  CHECK_OBJECT_PROPS (clip1, 0, 0, 1 * GST_SECOND);
  fail_unless (g_list_length (GES_CONTAINER_CHILDREN (clip1)), 1);

  clip2 = ges_layer_add_asset (layer, testsrc_asset, 1 * GST_SECOND, 0,
      1 * GST_SECOND, GES_TRACK_TYPE_VIDEO);
  CHECK_OBJECT_PROPS (clip2, 1 * GST_SECOND, 0, 1 * GST_SECOND);
  fail_unless (g_list_length (GES_CONTAINER_CHILDREN (clip2)), 1);

  clip3 = ges_layer_add_asset (layer, testsrc_asset, 0, 0,
      2 * GST_SECOND, GES_TRACK_TYPE_AUDIO);
  CHECK_OBJECT_PROPS (clip3, 0 * GST_SECOND, 0, 2 * GST_SECOND);
  fail_unless (g_list_length (GES_CONTAINER_CHILDREN (clip3)), 1);

  mainloop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message", (GCallback) message_received_cb, mainloop);

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);
  assert_equals_int (gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL,
          GST_CLOCK_TIME_NONE), GST_STATE_CHANGE_SUCCESS);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "prerolled");

  audio_sink = ges_timeline_pipeline_preview_get_audio_sink (pipeline);
  tmppad = gst_element_get_static_pad (audio_sink, "sink");
  check_audio = gst_consistency_checker_new (tmppad);
  gst_object_unref (audio_sink);
  gst_object_unref (tmppad);

  video_sink = ges_timeline_pipeline_preview_get_video_sink (pipeline);
  tmppad = gst_element_get_static_pad (video_sink, "sink");
  check_video = gst_consistency_checker_new (tmppad);
  gst_object_unref (video_sink);
  gst_object_unref (tmppad);

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
  g_main_loop_run (mainloop);
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);

  gst_object_unref (bus);
  gst_object_unref (pipeline);
  g_main_loop_unref (mainloop);
  gst_consistency_checker_free (check_audio);
  gst_consistency_checker_free (check_video);
}

GST_END_TEST;


GST_START_TEST (test_audio_video_change_in_audio_fakesink)
{
  GstBus *bus;
  GstPad *tmppad;
  GMainLoop *mainloop;
  GESClip *clip1, *clip2, *clip3;
  GstStreamConsistency *check_audio, *check_video;

  GESLayer *layer = ges_layer_new ();
  GESTimeline *timeline = ges_timeline_new_audio_video ();
  GESTimelinePipeline *pipeline = ges_timeline_pipeline_new ();
  GstElement *audio_sink = gst_element_factory_make ("fakesink", NULL);
  GstElement *video_sink = gst_element_factory_make ("fakesink", NULL);
  GESAsset *testsrc_asset = ges_asset_request (GES_TYPE_TEST_CLIP, NULL, NULL);

  ges_timeline_add_layer (timeline, layer);
  ges_timeline_pipeline_add_timeline (pipeline, timeline);
  bus = gst_element_get_bus (GST_ELEMENT (pipeline));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_DEFAULT);

  /**
   * timeline:
   * ------------
   *
   * Track video:
   * ______________________
   * --------  ----------
   * | clip1 | | clip2   |
   * 0------- 1 ---------2
   * Track audio:
   * ______________________
   * ----------------------
   * |     clip3          |
   * 0--------------------2
   */
  clip1 = ges_layer_add_asset (layer, testsrc_asset, 0, 0, 1 * GST_SECOND,
      GES_TRACK_TYPE_VIDEO);
  CHECK_OBJECT_PROPS (clip1, 0, 0, 1 * GST_SECOND);
  fail_unless (g_list_length (GES_CONTAINER_CHILDREN (clip1)), 1);

  clip2 = ges_layer_add_asset (layer, testsrc_asset, 1 * GST_SECOND, 0,
      1 * GST_SECOND, GES_TRACK_TYPE_VIDEO);
  CHECK_OBJECT_PROPS (clip2, 1 * GST_SECOND, 0, 1 * GST_SECOND);
  fail_unless (g_list_length (GES_CONTAINER_CHILDREN (clip2)), 1);

  clip3 = ges_layer_add_asset (layer, testsrc_asset, 0, 0,
      2 * GST_SECOND, GES_TRACK_TYPE_AUDIO);
  CHECK_OBJECT_PROPS (clip3, 0 * GST_SECOND, 0, 2 * GST_SECOND);
  fail_unless (g_list_length (GES_CONTAINER_CHILDREN (clip3)), 1);

  mainloop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (bus, "message", (GCallback) message_received_cb, mainloop);

  ges_timeline_pipeline_preview_set_audio_sink (pipeline, audio_sink);
  ges_timeline_pipeline_preview_set_video_sink (pipeline, video_sink);
  tmppad = gst_element_get_static_pad (audio_sink, "sink");
  check_audio = gst_consistency_checker_new (tmppad);
  gst_object_unref (audio_sink);
  gst_object_unref (tmppad);

  tmppad = gst_element_get_static_pad (video_sink, "sink");
  check_video = gst_consistency_checker_new (tmppad);
  gst_object_unref (video_sink);
  gst_object_unref (tmppad);

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);
  assert_equals_int (gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL,
          GST_CLOCK_TIME_NONE), GST_STATE_CHANGE_SUCCESS);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "prerolled");
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
  g_main_loop_run (mainloop);
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);

  gst_object_unref (bus);
  gst_object_unref (pipeline);
  g_main_loop_unref (mainloop);
  gst_consistency_checker_free (check_audio);
  gst_consistency_checker_free (check_video);
}

GST_END_TEST;


static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("pipelines-test");
  TCase *simple_cases = tcase_create ("simples");

  suite_add_tcase (s, simple_cases);

  tcase_add_test (simple_cases, test_audio_video_change_in_audio);
  tcase_add_test (simple_cases, test_audio_video_change_in_audio_fakesink);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = ges_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);
  ges_init ();

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
