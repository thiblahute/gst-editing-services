/* GStreamer
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstframepositionner.h"

static void gst_frame_positionner_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_frame_positionner_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_frame_positionner_transform_ip (GstBaseTransform *
    trans, GstBuffer * buf);

static gboolean
gst_frame_positionner_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data);

enum
{
  PROP_0,
  PROP_ALPHA,
  PROP_POSX,
  PROP_POSY,
  PROP_ZORDER,
  PROP_WIDTH,
  PROP_HEIGHT
};

static GstStaticPadTemplate gst_frame_positionner_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw")
    );

static GstStaticPadTemplate gst_frame_positionner_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw")
    );

G_DEFINE_TYPE (GstFramePositionner, gst_frame_positionner,
    GST_TYPE_BASE_TRANSFORM);

void
ges_frame_positionner_set_capsfilter (GstFramePositionner * pos,
    GstElement * capsfilter)
{
  pos->capsfilter = capsfilter;
}

static void
gst_frame_positionner_update_size (GstFramePositionner * pos)
{
  GstCaps *size_caps;

  if (pos->capsfilter == NULL)
    return;

  if (pos->width == 0 && pos->height == 0)
    size_caps = gst_caps_new_empty_simple ("video/x-raw");
  else if (pos->width == 0)
    size_caps =
        gst_caps_new_simple ("video/x-raw", "height", G_TYPE_INT, pos->height,
        NULL);
  else if (pos->height == 0)
    size_caps =
        gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, pos->width,
        NULL);
  else
    size_caps =
        gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT,
        pos->width, "height", G_TYPE_INT, pos->height, NULL);

  g_object_set (pos->capsfilter, "caps", size_caps, NULL);
}

static void
gst_frame_positionner_class_init (GstFramePositionnerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_frame_positionner_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_frame_positionner_sink_template));

  gobject_class->set_property = gst_frame_positionner_set_property;
  gobject_class->get_property = gst_frame_positionner_get_property;
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_frame_positionner_transform_ip);

  /**
   * gstframepositionner:alpha:
   *
   * The desired alpha for the stream.
   */
  g_object_class_install_property (gobject_class, PROP_ALPHA,
      g_param_spec_double ("alpha", "alpha", "alpha of the stream",
          0.0, 1.0, 1.0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  /**
   * gstframepositionner:posx:
   *
   * The desired x position for the stream.
   */
  g_object_class_install_property (gobject_class, PROP_POSX,
      g_param_spec_int ("posx", "posx", "x position of the stream",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  /**
   * gstframepositionner:posy:
   *
   * The desired y position for the stream.
   */
  g_object_class_install_property (gobject_class, PROP_POSY,
      g_param_spec_int ("posy", "posy", "y position of the stream",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  /**
   * gstframepositionner:zorder:
   *
   * The desired z order for the stream.
   */
  g_object_class_install_property (gobject_class, PROP_ZORDER,
      g_param_spec_uint ("zorder", "zorder", "z order of the stream",
          0, 10000, 0, G_PARAM_READWRITE));

  /**
   * gesframepositionner:width:
   *
   * The desired width for that source.
   * Set to 0 if size is not mandatory, will be set to width of the current track.
   */
  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_int ("width", "width", "width of the source",
          0, G_MAXSHORT, 0, G_PARAM_READWRITE));

  /**
   * gesframepositionner:height:
   *
   * The desired height for that source.
   * Set to 0 if size is not mandatory, will be set to height of the current track.
   */
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
      g_param_spec_int ("height", "height", "height of the source",
          0, G_MAXSHORT, 0, G_PARAM_READWRITE));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "frame positionner", "Metadata",
      "This element provides with tagging facilities",
      "mduponchelle1@gmail.com");
}

static void
gst_frame_positionner_init (GstFramePositionner * framepositionner)
{
  framepositionner->alpha = 1.0;
  framepositionner->posx = 0.0;
  framepositionner->posy = 0.0;
  framepositionner->zorder = 0;
  framepositionner->width = 0;
  framepositionner->height = 0;
  framepositionner->capsfilter = NULL;
}

void
gst_frame_positionner_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFramePositionner *framepositionner = GST_FRAME_POSITIONNER (object);


  GST_OBJECT_LOCK (framepositionner);
  switch (property_id) {
    case PROP_ALPHA:
      framepositionner->alpha = g_value_get_double (value);
      break;
    case PROP_POSX:
      framepositionner->posx = g_value_get_int (value);
      break;
    case PROP_POSY:
      framepositionner->posy = g_value_get_int (value);
      break;
    case PROP_ZORDER:
      framepositionner->zorder = g_value_get_uint (value);
      break;
    case PROP_WIDTH:
      framepositionner->width = g_value_get_int (value);
      gst_frame_positionner_update_size (framepositionner);
      break;
    case PROP_HEIGHT:
      framepositionner->height = g_value_get_int (value);
      gst_frame_positionner_update_size (framepositionner);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (framepositionner);
}

void
gst_frame_positionner_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstFramePositionner *framepositionner = GST_FRAME_POSITIONNER (object);

  GST_DEBUG_OBJECT (framepositionner, "get_property");

  switch (property_id) {
    case PROP_ALPHA:
      g_value_set_double (value, framepositionner->alpha);
      break;
    case PROP_POSX:
      g_value_set_int (value, framepositionner->posx);
      break;
    case PROP_POSY:
      g_value_set_int (value, framepositionner->posy);
      break;
    case PROP_ZORDER:
      g_value_set_uint (value, framepositionner->zorder);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, framepositionner->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, framepositionner->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

GType
gst_frame_positionner_meta_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "alpha", "posx", "posy", "zorder", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstFramePositionnerApi", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_frame_positionner_get_info (void)
{
  static const GstMetaInfo *meta_info = NULL;

  if (g_once_init_enter (&meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (gst_frame_positionner_meta_api_get_type (),
        "GstFramePositionnerMeta",
        sizeof (GstFramePositionnerMeta), (GstMetaInitFunction) NULL,
        (GstMetaFreeFunction) NULL,
        (GstMetaTransformFunction) gst_frame_positionner_meta_transform);
    g_once_init_leave (&meta_info, meta);
  }
  return meta_info;
}

static gboolean
gst_frame_positionner_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstFramePositionnerMeta *dmeta, *smeta;

  smeta = (GstFramePositionnerMeta *) meta;

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    /* only copy if the complete data is copied as well */
    dmeta =
        (GstFramePositionnerMeta *) gst_buffer_add_meta (dest,
        gst_frame_positionner_get_info (), NULL);
    dmeta->alpha = smeta->alpha;
    dmeta->posx = smeta->posx;
    dmeta->posy = smeta->posy;
    dmeta->zorder = smeta->zorder;
  }

  return TRUE;
}

static GstFlowReturn
gst_frame_positionner_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstFramePositionnerMeta *meta;
  GstFramePositionner *framepositionner = GST_FRAME_POSITIONNER (trans);
  GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buf);

  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    gst_object_sync_values (GST_OBJECT (trans), timestamp);
  }

  meta =
      (GstFramePositionnerMeta *) gst_buffer_add_meta (buf,
      gst_frame_positionner_get_info (), NULL);

  GST_OBJECT_LOCK (framepositionner);
  meta->alpha = framepositionner->alpha;
  meta->posx = framepositionner->posx;
  meta->posy = framepositionner->posy;
  meta->zorder = framepositionner->zorder;
  GST_OBJECT_UNLOCK (framepositionner);

  return GST_FLOW_OK;
}
