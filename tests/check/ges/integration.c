/* GStreamer Editing Services
 * Copyright (C) 2013 Mathieu Duponchelle <mduponchelle1@gmail.com>
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

#include <ges/ges.h>
//#include "test-utils.h"
/* 
 */
static GMainLoop *loop;
static GESPipeline *pipeline = NULL;
static gint64 seeked_position = GST_CLOCK_TIME_NONE;    /* last seeked position */
static gint64 seek_tol = 0.05 * GST_SECOND;     /* tolerance seek interval */
static GList *seeks;            /* list of seeks */
static gboolean got_async_done = FALSE;

/* This is used to specify a dot dumping after the target element started outputting buffers */
static const gchar *target_element = "smart-mixer-mixer";

typedef struct _ProfileDescription
{
  const gchar *container_caps;
  const gchar *audio_caps;
  const gchar *video_caps;
  const gchar *output_file;
  const gchar *name;

} ProfileDescription;

/* *INDENT-OFF* */
static const ProfileDescription profile_specs[] = {
  { "application/ogg", "audio/x-vorbis", "video/x-theora", "assets/vorbis_theora.rendered.ogv", "vorbis_theora_ogv" },
  { "video/webm", "audio/x-vorbis", "video/x-vp8", "assets/vorbis_vp8.rendered.webm", "vorbis_vp8_webm"},
  { "video/quicktime,variant=iso", "audio/mpeg,mpegversion=1,layer=3", "video/x-h264",  "assets/mp3_h264.rendered.mov", "mp3_h264_mp4"},
  { "video/x-matroska", "audio/x-vorbis", "video/x-h264", "assets/vorbis_h264.rendered.mkv", "vorbis_h264_mkv"},
};
/* *INDENT-ON* */

typedef enum
{
  PROFILE_VORBIS_THEORA_OGG,
  PROFILE_VORBIS_VP8_WEBM,
  PROFILE_AAC_H264_QUICKTIME,
  PROFILE_VORBIS_H264_MATROSKA,
  PROFILE_NONE,
  PROFILE_ANY,
} EncodingProfileName;

typedef struct _PresetInfos
{
  const gchar *muxer_preset_name;
  const gchar *audio_preset_name;
  const gchar *video_preset_name;
} PresetInfos;

typedef struct SeekInfo
{
  GstClockTime position;        /* position to seek to */
  GstClockTime seeking_position;        /* position to do seek from */
} SeekInfo;

typedef struct _TestingFunction
{
  GTestDataFunc func;
  GTestDataFunc func_subproc;
  const gchar *shortname;
} TestingFunction;

typedef struct _MediaFile
{
  const gchar *name;
  const gchar *file1;
  const gchar *file2;
  gboolean *is_audio_only;
  gboolean *is_image;

  const gchar *audioencoder;
  const gchar *videoencoder;
  const gchar *muxer;

} MediaFile;

typedef struct _Scenario
{
  EncodingProfileName profile;
  const gchar *path;

  /* This should be way more modular ... in the future! */
  gboolean seeking;
  gboolean seek_paused;
  gboolean seek_always_paused;

  gboolean audio_only;
  gboolean video_only;
} Scenario;

typedef struct _TestDefinition
{
  EncodingProfileName encoding_profile;
  const gchar *file1;
  const gchar *file2;
  gchar *path;

  Scenario scenario;
} TestDefinition;

#define DURATION_TOLERANCE 0.1 * GST_SECOND

#define get_asset(filename, asset)                                      \
{                                                                       \
  GError *error = NULL;                                                 \
  gchar *uri = ges_test_file_name (filename);                           \
  asset = ges_uri_clip_asset_request_sync (uri, &error);                \
  if (GES_IS_ASSET (asset) == FALSE) {                                  \
    g_test_message ("Testing file %s could not be used as an "          \
        "asset -- Reason: %s", uri, error ? error->message : "Uknown"); \
    g_assert_not_reached();                                             \
  }                                                                     \
  g_free (uri);                                                         \
}

static SeekInfo *
new_seek_info (GstClockTime seeking_position, GstClockTime position)
{
  SeekInfo *info = g_slice_new0 (SeekInfo);
  info->seeking_position = seeking_position;
  info->position = position;

  return info;
}

static GstEncodingProfile *
create_profile (const char *container, const char *container_preset,
    const char *audio, const char *audio_preset, const char *video,
    const char *video_preset)
{
  GstEncodingContainerProfile *cprof = NULL;
  GstEncodingProfile *prof = NULL;
  GstCaps *caps, *restriction_caps;

  /* If we have both audio and video, we must have container */
  if (audio && video && !container)
    return NULL;

  if (container) {
    caps = gst_caps_from_string (container);
    cprof = gst_encoding_container_profile_new ("User profile", "User profile",
        caps, NULL);
    gst_caps_unref (caps);
    if (!cprof)
      return NULL;
    if (container_preset)
      gst_encoding_profile_set_preset ((GstEncodingProfile *) cprof,
          container_preset);
  }

  if (audio) {
    caps = gst_caps_from_string (audio);
    prof = (GstEncodingProfile *) gst_encoding_audio_profile_new (caps, NULL,
        NULL, 0);
    if (!prof)
      goto beach;
    if (audio_preset)
      gst_encoding_profile_set_preset (prof, audio_preset);
    if (cprof)
      gst_encoding_container_profile_add_profile (cprof, prof);
    gst_caps_unref (caps);
  }
  if (video) {
    restriction_caps =
        gst_caps_new_simple ("video/x-raw", "framerate", GST_TYPE_FRACTION, 30,
        1, "format", G_TYPE_STRING, "I420", NULL);
    caps = gst_caps_from_string (video);
    prof = (GstEncodingProfile *) gst_encoding_video_profile_new (caps, NULL,
        restriction_caps, 0);
    if (!prof)
      goto beach;
    if (video_preset)
      gst_encoding_profile_set_preset (prof, video_preset);
    if (cprof)
      gst_encoding_container_profile_add_profile (cprof, prof);
    gst_caps_unref (caps);
  }

  return cprof ? (GstEncodingProfile *) cprof : (GstEncodingProfile *) prof;

beach:
  if (cprof)
    gst_encoding_profile_unref (cprof);
  else
    gst_encoding_profile_unref (prof);
  return NULL;
}

static GstEncodingProfile *
create_audio_video_profile (EncodingProfileName type)
{
  return create_profile (profile_specs[type].container_caps, NULL,
      profile_specs[type].audio_caps, NULL, profile_specs[type].video_caps,
      NULL);
}

static GESTimeline *
create_timeline (TestDefinition * test)
{
  GESTimeline *timeline = ges_timeline_new ();

  if (test->scenario.audio_only)
    ges_timeline_add_track (timeline, GES_TRACK (ges_audio_track_new ()));
  else if (test->scenario.video_only)
    ges_timeline_add_track (timeline, GES_TRACK (ges_video_track_new ()));
  else {
    ges_timeline_add_track (timeline, GES_TRACK (ges_video_track_new ()));
    ges_timeline_add_track (timeline, GES_TRACK (ges_audio_track_new ()));
  }

  return timeline;
}

static GstPadProbeReturn
dump_to_dot (GstPad * pad, GstPadProbeInfo * info)
{
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "ges-integration-smart-mixer-push-buffer");
  return (GST_PAD_PROBE_REMOVE);
}

static gboolean
_bus_callback (GstBus * bus, GstMessage * message, TestDefinition * test)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_STATE_CHANGED:{
      GstState old_state, new_state;

      gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
      /* HACK */
      if (new_state == GST_STATE_PLAYING
          && !g_strcmp0 (GST_MESSAGE_SRC_NAME (message), target_element))
        gst_pad_add_probe (gst_element_get_static_pad (GST_ELEMENT
                (GST_MESSAGE_SRC (message)), "src"), GST_PAD_PROBE_TYPE_BUFFER,
            (GstPadProbeCallback) dump_to_dot, NULL, NULL);
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
          GST_DEBUG_GRAPH_SHOW_ALL, "ges-integration-error");

      g_assert_no_error (err);
      g_error_free (err);
      g_free (debug);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:
      GST_INFO ("EOS\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ASYNC_DONE:
      got_async_done = TRUE;
      if (GST_CLOCK_TIME_IS_VALID (seeked_position))
        seeked_position = GST_CLOCK_TIME_NONE;

      if (seeks == NULL && test->scenario.seek_always_paused) {
        /* We are now done with seeking, let it play until the end */
        gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
        gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL, -1);
      }
      break;
    default:
      /* unhandled message */
      break;
  }
  return TRUE;
}

static gboolean
get_position (TestDefinition * test)
{
  GList *tmp;
  gint64 position;
  gst_element_query_position (GST_ELEMENT (pipeline), GST_FORMAT_TIME,
      &position);
  tmp = seeks;

  GST_LOG ("Current position: %" GST_TIME_FORMAT, GST_TIME_ARGS (position));
  while (tmp) {
    SeekInfo *seek = tmp->data;
    if ((position >= (seek->seeking_position - seek_tol))
        && (position <= (seek->seeking_position + seek_tol))) {

      if (!got_async_done)
        g_assert (GST_CLOCK_TIME_IS_VALID (seeked_position) == FALSE);
      got_async_done = FALSE;

      GST_INFO ("seeking to: %" GST_TIME_FORMAT,
          GST_TIME_ARGS (seek->position));

      seeked_position = seek->position;
      if (test->scenario.seek_paused) {
        gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);
        GST_LOG ("Set state playing");
        gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL, -1);
        GST_LOG ("Done wainting");
      }

      if (test->scenario.seek_always_paused) {
        gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);
        gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL, -1);
      }
      g_assert (gst_element_seek_simple (GST_ELEMENT (pipeline),
              GST_FORMAT_TIME,
              GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, seek->position));

      if (test->scenario.seek_paused) {
        gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
        gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL, -1);
      }
      seeks = g_list_remove_link (seeks, tmp);
      g_slice_free (SeekInfo, seek);
      g_list_free (tmp);
      break;
    }
    tmp = tmp->next;
  }
  /* if seeking paused without playing and we reached the last seek, just play
   * till the end */
  return TRUE;
}

#define assert (message) \
    g_assertion_message_expr (message == NULL); \
    g_free (message);


static void
check_rendered_file_properties (const gchar * render_file,
    GstClockTime duration)
{
  GESUriClipAsset *asset;
  GstDiscovererInfo *info;
  GstClockTime real_duration;

  /* TODO: extend these tests */

  get_asset (render_file, asset);
  info = ges_uri_clip_asset_get_info (GES_URI_CLIP_ASSET (asset));
  gst_object_unref (asset);

  if (!GST_IS_DISCOVERER_INFO (info)) {
    GST_ERROR_OBJECT (info, "WAT");
    g_assertion_message_expr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC,
        g_strdup_printf ("Could not discover file %s %p", render_file));
  }

  /* Let's not be too nazi */

  real_duration = gst_discoverer_info_get_duration (info);

  if ((real_duration < duration - DURATION_TOLERANCE)
      || (real_duration > duration + DURATION_TOLERANCE)) {
    g_assertion_message_expr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC,
        g_strdup_printf ("Duration %"
            GST_TIME_FORMAT " not in range [%" GST_TIME_FORMAT " -- %"
            GST_TIME_FORMAT "]", GST_TIME_ARGS (real_duration),
            GST_TIME_ARGS (duration - DURATION_TOLERANCE),
            GST_TIME_ARGS (duration + DURATION_TOLERANCE)));
  }


  gst_object_unref (info);
}

static void
check_timeline (TestDefinition * test, GESTimeline * timeline)
{
  GstBus *bus;
  GstEncodingProfile *profile;
  gchar *render_uri = NULL;

  ges_timeline_commit (timeline);
  pipeline = ges_pipeline_new ();
  if (test->encoding_profile != PROFILE_NONE) {
    render_uri =
        ges_test_file_name (profile_specs[test->encoding_profile].output_file);

    profile = create_audio_video_profile (test->encoding_profile);
    ges_pipeline_set_render_settings (pipeline, render_uri, profile);
    ges_pipeline_set_mode (pipeline, TIMELINE_MODE_RENDER);


    gst_object_unref (profile);
  } else if (g_getenv ("GES_MUTE_TESTS")) {
    GstElement *sink = gst_element_factory_make ("fakesink", NULL);

    g_object_set (sink, "sync", TRUE, NULL);
    ges_pipeline_preview_set_audio_sink (pipeline, sink);

    sink = gst_element_factory_make ("fakesink", NULL);
    g_object_set (sink, "sync", TRUE, NULL);
    ges_pipeline_preview_set_video_sink (pipeline, sink);
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, (GstBusFunc) _bus_callback, test);
  gst_object_unref (bus);

  ges_pipeline_add_timeline (pipeline, timeline);
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
  gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL, -1);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
      GST_DEBUG_GRAPH_SHOW_ALL, "ges-integration-playing");
  if (seeks != NULL)
    g_timeout_add (50, (GSourceFunc) get_position, test);

  g_main_loop_run (loop);

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
  gst_element_get_state (GST_ELEMENT (pipeline), NULL, NULL, -1);

  if (test->encoding_profile != PROFILE_NONE) {
    check_rendered_file_properties (profile_specs[test->encoding_profile].
        output_file, ges_timeline_get_duration (timeline));
    g_free (render_uri);
  }

  gst_object_unref (pipeline);
}

/* Test seeking in various situations */
static void
run_simple_seeks_test (TestDefinition * test, GESTimeline * timeline)
{
  GList *tmp;

  if (!test->scenario.seek_always_paused) {
    seeks =
        g_list_append (seeks, new_seek_info (0.2 * GST_SECOND,
            0.6 * GST_SECOND));
    seeks =
        g_list_append (seeks, new_seek_info (1.0 * GST_SECOND,
            1.2 * GST_SECOND));
    seeks =
        g_list_append (seeks, new_seek_info (1.5 * GST_SECOND,
            1.8 * GST_SECOND));
  } else {
    /* if pipeline is not playing, let's make point-to-point seeks */
    seeks =
        g_list_append (seeks, new_seek_info (0.2 * GST_SECOND,
            0.6 * GST_SECOND));
    seeks =
        g_list_append (seeks, new_seek_info (0.6 * GST_SECOND,
            1.2 * GST_SECOND));
    seeks =
        g_list_append (seeks, new_seek_info (1.2 * GST_SECOND,
            1.8 * GST_SECOND));
  }
  check_timeline (test, timeline);
  if (seeks != NULL) {
    /* free failed seeks */
    while (seeks) {
      SeekInfo *info = seeks->data;

      tmp = seeks;
      GST_ERROR ("Seeking at %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT
          " did not happen", GST_TIME_ARGS (info->seeking_position),
          GST_TIME_ARGS (info->position));
      seeks = g_list_remove_link (seeks, tmp);
      g_slice_free (SeekInfo, info);
      g_list_free (tmp);
    }
    g_assert_cmpstr ("Got EOS before being able to execute all seeks", ==, "");
  }
}

/* Test adding an effect [E] marks the effect */
static void
test_effect (TestDefinition * test)
{
  GESTimeline *timeline;
  GESLayer *layer;
  GError *error = NULL;
  GESUriClipAsset *asset1;
  GESEffect *effect;
  GESClip *clip;
  gchar *uri = ges_test_file_name (test->file1);

  asset1 = ges_uri_clip_asset_request_sync (uri, &error);
  g_free (uri);
  if (!GES_IS_ASSET (asset1)) {
    g_test_message ("Testing file %s could not be used as an "
        "asset -- Reason: %s", uri, error ? error->message : "Uknown");
    g_assert_not_reached ();
  }

  g_assert (asset1 != NULL);

  layer = ges_layer_new ();
  timeline = create_timeline (test);
  g_assert (ges_timeline_add_layer (timeline, layer));

  clip =
      ges_layer_add_asset (layer, GES_ASSET (asset1), 0 * GST_SECOND,
      0 * GST_SECOND, 2 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset1);

  effect = ges_effect_new ("agingtv");
  ges_container_add (GES_CONTAINER (clip), GES_TIMELINE_ELEMENT (effect));

    /**
   * Our timeline
   *          [   E    ]
   * inpoints 0--------0
   *          |  clip  |
   * time     0--------1
   */

  if (test->scenario.seeking)
    run_simple_seeks_test (test, timeline);
  else
    check_timeline (test, timeline);
}

static void
test_transition (TestDefinition * test)
{
  GESTimeline *timeline;
  GESLayer *layer;
  GESUriClipAsset *asset1, *asset2;
  GESClip *clip;

  timeline = create_timeline (test);
  layer = ges_layer_new ();
  g_assert (ges_timeline_add_layer (timeline, layer));

  g_object_set (layer, "auto-transition", TRUE, NULL);

  get_asset (test->file1, asset1);
  get_asset (test->file2, asset2);

  g_assert (asset1 != NULL && asset2 != NULL);

  clip =
      ges_layer_add_asset (layer, GES_ASSET (asset1), 0 * GST_SECOND,
      0 * GST_SECOND, 2 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset1);

  clip =
      ges_layer_add_asset (layer, GES_ASSET (asset2), 1 * GST_SECOND,
      0 * GST_SECOND, 2 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset2);

  ges_timeline_element_set_start (GES_TIMELINE_ELEMENT (clip), 1 * GST_SECOND);

    /**
   * Our timeline
   *                    [T]
   * inpoints 0--------0 0--------0
   *          |  clip  | |  clip2 |
   * time     0------- 2 1--------3
   */

  if (test->scenario.seeking)
    run_simple_seeks_test (test, timeline);
  else
    check_timeline (test, timeline);
}

static void
run_basic (TestDefinition * test, GESTimeline * timeline)
{
  GESLayer *layer;
  GESUriClipAsset *asset1;
  GESUriClipAsset *asset2;

  GST_ERROR ("SUCE");
  get_asset (test->file1, asset1);
  get_asset (test->file2, asset2);
  layer = ges_layer_new ();
  g_assert (ges_timeline_add_layer (timeline, layer));

  ges_layer_add_asset (layer, GES_ASSET (asset1), 0 * GST_SECOND,
      0 * GST_SECOND, 1 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset1);
  /* Test most simple case */

  ges_layer_add_asset (layer, GES_ASSET (asset2), 1 * GST_SECOND,
      0 * GST_SECOND, 1 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset2);

    /**
   * Our timeline
   *
   * inpoints 0--------0 0--------0
   *          |  clip  | |  clip2 |
   * time     0------- 1 1--------2
   */

  if (test->scenario.seeking)
    run_simple_seeks_test (test, timeline);
  else
    check_timeline (test, timeline);
}

static void
test_basic (TestDefinition * test)
{
  GST_ERROR ("HERE");
  run_basic (test, create_timeline (test));
}

#if 0
static void
test_image (TestDefinition * test)
{
  GESTimeline *timeline;
  GESLayer *layer;
  GESUriClipAsset *asset1, *asset2;

  get_asset (test_image_filename, asset1);
  get_asset (test->file1, asset2);

  layer = ges_layer_new ();
  timeline = ges_timeline_new_audio_video ();
  g_assert (ges_timeline_add_layer (timeline, layer));

  ges_layer_add_asset (layer, GES_ASSET (asset1), 0 * GST_SECOND,
      0 * GST_SECOND, 1 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset1);
  /* Test most simple case */

  layer = ges_layer_new ();
  g_assert (ges_timeline_add_layer (timeline, layer));

  ges_layer_add_asset (layer, GES_ASSET (asset2), 1 * GST_SECOND,
      0 * GST_SECOND, 1 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN);
  gst_object_unref (asset2);


    /**
   * Our timeline
   *
   * inpoints 0--------0
   *          |  clip  |
   * time     0--------1
   */

  if (test->seeking)
    run_simple_seeks_test (test, timeline);
  else
    check_timeline (test, timeline);
}
#endif

static gboolean
test_mix_layers (GESTimeline * timeline, GESUriClipAsset ** assets,
    guint32 num_assets, guint32 num_layers)
{
  GESLayer *layer;
  GESClip *clip;
  GList *tmp;
  GESTrackElement *track_element;
  GESTrackType track_type;
  GESUriClipAsset *asset;
  guint32 i, j;
  gfloat step = 4.0 / num_layers;

  for (i = 0; i < num_layers; i++) {
    layer = ges_timeline_append_layer (timeline);
    g_assert (layer != NULL);

    for (j = 0; j < num_assets; j++) {
      asset = assets[j];

      clip =
          ges_layer_add_asset (layer, GES_ASSET (asset),
          (i * step + j) * GST_SECOND, 0 * GST_SECOND, 1 * GST_SECOND,
          GES_TRACK_TYPE_UNKNOWN);
      g_assert (clip != NULL);

      for (tmp = GES_CONTAINER_CHILDREN (clip); tmp; tmp = tmp->next) {
        track_element = GES_TRACK_ELEMENT (tmp->data);
        track_type = ges_track_element_get_track_type (track_element);

        switch (track_type) {
          case GES_TRACK_TYPE_VIDEO:
            ges_track_element_set_child_properties (track_element, "alpha",
                (gdouble) (num_layers - 1 - i) * step, NULL);
            break;
          case GES_TRACK_TYPE_AUDIO:
            ges_track_element_set_child_properties (track_element, "volume",
                (gdouble) (num_layers - 1 - i) * step, NULL);
            break;
          default:
            break;
        }
      }
    }
  }
  return TRUE;
}


static void
test_mixing (TestDefinition * test)
{
  GESTimeline *timeline;
  GESUriClipAsset *asset[2];
  GError *error = NULL;

  gchar *uri1 = ges_test_file_name (test->file1);
  gchar *uri2 = ges_test_file_name (test->file1);

  timeline = create_timeline (test);
  ges_timeline_add_track (timeline, GES_TRACK (ges_audio_track_new ()));

  asset[0] = ges_uri_clip_asset_request_sync (uri1, &error);
  asset[1] = ges_uri_clip_asset_request_sync (uri2, &error);

  g_free (uri1);
  g_free (uri2);

  /* we are only using the first asset / clip for now */
  g_assert (test_mix_layers (timeline, asset, 1, 2));

    /**
   * Our timeline has 4 layers
   *
   * inpoints 0--------0
   *          |  clip  |
   * time     0--------1
   * inpoints    0--------0
   *             |  clip  |
   * time        0.25--1.25
   * inpoints       0--------0
   *                |  clip  |
   * time           0.5----1.5
   * inpoints          0--------0
   *                   |  clip  |
   * time              0.75--1.75
   */

  if (test->scenario.seeking)
    run_simple_seeks_test (test, timeline);
  else
    check_timeline (test, timeline);

}

static void
test_title (TestDefinition * test)
{
  GESTitleClip *title;
  GESAsset *asset[2];
  GESTimeline *timeline;
  GESLayer *layer, *layer1;

  GError *error = NULL;
  gchar *uri1 = ges_test_file_name (test->file1);

  timeline = ges_timeline_new_audio_video ();
  layer = ges_timeline_append_layer (timeline);
  layer1 = ges_timeline_append_layer (timeline);
  asset[0] = ges_asset_request (GES_TYPE_TITLE_CLIP, NULL, &error);
  asset[1] = GES_ASSET (ges_uri_clip_asset_request_sync (uri1, &error));
  g_free (uri1);

  /**
   * Our timeline
   *
   * inpoints 0--------0
   *          |  Title |
   * time     0------- 1
   * inpoints          0--------0
   *                   |  clip  |
   * time              0------- 1
   */
  title =
      GES_TITLE_CLIP (ges_layer_add_asset (layer, asset[0], 0, 0,
          1 * GST_SECOND, GES_TRACK_TYPE_UNKNOWN));
  ges_title_clip_set_text (title, "This is a title test");
  ges_layer_add_asset (layer1, asset[1], 1 * GST_SECOND, 0, 1 * GST_SECOND,
      GES_TRACK_TYPE_UNKNOWN);

  if (test->scenario.seeking)
    run_simple_seeks_test (test, timeline);
  else
    check_timeline (test, timeline);
}

static void
test_trick (TestDefinition * test)
{
  g_test_trap_subprocess (test->path, G_USEC_PER_SEC * 20,
      G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static TestingFunction testing_funcs[] = {
  {(GTestDataFunc) test_trick, (GTestDataFunc) test_basic, "basic"},
  {(GTestDataFunc) test_trick, (GTestDataFunc) test_title, "title"},
  {(GTestDataFunc) test_trick, (GTestDataFunc) test_mixing, "mixing"},
  {(GTestDataFunc) test_trick, (GTestDataFunc) test_effect, "effect"},
  {(GTestDataFunc) test_trick, (GTestDataFunc) test_transition, "transition"},
};

static MediaFile assets[] = {
  {"vorbis_vp8.webm", "assets/vorbis_vp8.0.webm", "assets/vorbis_vp8.1.webm",
      FALSE, FALSE, "vorbisenc", "vp8enc", "webmmux"},
  {"vorbis_theora.ogv", "assets/vorbis_theora.0.ogg",
        "assets/vorbis_theora.1.ogg", FALSE, FALSE, "vorbisenc", "theoraenc",
      "oggmux"},
  {"raw_h264.mp4", "assets/raw_h264.0.mov", "assets/raw_h264.1.mov", FALSE,
      FALSE, "x264enc", "qtmux"},
  {"mp3_h264.mkv", "assets/mp3_h264.0.mov", "assets/mp3_h264.1.mov", FALSE,
      FALSE, "lamemp3enc", "x264enc", "matroskamux"}
};

static Scenario scenarios[] = {
  {PROFILE_ANY, "", FALSE, FALSE, FALSE},
  {PROFILE_ANY, "/audio_only", FALSE, FALSE, FALSE, TRUE, FALSE},
  {PROFILE_ANY, "/video_only", FALSE, FALSE, FALSE, FALSE, TRUE},
  {PROFILE_NONE, "/seek", TRUE, FALSE, FALSE, FALSE, FALSE},
  {PROFILE_NONE, "/seek_paused", TRUE, TRUE, FALSE, FALSE, FALSE},
  {PROFILE_NONE, "/seek_always_paused", TRUE, FALSE, TRUE, FALSE, FALSE}
};

static gboolean
generate_all_files (void)
{
  gint assetscnt;

  for (assetscnt = 0; assetscnt < G_N_ELEMENTS (assets); assetscnt++) {
    MediaFile *m = &assets[assetscnt];

    if (m->is_image)
      continue;

    if (!ges_generate_test_file_audio_video (m->file1,
            m->audioencoder, m->videoencoder, m->muxer, "18", "11"))
      return FALSE;

    if (!ges_generate_test_file_audio_video (m->file2,
            m->audioencoder, m->videoencoder, m->muxer, "18", "11"))
      return FALSE;
  }

  return TRUE;
}


int
main (int argc, char **argv)
{
  gint funccnt, assetscnt, profilecnt, scenariocnt;

  g_test_init (&argc, &argv, NULL);

  gst_init (&argc, &argv);
  ges_init ();

  if (!generate_all_files ()) {
    GST_ERROR ("error generating necessary test files in rendering test\n");
    return 1;
  }

  for (funccnt = 0; funccnt < G_N_ELEMENTS (testing_funcs); funccnt++) {
    for (profilecnt = 0; profilecnt < PROFILE_ANY; profilecnt++) {
      for (assetscnt = 0; assetscnt < G_N_ELEMENTS (assets); assetscnt++) {
        for (scenariocnt = 0; scenariocnt < G_N_ELEMENTS (scenarios);
            scenariocnt++) {
          gchar *path, *path_subproc;
          TestDefinition *test;

          if (!(scenarios[scenariocnt].profile == PROFILE_ANY ||
                  scenarios[scenariocnt].profile == profilecnt))
            continue;

          test = g_slice_new0 (TestDefinition);
          test->encoding_profile = profilecnt;
          test->file1 = assets[assetscnt].file1;
          test->file2 = assets[assetscnt].file2;
          if (profilecnt == PROFILE_NONE) {
            path =
                g_strdup_printf ("/playback%s/%s/%s",
                scenarios[scenariocnt].path, testing_funcs[funccnt].shortname,
                assets[assetscnt].name);
            path_subproc =
                g_strdup_printf ("/playback%s/%s/%s/subprocess",
                scenarios[scenariocnt].path, testing_funcs[funccnt].shortname,
                assets[assetscnt].name);
          } else {
            path =
                g_strdup_printf ("/rendering/%s/%s/%s",
                testing_funcs[funccnt].shortname,
                profile_specs[profilecnt].name, assets[assetscnt].name);
            path_subproc =
                g_strdup_printf ("/rendering/%s/%s/subprocess",
                testing_funcs[funccnt].shortname, assets[assetscnt].name);
          }

          test->path = path_subproc;
          g_test_add_data_func (path, test, testing_funcs[funccnt].func);
          g_test_add_data_func (path_subproc, test,
              testing_funcs[funccnt].func_subproc);
        }
      }
    }
  }

  loop = g_main_loop_new (NULL, FALSE);

  return g_test_run ();
}
