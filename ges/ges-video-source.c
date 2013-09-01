/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
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

/**
 * SECTION:ges-video-source
 * @short_description: Base Class for video sources
 */

#include "ges-internal.h"
#include "ges/ges-meta-container.h"
#include "ges-track-element.h"
#include "ges-video-source.h"
#include "ges-layer.h"
#include "gstframepositionner.h"

G_DEFINE_TYPE (GESVideoSource, ges_video_source, GES_TYPE_SOURCE);

struct _GESVideoSourcePrivate
{
  GstFramePositionner *positionner;
  GstElement *capsfilter;
  gint width;
  gint height;
};

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_LAST
};

static void
update_z_order_cb (GESClip * clip, GParamSpec * arg G_GNUC_UNUSED,
    GESVideoSource * self)
{
  GESLayer *layer = ges_clip_get_layer (clip);

  if (layer == NULL)
    return;

  /* 10000 is the max value of zorder on videomixerpad, hardcoded */

  g_object_set (self->priv->positionner, "zorder",
      10000 - ges_layer_get_priority (layer), NULL);

  gst_object_unref (layer);
}

static GstElement *
ges_video_source_create_element (GESTrackElement * trksrc)
{
  GstElement *topbin;
  GstElement *sub_element;
  GESVideoSourceClass *source_class = GES_VIDEO_SOURCE_GET_CLASS (trksrc);
  GESVideoSource *self;
  GstElement *positionner, *videoscale, *capsfilter;
  const gchar *props[] = { "alpha", "posx", "posy", NULL };
  GESTimelineElement *parent;

  if (!source_class->create_source)
    return NULL;

  sub_element = source_class->create_source (trksrc);

  self = (GESVideoSource *) trksrc;

  /* That positionner will add metadata to buffers according to its
     properties, acting like a proxy for our smart-mixer dynamic pads. */
  positionner = gst_element_factory_make ("framepositionner", "frame_tagger");
  videoscale =
      gst_element_factory_make ("videoscale", "track-element-videoscale");
  capsfilter =
      gst_element_factory_make ("capsfilter", "track-element-capsfilter");

  ges_track_element_add_children_props (trksrc, positionner, NULL, NULL, props);
  topbin =
      create_bin ("videosrcbin", sub_element, positionner, videoscale,
      capsfilter, NULL);
  parent = ges_timeline_element_get_parent (GES_TIMELINE_ELEMENT (trksrc));
  if (parent) {
    self->priv->positionner = GST_FRAME_POSITIONNER (positionner);
    g_signal_connect (parent, "notify::layer",
        (GCallback) update_z_order_cb, trksrc);
    update_z_order_cb (GES_CLIP (parent), NULL, self);
    gst_object_unref (parent);
  } else {
    GST_ERROR ("No parent timeline element, SHOULD NOT HAPPEN");
  }

  self->priv->capsfilter = capsfilter;

  return topbin;
}

static void
ges_video_source_update_size (GESVideoSource * vsource)
{
  GstCaps *size_caps;

  size_caps =
      gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT,
      vsource->priv->width, "height", G_TYPE_INT, vsource->priv->height, NULL);
  g_object_set (vsource->priv->capsfilter, "caps", size_caps, NULL);
}

static void
ges_video_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESVideoSource *vsource = GES_VIDEO_SOURCE (object);

  switch (property_id) {
    case PROP_WIDTH:
      vsource->priv->width = g_value_get_int (value);
      ges_video_source_update_size (vsource);
      break;
    case PROP_HEIGHT:
      vsource->priv->height = g_value_get_int (value);
      ges_video_source_update_size (vsource);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ges_video_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESVideoSource *vsource = GES_VIDEO_SOURCE (object);

  switch (property_id) {
    case PROP_WIDTH:
      g_value_set_int (value, vsource->priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, vsource->priv->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ges_video_source_class_init (GESVideoSourceClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GESTrackElementClass *track_class = GES_TRACK_ELEMENT_CLASS (klass);
  GESVideoSourceClass *video_source_class = GES_VIDEO_SOURCE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESVideoSourcePrivate));

  gobject_class->set_property = ges_video_source_set_property;
  gobject_class->get_property = ges_video_source_get_property;

  /**
   * gesvideosource:width:
   *
   * The desired width for that source.
   * Set to 0 if size is not mandatory, will be set to width of the current track.
   */
  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_int ("width", "width", "width of the source",
          0, G_MAXSHORT, 0, G_PARAM_READWRITE));

  /**
   * gesvideosource:height:
   *
   * The desired height for that source.
   * Set to 0 if size is not mandatory, will be set to height of the current track.
   */
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
      g_param_spec_int ("height", "height", "height of the source",
          0, G_MAXSHORT, 0, G_PARAM_READWRITE));

  track_class->gnlobject_factorytype = "gnlsource";
  track_class->create_element = ges_video_source_create_element;
  video_source_class->create_source = NULL;
}

static void
ges_video_source_init (GESVideoSource * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_VIDEO_SOURCE, GESVideoSourcePrivate);
  self->priv->positionner = NULL;
  self->priv->width = 0;
  self->priv->height = 0;
  self->priv->capsfilter = NULL;
}
