/* GStreamer GES plugin
 *
 * Copyright (C) 2015 Thibault Saunier <thibault.saunier@collabora.com>
 *
 * gstges.c
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
 *

 **
 * SECTION:gstgessource
 * @short_description: A GstBin subclasses use to use GESTimeline
 * as sources inside any GstPipeline.
 * @see_also: #GESTimeline
 *
 * The gstgessource is a bin that will simply expose the track source pads
 * and implements the GstUriHandler interface using a custom ges://0Xpointer
 * uri scheme.
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstgessource.h"

GST_DEBUG_CATEGORY_STATIC (gstgessource);
#define GST_CAT_DEFAULT gstgessource

enum
{
  PROP_0,
  PROP_TIMELINE,
  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static gboolean
gst_ges_source_set_timeline (GstGesSource * self, GESTimeline * timeline)
{
  GList *tmp;
  guint naudiopad = 0, nvideopad = 0;
  GstBin *sbin = GST_BIN (self);

  g_return_val_if_fail (GES_IS_TIMELINE (timeline), FALSE);

  if (self->timeline) {
    GST_FIXME_OBJECT (self, "Implement changing timeline support");

    return FALSE;
  }

  self->timeline = timeline;

  gst_bin_add (sbin, GST_ELEMENT (self->timeline));
  for (tmp = self->timeline->tracks; tmp; tmp = tmp->next) {
    GstPad *gpad;
    gchar *name = NULL;
    GstPad *pad = ges_timeline_get_pad_for_track (self->timeline, tmp->data);

    if (ges_track_element_get_track_type (tmp->data) == GES_TRACK_TYPE_AUDIO)
      name = g_strdup_printf ("audio_%u", naudiopad++);
    else if (ges_track_element_get_track_type (tmp->data) ==
        GES_TRACK_TYPE_VIDEO)
      name = g_strdup_printf ("video_%u", nvideopad++);

    gpad = gst_ghost_pad_new (name, pad);

    gst_pad_set_active (gpad, TRUE);
    gst_element_add_pad (GST_ELEMENT (self), gpad);
  }

  return TRUE;
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_ges_source_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_ges_source_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "ges", NULL };

  return protocols;
}

static gchar *
gst_ges_source_uri_get_uri (GstURIHandler * handler)
{
  GstGesSource *self = GST_GES_SOURCE (handler);

  return self->timeline ? g_strdup_printf ("ges://%p", self->timeline) : NULL;
}

static gboolean
gst_ges_source_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  return TRUE;
}

static void
gst_ges_source_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_ges_source_uri_get_type;
  iface->get_protocols = gst_ges_source_uri_get_protocols;
  iface->get_uri = gst_ges_source_uri_get_uri;
  iface->set_uri = gst_ges_source_uri_set_uri;
}

G_DEFINE_TYPE_WITH_CODE (GstGesSource, gst_ges_source, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_ges_source_uri_handler_init));

static void
gst_ges_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstGesSource *self = GST_GES_SOURCE (object);

  switch (property_id) {
    case PROP_TIMELINE:
      g_value_set_object (value, self->timeline);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
gst_ges_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGesSource *self = GST_GES_SOURCE (object);

  switch (property_id) {
    case PROP_TIMELINE:
      gst_ges_source_set_timeline (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
gst_ges_source_class_init (GstGesSourceClass * self_class)
{
  GObjectClass *gclass = G_OBJECT_CLASS (self_class);

  GST_DEBUG_CATEGORY_INIT (gstgessource, "gstgessource", 0,
      "ges source element");

  gclass->get_property = gst_ges_source_get_property;
  gclass->set_property = gst_ges_source_set_property;

  /**
   * GstGesSource:timeline:
   *
   * Timeline to use in this source.
   */
  properties[PROP_TIMELINE] = g_param_spec_object ("timeline", "Timeline",
      "Timeline to use in this source.",
      GES_TYPE_TIMELINE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gclass, PROP_LAST, properties);

}

static void
gst_ges_source_init (GstGesSource * self)
{
}
