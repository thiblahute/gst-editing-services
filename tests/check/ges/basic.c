/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <bilboed@bilboed.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <ges/ges.h>
#include <gst/check/gstcheck.h>

GST_START_TEST (test_ges_init)
{
  /* Yes, I know.. minimalistic... */
  ges_init ();
}

GST_END_TEST;

static gboolean
my_fill_track_func (GESTimelineObject * object,
    GESTrackObject * trobject, GnlObject * gnlobj, gpointer user_data)
{
  GstElement *src;

  GST_DEBUG ("timelineobj:%p, trackobject:%p, gnlobj:%p",
      object, trobject, gnlobj);

  /* Let's just put a fakesource in for the time being */
  src = gst_element_factory_make ("fakesrc", NULL);

  /* If this fails... that means that there already was something
   * in it */
  fail_unless (gst_bin_add (GST_BIN (gnlobj), src));

  return TRUE;
}

GST_START_TEST (test_ges_scenario)
{
  GESTimeline *timeline;
  GESTimelineLayer *layer, *tmp_layer;
  GESTrack *track;
  GESCustomTimelineSource *source;
  GESTrackObject *trackobject;
  GList *trackobjects, *tmp, *layers, *tracks;

  ges_init ();
  /* This is the simplest scenario ever */

  /* Timeline and 1 Layer */
  GST_DEBUG ("Create a timeline");
  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  GST_DEBUG ("Create a layer");
  layer = ges_timeline_layer_new ();
  fail_unless (layer != NULL);

  GST_DEBUG ("Add the layer to the timeline");
  fail_unless (ges_timeline_add_layer (timeline, layer));
  /* The timeline steals our reference to the layer */
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  fail_unless (layer->timeline == timeline);

  layers = ges_timeline_get_layers (timeline);
  fail_unless (g_list_find (layers, layer) != NULL);
  g_list_foreach (layers, (GFunc) g_object_unref, NULL);
  g_list_free (layers);

  /* Give the Timeline a Track */
  GST_DEBUG ("Create a Track");
  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, GST_CAPS_ANY);
  fail_unless (track != NULL);

  GST_DEBUG ("Add the track to the timeline");
  fail_unless (ges_timeline_add_track (timeline, track));

  /* The timeline steals the reference to the track */
  ASSERT_OBJECT_REFCOUNT (track, "track", 1);
  fail_unless (ges_track_get_timeline (track) == timeline);
  fail_unless ((gpointer) GST_ELEMENT_PARENT (track) == (gpointer) timeline);

  /* Create a source and add it to the Layer */
  GST_DEBUG ("Creating a source");
  source = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (source != NULL);

  /* The source will be floating before added to the layer... */
  fail_unless (g_object_is_floating (source));
  GST_DEBUG ("Adding the source to the timeline layer");
  fail_unless (ges_timeline_layer_add_object (layer,
          GES_TIMELINE_OBJECT (source)));
  fail_if (g_object_is_floating (source));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (source));
  fail_unless (tmp_layer == layer);
  /* The timeline stole our reference */
  ASSERT_OBJECT_REFCOUNT (source, "source", 1);
  g_object_unref (tmp_layer);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);

  /* Make sure the associated TrackObject is in the Track */
  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (source));
  fail_unless (trackobjects != NULL);
  trackobject = GES_TRACK_OBJECT (trackobjects->data);
  /* There are 4 references:
   * 1 by the timelineobject
   * 1 by the track
   * 1 by the timeline
   * 1 added by the call to _get_track_objects() above */
  ASSERT_OBJECT_REFCOUNT (trackobject, "trackobject", 4);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);
  /* There are 3 references:
   * 1 by the timelineobject
   * 1 by the timeline
   * 1 by the track */
  ASSERT_OBJECT_REFCOUNT (trackobject, "trackobject", 3);

  GST_DEBUG ("Remove the TimelineObject from the layer");

  /* Now remove the timelineobject */
  g_object_ref (source);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  fail_unless (ges_timeline_layer_remove_object (layer,
          GES_TIMELINE_OBJECT (source)));
  ASSERT_OBJECT_REFCOUNT (source, "source", 1);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (source));
  fail_unless (tmp_layer == NULL);
  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (source));
  fail_unless (trackobjects == NULL);   /* No unreffing then */
  g_object_unref (source);

  GST_DEBUG ("Removing track from the timeline");
  /* Remove the track from the timeline */
  g_object_ref (track);
  fail_unless (ges_timeline_remove_track (timeline, track));
  fail_unless (ges_track_get_timeline (track) == NULL);

  tracks = ges_timeline_get_tracks (timeline);
  fail_unless (tracks == NULL);
  ASSERT_OBJECT_REFCOUNT (track, "track", 1);
  g_object_unref (track);

  GST_DEBUG ("Removing layer from the timeline");
  /* Remove the layer from the timeline */
  g_object_ref (layer);
  fail_unless (ges_timeline_remove_layer (timeline, layer));
  fail_unless (layer->timeline == NULL);

  layers = ges_timeline_get_layers (timeline);
  fail_unless (layers == NULL);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  g_object_unref (layer);

  /* Finally clean up our object */
  ASSERT_OBJECT_REFCOUNT (timeline, "timeline", 1);
  g_object_unref (timeline);
}

GST_END_TEST;

/* very similar to the above, except we add the timeline object to the layer
 * and then add it to the timeline.
 */

GST_START_TEST (test_ges_timeline_add_layer)
{
  GESTimeline *timeline;
  GESTimelineLayer *layer, *tmp_layer;
  GESTrack *track;
  GESCustomTimelineSource *s1, *s2, *s3;
  GList *trackobjects, *tmp, *layers;
  GESTrackObject *trackobject;

  ges_init ();

  /* Timeline and 1 Layer */
  GST_DEBUG ("Create a timeline");
  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  GST_DEBUG ("Create a layer");
  layer = ges_timeline_layer_new ();
  fail_unless (layer != NULL);
  /* Give the Timeline a Track */
  GST_DEBUG ("Create a Track");
  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, GST_CAPS_ANY);
  fail_unless (track != NULL);

  GST_DEBUG ("Add the track to the timeline");
  fail_unless (ges_timeline_add_track (timeline, track));
  /* The timeline steals the reference to the track */
  ASSERT_OBJECT_REFCOUNT (track, "track", 1);
  fail_unless (ges_track_get_timeline (track) == timeline);
  fail_unless ((gpointer) GST_ELEMENT_PARENT (track) == (gpointer) timeline);

  /* Create a source and add it to the Layer */
  GST_DEBUG ("Creating a source");
  s1 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s1 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s1)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s1));
  fail_unless (tmp_layer == layer);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 2);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Creating a source");
  s2 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s2 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s2)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s2));
  fail_unless (tmp_layer == layer);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 2);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Creating a source");
  s3 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s3 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s3)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s3));
  fail_unless (tmp_layer == layer);
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 2);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Add the layer to the timeline");
  fail_unless (ges_timeline_add_layer (timeline, layer));
  /* The timeline steals our reference to the layer */
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  fail_unless (layer->timeline == timeline);
  layers = ges_timeline_get_layers (timeline);
  fail_unless (g_list_find (layers, layer) != NULL);
  g_list_foreach (layers, (GFunc) g_object_unref, NULL);
  g_list_free (layers);

  /* Make sure the associated TrackObjects are in the Track */
  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s1));
  fail_unless (trackobjects != NULL);
  trackobject = GES_TRACK_OBJECT (trackobjects->data);
  /* There are 4 references:
   * 1 by the timelineobject
   * 1 by the trackobject
   * 1 by the timeline
   * 1 added by the call to _get_track_objects() above */
  ASSERT_OBJECT_REFCOUNT (trackobject, "trackobject", 4);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);
  /* There are 3 references:
   * 1 by the timelineobject
   * 1 by the timeline
   * 1 by the trackobject */
  ASSERT_OBJECT_REFCOUNT (trackobject, "trackobject", 3);

  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s2));
  trackobject = GES_TRACK_OBJECT (trackobjects->data);
  fail_unless (trackobjects != NULL);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);
  /* There are 3 references:
   * 1 by the timelineobject
   * 1 by the timeline
   * 1 by the trackobject */
  ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (trackobject), "trackobject", 3);

  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s3));
  trackobject = GES_TRACK_OBJECT (trackobjects->data);
  fail_unless (trackobjects != NULL);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);
  /* There are 3 references:
   * 1 by the timelineobject
   * 1 by the timeline
   * 1 by the trackobject */
  ASSERT_OBJECT_REFCOUNT (trackobject, "trackobject", 3);

  /* theoretically this is all we need to do to ensure cleanup */
  g_object_unref (timeline);
}

GST_END_TEST;

/* this time we add the layer before we add the track. */

GST_START_TEST (test_ges_timeline_add_layer_first)
{
  GESTimeline *timeline;
  GESTimelineLayer *layer, *tmp_layer;
  GESTrack *track;
  GESCustomTimelineSource *s1, *s2, *s3;
  GList *trackobjects, *tmp, *layers;

  ges_init ();

  /* Timeline and 1 Layer */
  GST_DEBUG ("Create a timeline");
  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  GST_DEBUG ("Create a layer");
  layer = ges_timeline_layer_new ();
  fail_unless (layer != NULL);
  /* Give the Timeline a Track */
  GST_DEBUG ("Create a Track");
  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, GST_CAPS_ANY);
  fail_unless (track != NULL);

  /* Create a source and add it to the Layer */
  GST_DEBUG ("Creating a source");
  s1 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s1 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s1)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s1));
  fail_unless (tmp_layer == layer);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Creating a source");
  s2 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s2 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s2)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s2));
  fail_unless (tmp_layer == layer);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Creating a source");
  s3 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s3 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s3)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s3));
  fail_unless (tmp_layer == layer);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Add the layer to the timeline");
  fail_unless (ges_timeline_add_layer (timeline, layer));
  /* The timeline steals our reference to the layer */
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  fail_unless (layer->timeline == timeline);
  layers = ges_timeline_get_layers (timeline);
  fail_unless (g_list_find (layers, layer) != NULL);
  g_list_foreach (layers, (GFunc) g_object_unref, NULL);
  g_list_free (layers);

  GST_DEBUG ("Add the track to the timeline");
  fail_unless (ges_timeline_add_track (timeline, track));
  ASSERT_OBJECT_REFCOUNT (track, "track", 1);
  fail_unless (ges_track_get_timeline (track) == timeline);
  fail_unless ((gpointer) GST_ELEMENT_PARENT (track) == (gpointer) timeline);

  /* Make sure the associated TrackObjects are in the Track */
  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s1));
  fail_unless (trackobjects != NULL);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    /* Each object has 4 references:
     * 1 by the timelineobject
     * 1 by the track
     * 1 by the timeline
     * 1 added by _get_track_object() above */
    ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (tmp->data), "trackobject", 4);
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);

  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s2));
  fail_unless (trackobjects != NULL);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    /* Each object has 4 references:
     * 1 by the timelineobject
     * 1 by the track
     * 1 by the timeline
     * 1 added by _get_track_object() above */
    ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (tmp->data), "trackobject", 4);
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);

  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s3));
  fail_unless (trackobjects != NULL);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    /* Each object has 4 references:
     * 1 by the timelineobject
     * 1 by the track
     * 1 by the timeline
     * 1 added by _get_track_object() above */
    ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (tmp->data), "trackobject", 4);
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_list_free (trackobjects);

  /* theoretically this is all we need to do to ensure cleanup */
  g_object_unref (timeline);
}

GST_END_TEST;

GST_START_TEST (test_ges_timeline_remove_track)
{
  GESTimeline *timeline;
  GESTimelineLayer *layer, *tmp_layer;
  GESTrack *track;
  GESCustomTimelineSource *s1, *s2, *s3;
  GESTrackObject *t1, *t2, *t3;
  GList *trackobjects, *tmp, *layers;

  ges_init ();

  /* Timeline and 1 Layer */
  GST_DEBUG ("Create a timeline");
  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  GST_DEBUG ("Create a layer");
  layer = ges_timeline_layer_new ();
  fail_unless (layer != NULL);
  /* Give the Timeline a Track */
  GST_DEBUG ("Create a Track");
  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, GST_CAPS_ANY);
  fail_unless (track != NULL);

  /* Create a source and add it to the Layer */
  GST_DEBUG ("Creating a source");
  s1 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s1 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s1)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s1));
  fail_unless (tmp_layer == layer);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Creating a source");
  s2 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s2 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s2)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s2));
  fail_unless (tmp_layer == layer);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Creating a source");
  s3 = ges_custom_timeline_source_new (my_fill_track_func, NULL);
  fail_unless (s3 != NULL);
  fail_unless (ges_timeline_layer_add_object (layer, GES_TIMELINE_OBJECT (s3)));
  tmp_layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (s3));
  fail_unless (tmp_layer == layer);
  g_object_unref (tmp_layer);

  GST_DEBUG ("Add the layer to the timeline");
  fail_unless (ges_timeline_add_layer (timeline, layer));
  /* The timeline steals our reference to the layer */
  ASSERT_OBJECT_REFCOUNT (layer, "layer", 1);
  fail_unless (layer->timeline == timeline);

  layers = ges_timeline_get_layers (timeline);
  fail_unless (g_list_find (layers, layer) != NULL);
  g_list_foreach (layers, (GFunc) g_object_unref, NULL);
  g_list_free (layers);

  GST_DEBUG ("Add the track to the timeline");
  fail_unless (ges_timeline_add_track (timeline, track));
  ASSERT_OBJECT_REFCOUNT (track, "track", 1);
  fail_unless (ges_track_get_timeline (track) == timeline);
  fail_unless ((gpointer) GST_ELEMENT_PARENT (track) == (gpointer) timeline);

  /* Make sure the associated TrackObjects are in the Track */
  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s1));
  fail_unless (trackobjects != NULL);
  t1 = GES_TRACK_OBJECT ((trackobjects)->data);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    /* There are 4 references held:
     * 1 by the timelineobject
     * 1 by the track
     * 1 by the timeline
     * 1 added by the call to _get_track_objects() above */
    ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (tmp->data), "trackobject", 4);
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_object_ref (t1);
  g_list_free (trackobjects);
  /* There are 4 references held:
   * 1 by the timelinobject
   * 1 by the track
   * 1 by the timeline
   * 1 added by ourselves above (g_object_ref (t1)) */
  ASSERT_OBJECT_REFCOUNT (t1, "trackobject", 4);

  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s2));
  fail_unless (trackobjects != NULL);
  t2 = GES_TRACK_OBJECT (trackobjects->data);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    /* There are 4 references held:
     * 1 by the timelineobject
     * 1 by the track
     * 1 by the timeline
     * 1 added by the call to _get_track_objects() above */
    ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (tmp->data), "trackobject", 4);
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_object_ref (t2);
  g_list_free (trackobjects);
  /* There are 4 references held:
   * 1 by the timelinobject
   * 1 by the track
   * 1 by the timeline
   * 1 added by ourselves above (g_object_ref (t1)) */
  ASSERT_OBJECT_REFCOUNT (t2, "t2", 4);

  trackobjects =
      ges_timeline_object_get_track_objects (GES_TIMELINE_OBJECT (s3));
  fail_unless (trackobjects != NULL);
  t3 = GES_TRACK_OBJECT (trackobjects->data);
  for (tmp = trackobjects; tmp; tmp = tmp->next) {
    /* There are 4 references held:
     * 1 by the timelineobject
     * 1 by the track
     * 1 by the timeline
     * 1 added by the call to _get_track_objects() above */
    ASSERT_OBJECT_REFCOUNT (GES_TRACK_OBJECT (tmp->data), "trackobject", 4);
    g_object_unref (GES_TRACK_OBJECT (tmp->data));
  }
  g_object_ref (t3);
  g_list_free (trackobjects);
  /* There are 4 references held:
   * 1 by the timelinobject
   * 1 by the track
   * 1 by the timeline
   * 1 added by ourselves above (g_object_ref (t1)) */
  ASSERT_OBJECT_REFCOUNT (t3, "t3", 4);

  /* remove the track and check that the track objects have been released */
  fail_unless (ges_timeline_remove_track (timeline, track));

  ASSERT_OBJECT_REFCOUNT (t1, "trackobject", 2);
  ASSERT_OBJECT_REFCOUNT (t2, "trackobject", 2);
  ASSERT_OBJECT_REFCOUNT (t3, "trackobject", 2);

  g_object_unref (t1);
  g_object_unref (t2);
  g_object_unref (t3);

  g_object_unref (timeline);
}

GST_END_TEST;

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges-basic");
  TCase *tc_chain = tcase_create ("basic");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_ges_init);
  tcase_add_test (tc_chain, test_ges_scenario);
  tcase_add_test (tc_chain, test_ges_timeline_add_layer);
  tcase_add_test (tc_chain, test_ges_timeline_add_layer_first);
  tcase_add_test (tc_chain, test_ges_timeline_remove_track);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = ges_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
