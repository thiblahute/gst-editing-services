/* GStreamer Editing Services
 * Copyright (C) 2010 Edward Hervey <edward.hervey@collabora.co.uk>
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
 * SECTION:ges-utils
 * @short_description: Convenience methods
 *
 */

#include <string.h>

#include "ges-internal.h"
#include "ges-timeline.h"
#include "ges-track.h"
#include "ges-layer.h"
#include "ges.h"

/**
 * ges_timeline_new_audio_video:
 * 
 * Creates a new #GESTimeline containing a raw audio and a
 * raw video track.
 *
 * Returns: The newly created #GESTimeline.
 */

GESTimeline *
ges_timeline_new_audio_video (void)
{
  GESTrack *tracka, *trackv;
  GESTimeline *timeline;

  /* This is our main GESTimeline */
  timeline = ges_timeline_new ();

  tracka = GES_TRACK (ges_audio_track_new ());
  trackv = GES_TRACK (ges_video_track_new ());

  if (!ges_timeline_add_track (timeline, trackv) ||
      !ges_timeline_add_track (timeline, tracka)) {
    gst_object_unref (timeline);
    timeline = NULL;
  }

  return timeline;
}

/* Internal utilities */
gint
element_start_compare (GESTimelineElement * a, GESTimelineElement * b)
{
  if (a->start == b->start) {
    if (a->priority < b->priority)
      return -1;
    if (a->priority > b->priority)
      return 1;
    if (a->duration < b->duration)
      return -1;
    if (a->duration > b->duration)
      return 1;
    return 0;
  } else if (a->start < b->start)
    return -1;

  return 1;
}

gint
element_end_compare (GESTimelineElement * a, GESTimelineElement * b)
{
  if (GES_TIMELINE_ELEMENT_END (a) == GES_TIMELINE_ELEMENT_END (b)) {
    if (a->priority < b->priority)
      return -1;
    if (a->priority > b->priority)
      return 1;
    if (a->duration < b->duration)
      return -1;
    if (a->duration > b->duration)
      return 1;
    return 0;
  } else if (GES_TIMELINE_ELEMENT_END (a) < (GES_TIMELINE_ELEMENT_END (b)))
    return -1;

  return 1;
}

gboolean
pspec_equal (gconstpointer key_spec_1, gconstpointer key_spec_2)
{
  const GParamSpec *key1 = key_spec_1;
  const GParamSpec *key2 = key_spec_2;

  return (key1->owner_type == key2->owner_type &&
      strcmp (key1->name, key2->name) == 0);
}

guint
pspec_hash (gconstpointer key_spec)
{
  const GParamSpec *key = key_spec;
  const gchar *p;
  guint h = key->owner_type;

  for (p = key->name; *p; p++)
    h = (h << 5) - h + *p;

  return h;
}

static void
_pad_added_cb (GstElement * element, GstPad * srcpad, GstPad * sinkpad)
{
  gst_element_no_more_pads (element);
  gst_pad_link (srcpad, sinkpad);
}

static void
_ghost_pad_added_cb (GstElement * element, GstPad * srcpad, GstElement * bin)
{
  GstPad *ghost;

  ghost = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (ghost, TRUE);
  gst_element_add_pad (bin, ghost);
  gst_element_no_more_pads (element);
}

GstElement *
create_bin (const gchar * bin_name, GstElement * sub_element, ...)
{
  va_list argp;

  GstElement *element;
  GstElement *prev = NULL;
  GstElement *first = NULL;
  GstElement *bin;
  GstPad *sub_srcpad;

  va_start (argp, sub_element);
  bin = gst_bin_new (bin_name);
  gst_bin_add (GST_BIN (bin), sub_element);

  while ((element = va_arg (argp, GstElement *)) != NULL) {
    gst_bin_add (GST_BIN (bin), element);
    if (prev)
      gst_element_link (prev, element);
    prev = element;
    if (first == NULL)
      first = element;
  }

  va_end (argp);

  sub_srcpad = gst_element_get_static_pad (sub_element, "src");

  if (prev != NULL) {
    GstPad *srcpad, *sinkpad, *ghost;

    srcpad = gst_element_get_static_pad (prev, "src");
    ghost = gst_ghost_pad_new ("src", srcpad);
    gst_pad_set_active (ghost, TRUE);
    gst_element_add_pad (bin, ghost);

    sinkpad = gst_element_get_static_pad (first, "sink");
    if (sub_srcpad)
      gst_pad_link (sub_srcpad, sinkpad);
    else
      g_signal_connect (sub_element, "pad-added", G_CALLBACK (_pad_added_cb),
          sinkpad);

    gst_object_unref (srcpad);
    gst_object_unref (sinkpad);

  } else if (sub_srcpad) {
    GstPad *ghost;

    ghost = gst_ghost_pad_new ("src", sub_srcpad);
    gst_pad_set_active (ghost, TRUE);
    gst_element_add_pad (bin, ghost);
  } else {
    g_signal_connect (sub_element, "pad-added",
        G_CALLBACK (_ghost_pad_added_cb), bin);
  }

  return bin;
}
