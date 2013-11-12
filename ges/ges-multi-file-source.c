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
 * SECTION:ges-multi-file-source
 * @short_description: outputs the video stream from a sequence of images.
 * 
 * Outputs the video stream from a given image sequence. The start frame
 * chosen will be determined by the in-point property on the track element.
 */
#include "ges-internal.h"
#include "ges-track-element.h"
#include "ges-multi-file-source.h"
#include "ges-extractable.h"
#include "ges-uri-asset.h"

/* Extractable interface implementation */

static gchar *
ges_extractable_check_id (GType type, const gchar * id, GError ** error)
{
  return g_strdup (id);
}

static void
ges_extractable_interface_init (GESExtractableInterface * iface)
{
  iface->check_id = ges_extractable_check_id;
}

G_DEFINE_TYPE_WITH_CODE (GESMultiFileSource, ges_multi_file_source,
    GES_TYPE_VIDEO_SOURCE,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE,
        ges_extractable_interface_init));

struct _GESMultiFileSourcePrivate
{
  /*  Dummy variable */
  void *nothing;
};

enum
{
  PROP_0,
  PROP_LOCATION
};

static void
ges_multi_file_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESMultiFileSource *uriclip = GES_MULTI_FILE_SOURCE (object);

  switch (property_id) {
    case PROP_LOCATION:
      g_value_set_string (value, uriclip->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_multi_file_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESMultiFileSource *uriclip = GES_MULTI_FILE_SOURCE (object);

  switch (property_id) {
    case PROP_LOCATION:
      uriclip->location = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_multi_file_source_dispose (GObject * object)
{
  GESMultiFileSource *uriclip = GES_MULTI_FILE_SOURCE (object);

  if (uriclip->location)
    g_free (uriclip->location);

  G_OBJECT_CLASS (ges_multi_file_source_parent_class)->dispose (object);
}

static void
pad_added_cb (GstElement * decodebin, GstPad * pad, GstElement * bin)
{
  GstPad *srcpad;

  srcpad = gst_ghost_pad_new ("src", pad);

  gst_pad_set_active (srcpad, TRUE);
  gst_element_add_pad (bin, srcpad);
}

static GstElement *
ges_multi_file_source_create_source (GESTrackElement * track_element)
{
  GESMultiFileSource *self;
  GstElement *bin, *src, *decodebin;
  GstCaps *disc_caps;
  GstDiscovererStreamInfo *stream_info;
  GValue fps = G_VALUE_INIT;
  GstCaps *caps;
  GESUriSourceAsset *asset;

  self = (GESMultiFileSource *) track_element;

  asset =
      GES_URI_SOURCE_ASSET (ges_extractable_get_asset (GES_EXTRACTABLE (self)));

  if (asset != NULL) {
    stream_info = ges_uri_source_asset_get_stream_info (asset);
    disc_caps = gst_discoverer_stream_info_get_caps (stream_info);
    caps = gst_caps_copy (disc_caps);
    g_assert (stream_info);
    GST_DEBUG ("Got some nice caps %s", gst_caps_to_string (disc_caps));
  } else {
    caps = gst_caps_new_empty ();
    GST_WARNING ("Could not extract asset.");
  }

  g_value_init (&fps, GST_TYPE_FRACTION);
  gst_value_set_fraction (&fps, 25, 1);
  gst_caps_set_value (caps, "framerate", &fps);

  bin = GST_ELEMENT (gst_bin_new ("multi-image-bin"));

  src = gst_element_factory_make ("multifilesrc", NULL);
  decodebin = gst_element_factory_make ("decodebin", NULL);

  g_object_set (src, "caps", caps, "location", self->location, NULL);

  gst_bin_add_many (GST_BIN (bin), src, decodebin, NULL);
  gst_element_link_pads_full (src, "src", decodebin, "sink",
      GST_PAD_LINK_CHECK_NOTHING);

  g_signal_connect (G_OBJECT (decodebin), "pad-added",
      G_CALLBACK (pad_added_cb), bin);

  return bin;
}

static void
ges_multi_file_source_class_init (GESMultiFileSourceClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESVideoSourceClass *source_class = GES_VIDEO_SOURCE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESMultiFileSourcePrivate));

  object_class->get_property = ges_multi_file_source_get_property;
  object_class->set_property = ges_multi_file_source_set_property;
  object_class->dispose = ges_multi_file_source_dispose;

  /**
   * GESMultiFileSource:location:
   *
   * The location of the file/resource to use.
   */
  g_object_class_install_property (object_class, PROP_LOCATION,
      g_param_spec_string ("location", "LOCATION", "location of the resource",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  source_class->create_source = ges_multi_file_source_create_source;
}

static void
ges_multi_file_source_init (GESMultiFileSource * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_MULTI_FILE_SOURCE, GESMultiFileSourcePrivate);
}

/**
 * ges_multi_file_source_new:
 * @location: the URI the source should control
 *
 * Creates a new #GESMultiFileSource for the provided @location.
 *
 * Returns: A new #GESMultiFileSource.
 */
GESMultiFileSource *
ges_multi_file_source_new (gchar * location)
{
  return g_object_new (GES_TYPE_MULTI_FILE_SOURCE, "location", location,
      "track-type", GES_TRACK_TYPE_VIDEO, NULL);
}
