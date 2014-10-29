/* GStreamer Editing Services
 *
 * Copyright (C) <2013> Thibault Saunier <thibault.saunier@collabora.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ges-validate.h"
#include <ges/ges.h>

#ifdef HAVE_GST_VALIDATE
#include <gst/validate/gst-validate-scenario.h>
#include <gst/validate/validate.h>
#include <gst/validate/gst-validate-utils.h>

#define MONITOR_ON_PIPELINE "validate-monitor"
#define RUNNER_ON_PIPELINE "runner-monitor"

static void
_validate_report_added_cb (GstValidateRunner * runner,
    GstValidateReport * report, GstPipeline * pipeline)
{
  if (report->level == GST_VALIDATE_REPORT_LEVEL_CRITICAL) {
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
        GST_DEBUG_GRAPH_SHOW_ALL, "ges-launch--validate-error");
  }
}

static GESTimeline *
get_timeline (GstValidateScenario * scenario)
{
  GESTimeline *timeline;

  g_object_get (scenario->pipeline, "timeline", &timeline, NULL);

  return timeline;
}

static gboolean
_set_child_property (GstValidateScenario * scenario, GstValidateAction * action)
{
  const GValue *value;
  GESTimeline *timeline;
  GESTimelineElement *element;
  const gchar *property_name, *element_name;

  element_name = gst_structure_get_string (action->structure, "element-name");

  timeline = get_timeline (scenario);
  g_return_val_if_fail (timeline, FALSE);

  element = ges_timeline_get_element (timeline, element_name);
  g_return_val_if_fail (GES_IS_TRACK_ELEMENT (element), FALSE);

  property_name = gst_structure_get_string (action->structure, "property");
  value = gst_structure_get_value (action->structure, "value");

  GST_DEBUG ("%s Setting %s property to %p",
      element->name, property_name, value);
  ges_track_element_set_child_property (GES_TRACK_ELEMENT (element),
      property_name, (GValue *) value);

  g_object_unref (timeline);

  return TRUE;
}

static gboolean
_serialize_project (GstValidateScenario * scenario, GstValidateAction * action)
{
  const gchar *uri = gst_structure_get_string (action->structure, "uri");
  GESTimeline *timeline = get_timeline (scenario);
  gboolean res;

  gst_validate_printf (action, "Saving project to %s", uri);

  res = ges_timeline_save_to_uri (timeline, uri, NULL, TRUE, NULL);

  g_object_unref (timeline);
  return res;
}

static gboolean
_remove_asset (GstValidateScenario * scenario, GstValidateAction * action)
{
  const gchar *id = NULL;
  const gchar *type_string = NULL;
  GType type;
  GESTimeline *timeline = get_timeline (scenario);
  GESProject *project = ges_timeline_get_project (timeline);
  GESAsset *asset;
  gboolean res = FALSE;

  id = gst_structure_get_string (action->structure, "id");
  type_string = gst_structure_get_string (action->structure, "type");

  if (!type_string || !id) {
    GST_ERROR ("Missing parameters, we got type %s and id %s", type_string, id);
    goto beach;
  }

  if (!(type = g_type_from_name (type_string))) {
    GST_ERROR ("This type doesn't exist : %s", type_string);
    goto beach;
  }

  asset = ges_project_get_asset (project, id, type);

  if (!asset) {
    GST_ERROR ("No asset with id %s and type %s", id, type_string);
    goto beach;
  }

  res = ges_project_remove_asset (project, asset);

beach:
  g_object_unref (timeline);
  return res;
}

static GESAsset *
_get_asset (GType type, const gchar * id)
{
  GESAsset *asset;
  GError *error = NULL;

  const gchar *new_id = id;

  if (type == GES_TYPE_URI_CLIP)
    asset = (GESAsset *) ges_uri_clip_asset_request_sync (id, &error);
  else
    asset = ges_asset_request (type, id, &error);

  while (error &&
      error->domain == GST_RESOURCE_ERROR &&
      error->code == GST_RESOURCE_ERROR_NOT_FOUND &&
      type == GES_TYPE_URI_CLIP) {

    if (new_id == NULL)
      break;

    g_clear_error (&error);
    new_id = ges_launch_get_new_uri_from_wrong_uri (new_id);

    if (new_id)
      asset = (GESAsset *) ges_uri_clip_asset_request_sync (new_id, &error);
    else
      GST_ERROR ("Cant find anything for %s", new_id);

    if (asset && !error)
      ges_launch_validate_uri (new_id);
  }

  if (!asset || error) {
    GST_ERROR
        ("There was an error requesting the asset with id %s and type %s (%s)",
        id, g_type_name (type), error ? error->message : "unknown");

    return NULL;
  }

  return asset;
}



static gboolean
_add_asset (GstValidateScenario * scenario, GstValidateAction * action)
{
  const gchar *id = NULL;
  const gchar *type_string = NULL;
  GType type;
  GESTimeline *timeline = get_timeline (scenario);
  GESProject *project = ges_timeline_get_project (timeline);
  GESAsset *asset;
  gboolean res = FALSE;

  id = gst_structure_get_string (action->structure, "id");
  type_string = gst_structure_get_string (action->structure, "type");

  gst_validate_printf (action, "Adding asset of type %s with ID %s\n",
      id, type_string);

  if (!type_string || !id) {
    GST_ERROR ("Missing parameters, we got type %s and id %s", type_string, id);
    goto beach;
  }

  if (!(type = g_type_from_name (type_string))) {
    GST_ERROR ("This type doesn't exist : %s", type_string);
    goto beach;
  }

  asset = _get_asset (type, id);

  if (!asset) {
    res = FALSE;

    goto beach;
  }

  res = ges_project_add_asset (project, asset);

beach:
  g_object_unref (timeline);
  return res;
}

/* Unref after usage */
static GESLayer *
_get_layer_by_priority (GESTimeline * timeline, gint priority)
{
  GList *layers, *tmp;
  GESLayer *layer = NULL;

  layers = ges_timeline_get_layers (timeline);
  for (tmp = layers; tmp; tmp = tmp->next) {
    GESLayer *tmp_layer = GES_LAYER (tmp->data);
    guint tmp_priority;
    g_object_get (tmp_layer, "priority", &tmp_priority, NULL);
    if ((gint) tmp_priority == priority) {
      layer = gst_object_ref (tmp_layer);
      break;
    }
  }

  g_list_free_full (layers, gst_object_unref);
  return layer;
}

static gboolean
_add_layer (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTimeline *timeline = get_timeline (scenario);
  GESLayer *layer;
  gint priority;
  gboolean res = FALSE, auto_transition = FALSE;

  if (!gst_structure_get_int (action->structure, "priority", &priority)) {
    GST_ERROR ("priority is needed when adding a layer");
    goto beach;
  }

  layer = _get_layer_by_priority (timeline, priority);

  if (layer != NULL) {
    GST_ERROR
        ("A layer with priority %d already exists, not creating a new one",
        priority);
    gst_object_unref (layer);
    goto beach;
  }

  gst_structure_get_boolean (action->structure, "auto-transition",
      &auto_transition);

  layer = ges_layer_new ();
  g_object_set (layer, "priority", priority, "auto-transition", auto_transition,
      NULL);
  res = ges_timeline_add_layer (timeline, layer);

beach:
  g_object_unref (timeline);
  return res;
}

static gboolean
_remove_layer (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTimeline *timeline = get_timeline (scenario);
  GESLayer *layer;
  gint priority;
  gboolean res = FALSE;

  if (!gst_structure_get_int (action->structure, "priority", &priority)) {
    GST_ERROR ("priority is needed when removing a layer");
    goto beach;
  }

  layer = _get_layer_by_priority (timeline, priority);

  if (layer) {
    res = ges_timeline_remove_layer (timeline, layer);
    gst_object_unref (layer);
  } else {
    GST_ERROR ("No layer with priority %d", priority);
  }

beach:
  g_object_unref (timeline);
  return res;
}

static gboolean
_remove_clip (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTimeline *timeline = get_timeline (scenario);
  GESTimelineElement *clip;
  GESLayer *layer;
  const gchar *name;
  gboolean res = FALSE;

  name = gst_structure_get_string (action->structure, "name");
  clip = ges_timeline_get_element (timeline, name);
  g_return_val_if_fail (GES_IS_CLIP (clip), FALSE);

  gst_validate_printf (action, "removing clip with ID %s\n", name);
  layer = ges_clip_get_layer (GES_CLIP (clip));

  if (layer) {
    res = ges_layer_remove_clip (layer, GES_CLIP (clip));
    gst_object_unref (layer);
  } else {
    GST_ERROR ("No layer for clip %s", ges_timeline_element_get_name (clip));
  }

  g_object_unref (timeline);
  return res;
}

static gboolean
_add_clip (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTimeline *timeline = get_timeline (scenario);
  GESAsset *asset;
  GESLayer *layer;
  GESClip *clip;
  gint layer_priority;
  const gchar *name;
  const gchar *asset_id;
  const gchar *type_string;
  GType type;
  gboolean res = FALSE;
  GstClockTime duration = 1 * GST_SECOND, inpoint = 0, start =
      GST_CLOCK_TIME_NONE;

  gst_structure_get_int (action->structure, "layer-priority", &layer_priority);
  name = gst_structure_get_string (action->structure, "name");
  asset_id = gst_structure_get_string (action->structure, "asset-id");
  type_string = gst_structure_get_string (action->structure, "type");

  gst_validate_action_get_clocktime (scenario, action, "start", &start);
  gst_validate_action_get_clocktime (scenario, action, "inpoint", &inpoint);
  gst_validate_action_get_clocktime (scenario, action, "duration", &duration);

  gst_validate_printf (action, "Adding clip from asset of type %s with ID %s"
      " wanted name: %s"
      " -- start: %" GST_TIME_FORMAT
      ", inpoint: %" GST_TIME_FORMAT
      ", duration: %" GST_TIME_FORMAT "\n",
      type_string, asset_id, name,
      GST_TIME_ARGS (start), GST_TIME_ARGS (inpoint), GST_TIME_ARGS (duration));

  if (!(type = g_type_from_name (type_string))) {
    GST_ERROR ("This type doesn't exist : %s", type_string);
    goto beach;
  }

  asset = _get_asset (type, asset_id);
  if (!asset) {
    res = FALSE;

    goto beach;
  }

  layer = _get_layer_by_priority (timeline, layer_priority);

  if (!layer) {
    GST_ERROR ("No layer with priority %d", layer_priority);
    goto beach;
  }

  if (type == GES_TYPE_URI_CLIP) {
    duration = GST_CLOCK_TIME_NONE;
  }

  clip = ges_layer_add_asset (layer, asset, start, inpoint, duration,
      GES_TRACK_TYPE_UNKNOWN);

  if (clip) {
    res = TRUE;
    if (!ges_timeline_element_set_name (GES_TIMELINE_ELEMENT (clip), name)) {
      res = FALSE;
      GST_ERROR ("couldn't set name %s on clip with id %s", name, asset_id);
    }
  } else {
    GST_ERROR ("Couldn't add clip with id %s to layer with priority %d",
        asset_id, layer_priority);
  }

  gst_object_unref (layer);

beach:
  g_object_unref (timeline);
  return res;
}

static gboolean
_edit_container (GstValidateScenario * scenario, GstValidateAction * action)
{
  GList *layers = NULL;
  GESTimeline *timeline;
  GESTimelineElement *container;
  GstClockTime position;
  gboolean res = FALSE;

  gint new_layer_priority = -1;
  GESEditMode edge = GES_EDGE_NONE;
  GESEditMode mode = GES_EDIT_MODE_NORMAL;

  const gchar *edit_mode_str = NULL, *edge_str = NULL;
  const gchar *clip_name;

  clip_name = gst_structure_get_string (action->structure, "container-name");

  timeline = get_timeline (scenario);
  g_return_val_if_fail (timeline, FALSE);

  container = ges_timeline_get_element (timeline, clip_name);
  g_return_val_if_fail (GES_IS_CONTAINER (container), FALSE);

  if (!gst_validate_action_get_clocktime (scenario, action,
          "position", &position)) {
    GST_WARNING ("Could not get position");
    goto beach;
  }

  if ((edit_mode_str =
          gst_structure_get_string (action->structure, "edit-mode")))
    g_return_val_if_fail (gst_validate_utils_enum_from_str (GES_TYPE_EDIT_MODE,
            edit_mode_str, &mode), FALSE);

  if ((edge_str = gst_structure_get_string (action->structure, "edge")))
    g_return_val_if_fail (gst_validate_utils_enum_from_str (GES_TYPE_EDGE,
            edge_str, &edge), FALSE);

  gst_structure_get_int (action->structure, "new-layer-priority",
      &new_layer_priority);

  gst_validate_printf (action, "Editing %s to %" GST_TIME_FORMAT
      " in %s mode, edge: %s "
      "with new layer prio: %d \n\n",
      clip_name, GST_TIME_ARGS (position),
      edit_mode_str ? edit_mode_str : "normal",
      edge_str ? edge_str : "None", new_layer_priority);

  if (!(res = ges_container_edit (GES_CONTAINER (container), layers,
              new_layer_priority, mode, edge, position))) {
    gst_object_unref (container);
    GST_ERROR ("HERE");
    goto beach;
  }
  gst_object_unref (container);

beach:
  g_object_unref (timeline);
  return res;
}

static gboolean
_commit (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTimeline *timeline = get_timeline (scenario);

  gst_validate_printf (action, "Commiting timeline %s\n",
      GST_OBJECT_NAME (timeline));

  ges_timeline_commit (timeline);

  gst_object_unref (timeline);

  return TRUE;
}

static gboolean
_split_clip (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTimeline *timeline;
  const gchar *clip_name;
  GESTimelineElement *element;
  GstClockTime position;

  clip_name = gst_structure_get_string (action->structure, "clip-name");

  timeline = get_timeline (scenario);
  g_return_val_if_fail (timeline, FALSE);

  element = ges_timeline_get_element (timeline, clip_name);
  g_return_val_if_fail (GES_IS_CLIP (element), FALSE);
  g_object_unref (timeline);

  g_return_val_if_fail (gst_validate_action_get_clocktime (scenario, action,
          "position", &position), FALSE);

  return (ges_clip_split (GES_CLIP (element), position) != NULL);
}

static gboolean
_set_track_restriction_caps (GstValidateScenario * scenario,
    GstValidateAction * action)
{
  GList *tmp;
  GstCaps *caps;
  gboolean res = FALSE;
  GESTrackType track_types;

  GESTimeline *timeline = get_timeline (scenario);
  const gchar *track_type_str =
      gst_structure_get_string (action->structure, "track-type");
  const gchar *caps_str = gst_structure_get_string (action->structure, "caps");

  g_return_val_if_fail ((track_types =
          gst_validate_utils_flags_from_str (GES_TYPE_TRACK_TYPE,
              track_type_str)), FALSE);

  g_return_val_if_fail ((caps =
          gst_caps_from_string (caps_str)) != NULL, FALSE);

  timeline = get_timeline (scenario);

  for (tmp = timeline->tracks; tmp; tmp = tmp->next) {
    GESTrack *track = tmp->data;

    if (track->type & track_types) {
      ges_track_set_restriction_caps (track, caps);

      res = TRUE;
    }
  }

  gst_caps_unref (caps);
  gst_object_unref (timeline);

  return res;
}

static gboolean
_set_asset_on_element (GstValidateScenario * scenario,
    GstValidateAction * action)
{
  GESAsset *asset;
  GESTimelineElement *element;
  const gchar *element_name, *id;

  gboolean res = TRUE;
  GESTimeline *timeline = get_timeline (scenario);

  element_name = gst_structure_get_string (action->structure, "element-name");
  element = ges_timeline_get_element (timeline, element_name);
  g_return_val_if_fail (GES_IS_TIMELINE_ELEMENT (element), FALSE);

  id = gst_structure_get_string (action->structure, "asset-id");

  gst_validate_printf (action, "Setting asset %s on element %s\n",
      id, element_name);

  asset = _get_asset (G_OBJECT_TYPE (element), id);
  if (asset == NULL) {
    res = FALSE;
    GST_ERROR ("Could not find asset: %s", id);
    goto beach;
  }

  res = ges_extractable_set_asset (GES_EXTRACTABLE (element), asset);

beach:
  gst_object_unref (timeline);

  return res;
}

static gboolean
_container_add_child (GstValidateScenario * scenario,
    GstValidateAction * action)
{
  GESAsset *asset;
  GESContainer *container;
  GESTimelineElement *child = NULL;
  const gchar *container_name, *child_name, *child_type, *id;

  gboolean res = TRUE;
  GESTimeline *timeline = get_timeline (scenario);

  container_name =
      gst_structure_get_string (action->structure, "container-name");
  container =
      GES_CONTAINER (ges_timeline_get_element (timeline, container_name));
  g_return_val_if_fail (GES_IS_CONTAINER (container), FALSE);

  id = gst_structure_get_string (action->structure, "asset-id");
  child_type = gst_structure_get_string (action->structure, "child-type");

  if (id && child_type) {
    asset = _get_asset (g_type_from_name (child_type), id);

    if (asset == NULL) {
      res = FALSE;
      GST_ERROR ("Could not find asset: %s", id);
      goto beach;
    }

    child = GES_TIMELINE_ELEMENT (ges_asset_extract (asset, NULL));
    g_return_val_if_fail (GES_IS_TIMELINE_ELEMENT (child), FALSE);
  }

  child_name = gst_structure_get_string (action->structure, "child-name");
  if (!child && child_name) {
    child = ges_timeline_get_element (timeline, child_name);
    g_return_val_if_fail (GES_IS_TIMELINE_ELEMENT (child), FALSE);
  }

  if (!child) {
    GST_ERROR_OBJECT (scenario, "Wong parametters, could not get a child");

    return FALSE;
  }

  if (child_name)
    ges_timeline_element_set_name (child, child_name);
  else
    child_name = GES_TIMELINE_ELEMENT_NAME (child);

  gst_validate_printf (action, "Adding child %s to container %s\n",
      child_name, GES_TIMELINE_ELEMENT_NAME (container));

  res = ges_container_add (container, child);

beach:
  gst_object_unref (timeline);

  return res;
}

static gboolean
_container_remove_child (GstValidateScenario * scenario,
    GstValidateAction * action)
{
  GESContainer *container;
  GESTimelineElement *child;
  const gchar *container_name, *child_name;

  gboolean res = TRUE;
  GESTimeline *timeline = get_timeline (scenario);

  container_name =
      gst_structure_get_string (action->structure, "container-name");
  container =
      GES_CONTAINER (ges_timeline_get_element (timeline, container_name));
  g_return_val_if_fail (GES_IS_CONTAINER (container), FALSE);

  child_name = gst_structure_get_string (action->structure, "child-name");
  child = ges_timeline_get_element (timeline, child_name);
  g_return_val_if_fail (GES_IS_TIMELINE_ELEMENT (child), FALSE);

  gst_validate_printf (action, "Remove child %s from container %s\n",
      child_name, GES_TIMELINE_ELEMENT_NAME (container));

  res = ges_container_remove (container, child);

  gst_object_unref (timeline);

  return res;
}

static gboolean
_set_control_source (GstValidateScenario * scenario, GstValidateAction * action)
{
  GESTrackElement *element;

  gboolean ret = FALSE;
  GstControlSource *source = NULL;
  gchar *element_name, *property_name, *binding_type = NULL,
      *source_type = NULL, *interpolation_mode = NULL;

  GESTimeline *timeline = get_timeline (scenario);

  gst_structure_get (action->structure,
      "element-name", G_TYPE_STRING, &element_name,
      "property-name", G_TYPE_STRING, &property_name,
      "binding-type", G_TYPE_STRING, &binding_type,
      "source-type", G_TYPE_STRING, &source_type,
      "interpolation-mode", G_TYPE_STRING, &interpolation_mode, NULL);

  element =
      GES_TRACK_ELEMENT (ges_timeline_get_element (timeline, element_name));

  g_return_val_if_fail (GES_IS_TRACK_ELEMENT (element), FALSE);

  if (!binding_type)
    binding_type = g_strdup ("direct");

  if (source_type == NULL || !g_strcmp0 (source_type, "interpolation")) {
    GstInterpolationMode mode;

    source = gst_interpolation_control_source_new ();

    if (interpolation_mode)
      g_return_val_if_fail (gst_validate_utils_enum_from_str
          (GST_TYPE_INTERPOLATION_MODE, interpolation_mode, &mode), FALSE);
    else
      mode = GST_INTERPOLATION_MODE_LINEAR;

    g_object_set (source, "mode", mode, NULL);

  } else {
    GST_ERROR_OBJECT (scenario, "Interpolation type %s not supported",
        source_type);

    goto done;
  }

  ret = ges_track_element_set_control_source (element,
      source, property_name, binding_type);


done:
  gst_object_unref (timeline);
  g_free (element_name);
  g_free (binding_type);
  g_free (source_type);
  g_free (interpolation_mode);

  if (ret == FALSE) {
    GST_ERROR_OBJECT (scenario, "NOP, COULD NOT!");
  } else {
    GST_ERROR_OBJECT (scenario, "===> GOOD TO GO!!");
  }

  return ret;
}

static gboolean
_add_remove_keyframe (GstValidateScenario * scenario,
    GstValidateAction * action)
{
  GESTrackElement *element;

  gdouble value;
  GstClockTime timestamp;
  GstControlBinding *binding = NULL;
  GstTimedValueControlSource *source = NULL;
  gchar *element_name, *property_name;

  gboolean ret = FALSE;
  GESTimeline *timeline = get_timeline (scenario);

  gst_structure_get (action->structure,
      "element-name", G_TYPE_STRING, &element_name,
      "property-name", G_TYPE_STRING, &property_name,
      "value", G_TYPE_DOUBLE, &value, NULL);

  gst_validate_action_get_clocktime (scenario, action, "timestamp", &timestamp);

  element =
      GES_TRACK_ELEMENT (ges_timeline_get_element (timeline, element_name));
  g_return_val_if_fail (GES_IS_TRACK_ELEMENT (element), FALSE);

  binding = ges_track_element_get_control_binding (element, property_name);
  if (binding == NULL) {
    GST_ERROR_OBJECT (scenario, "No control binding found for %s:%s"
        " you should first set-control-binding on it",
        element_name, property_name);

    goto done;
  }

  g_object_get (binding, "control-source", &source, NULL);

  if (source == NULL) {
    GST_ERROR_OBJECT (scenario, "No control source found for %s:%s"
        " you should first set-control-binding on it",
        element_name, property_name);

    goto done;
  }

  if (!GST_IS_TIMED_VALUE_CONTROL_SOURCE (source)) {
    GST_ERROR_OBJECT (scenario, "You can use add-keyframe"
        " only on GstTimedValueControlSource not %s",
        G_OBJECT_TYPE_NAME (source));

    goto done;
  }

  if (!g_strcmp0 (action->type, "add-keyframe"))
    ret = gst_timed_value_control_source_set (source, timestamp, value);
  else
    ret = gst_timed_value_control_source_unset (source, timestamp);

done:
  gst_object_unref (timeline);
  if (source)
    gst_object_unref (source);
  g_free (element_name);
  g_free (property_name);

  return ret;
}

static void
ges_validate_register_action_types (void)
{
  gst_validate_init ();

  /*  *INDENT-OFF* */
  gst_validate_register_action_type ("edit-container", "ges", _edit_container,
      (GstValidateActionParameter [])  {
        {
         .name = "container-name",
         .description = "The name of the GESContainer to edit",
         .mandatory = TRUE,
         .types = "string",
        },
        {
          .name = "position",
          .description = "The new position of the GESContainer",
          .mandatory = TRUE,
          .types = "double or string",
          .possible_variables = "position: The current position in the stream\n"
            "duration: The duration of the stream",
           NULL
        },
        {
          .name = "edit-mode",
          .description = "The GESEditMode to use to edit @container-name",
          .mandatory = FALSE,
          .types = "string",
          .def = "normal",
        },
        {
          .name = "edge",
          .description = "The GESEdge to use to edit @container-name\n"
                         "should be in [ edge_start, edge_end, edge_none ] ",
          .mandatory = FALSE,
          .types = "string",
          .def = "edge_none",
        },
        {
          .name = "new-layer-priority",
          .description = "The priority of the layer @container should land in.\n"
                         "If the layer you're trying to move the container to doesn't exist, it will\n"
                         "be created automatically. -1 means no move.",
          .mandatory = FALSE,
          .types = "int",
          .def = "-1",
        },
        {NULL}
       },
       "Allows to edit a container (like a GESClip), for more details, have a look at:\n"
       "ges_container_edit documentation, Note that the timeline will\n"
       "be commited, and flushed so that the edition is taken into account",
       FALSE);

  gst_validate_register_action_type ("add-asset", "ges", _add_asset,
      (GstValidateActionParameter [])  {
        {
          .name = "id",
          .description = "Adds an asset to a project.",
          .mandatory = TRUE,
          NULL
        },
        {
          .name = "type",
          .description = "The type of asset to add",
          .mandatory = TRUE,
          NULL
        },
        {NULL}
      },
      "Allows to add an asset to the current project", FALSE);

  gst_validate_register_action_type ("remove-asset", "ges", _remove_asset,
      (GstValidateActionParameter [])  {
        {
          .name = "id",
          .description = "The ID of the clip to remove",
          .mandatory = TRUE,
          NULL
        },
        {
          .name = "type",
          .description = "The type of asset to remove",
          .mandatory = TRUE,
          NULL
        },
        { NULL }
      },
      "Allows to remove an asset from the current project", FALSE);

  gst_validate_register_action_type ("add-layer", "ges", _add_layer,
      (GstValidateActionParameter [])  {
        {
          .name = "priority",
          .description = "The priority of the new layer to add",
          .mandatory = TRUE,
          NULL
        },
        { NULL }
      },
      "Allows to add a layer to the current timeline", FALSE);

  gst_validate_register_action_type ("remove-layer", "ges", _remove_layer,
      (GstValidateActionParameter [])  {
        {
          .name = "priority",
          .description = "The priority of the layer to remove",
          .mandatory = TRUE,
          NULL
        },
        {
          .name = "auto-transition",
          .description = "Wheter auto-transition is activated on the new layer.",
          .mandatory = FALSE,
          .types="boolean",
          .def = "False"
        },
        { NULL }
      },
      "Allows to remove a layer from the current timeline", FALSE);

  gst_validate_register_action_type ("add-clip", "ges", _add_clip,
      (GstValidateActionParameter []) {
        {
          .name = "name",
          .description = "The name of the clip to add",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "layer-priority",
          .description = "The priority of the clip to add",
          .types = "int",
          .mandatory = TRUE,
        },
        {
          .name = "asset-id",
          .description = "The id of the asset from which to extract the clip",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "type",
          .description = "The type of the clip to create",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "start",
          .description = "The start value to set on the new GESClip.",
          .types = "double or string",
          .mandatory = FALSE,
        },
        {
          .name = "inpoint",
          .description = "The  inpoint value to set on the new GESClip",
          .types = "double or string",
          .mandatory = FALSE,
        },
        {
          .name = "duration",
          .description = "The  duration value to set on the new GESClip",
          .types = "double or string",
          .mandatory = FALSE,
        },
        {NULL}
      }, "Allows to add a clip to a given layer", FALSE);

  gst_validate_register_action_type ("remove-clip", "ges", _remove_clip,
      (GstValidateActionParameter []) {
        {
          .name = "name",
          .description = "The name of the clip to remove",
          .types = "string",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Allows to remove a clip from a given layer", FALSE);

  gst_validate_register_action_type ("serialize-project", "ges", _serialize_project,
      (GstValidateActionParameter []) {
        {
          .name = "uri",
          .description = "The uri where to store the serialized project",
          .types = "string",
          .mandatory = TRUE,
        },
        {NULL}
      }, "serializes a project", FALSE);

  gst_validate_register_action_type ("set-child-property", "ges", _set_child_property,
      (GstValidateActionParameter []) {
        {
          .name = "element-name",
          .description = "The name of the element on which to modify the property",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "property",
          .description = "The name of the property to modify",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "value",
          .description = "The value of the property",
          .types = "gvalue",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Allows to change child property of an object", FALSE);

  gst_validate_register_action_type ("split-clip", "ges", _split_clip,
      (GstValidateActionParameter []) {
        {
          .name = "clip-name",
          .description = "The name of the clip to split",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "position",
          .description = "The position at which to split the clip",
          .types = "double or string",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Split a clip at a specified position.", FALSE);

  gst_validate_register_action_type ("set-track-restriction-caps", "ges", _set_track_restriction_caps,
      (GstValidateActionParameter []) {
        {
          .name = "track-type",
          .description = "The type of track to set restriction caps on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "caps",
          .description = "The caps to set on the track",
          .types = "string",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Sets restriction caps on tracks of a specific type.", FALSE);

  gst_validate_register_action_type ("element-set-asset", "ges", _set_asset_on_element,
      (GstValidateActionParameter []) {
        {
          .name = "element-name",
          .description = "The name of the TimelineElement to set an asset on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "asset-id",
          .description = "The id of the asset from which to extract the clip",
          .types = "string",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Sets restriction caps on tracks of a specific type.", FALSE);


  gst_validate_register_action_type ("container-add-child", "ges", _container_add_child,
      (GstValidateActionParameter []) {
        {
          .name = "container-name",
          .description = "The name of the GESContainer to add a child to",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "child-name",
          .description = "The name of the child to add to @container-name",
          .types = "string",
          .mandatory = FALSE,
          .def = "NULL"
        },
        {
          .name = "asset-id",
          .description = "The id of the asset from which to extract the child",
          .types = "string",
          .mandatory = TRUE,
          .def = "NULL"
        },
        {
          .name = "child-type",
          .description = "The type of the child to create",
          .types = "string",
          .mandatory = FALSE,
          .def = "NULL"
        },
        {NULL}
      }, "Add a child to @container-name. If asset-id and child-type are specified,"
       " the child will be created and added. Otherwize @child-name has to be specified"
       " and will be added to the container.", FALSE);

  gst_validate_register_action_type ("container-remove-child", "ges", _container_remove_child,
      (GstValidateActionParameter []) {
        {
          .name = "container-name",
          .description = "The name of the GESContainer to remove a child from",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "child-name",
          .description = "The name of the child to reomve from @container-name",
          .types = "string",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Remove a child from @container-name.", FALSE);

  gst_validate_register_action_type ("set-control-source", "ges", _set_control_source,
      (GstValidateActionParameter []) {
        {
          .name = "element-name",
          .description = "The name of the GESTrackElement to set the control source on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "property-name",
          .description = "The name of the property for which to set a control source",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "binding-type",
          .description = "The name of the type of binding to use",
          .types = "string",
          .mandatory = FALSE,
          .def = "direct",
        },
        {
          .name = "source-type",
          .description = "The name of the type of ControlSource to use",
          .types = "string",
          .mandatory = FALSE,
          .def = "interpolation",
        },
        {
          .name = "interpolation-mode",
          .description = "The name of the GstInterpolationMode to on the source",
          .types = "string",
          .mandatory = FALSE,
          .def = "linear",
        },
        {NULL}
      }, "Adds a GstControlSource on @element-name::@property-name"
         " allowing you to then add keyframes on that property.", FALSE);

  gst_validate_register_action_type ("add-keyframe", "ges", _add_remove_keyframe,
      (GstValidateActionParameter []) {
        {
          .name = "element-name",
          .description = "The name of the GESTrackElement to add a keyframe on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "property-name",
          .description = "The name of the property for which to add a keyframe on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "timestamp",
          .description = "The timestamp of the keyframe",
          .types = "string or float",
          .mandatory = TRUE,
        },
        {
          .name = "value",
          .description = "The value of the keyframe",
          .types = "float",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Remove a child from @container-name.", FALSE);

  gst_validate_register_action_type ("remove-keyframe", "ges", _add_remove_keyframe,
      (GstValidateActionParameter []) {
        {
          .name = "element-name",
          .description = "The name of the GESTrackElement to add a keyframe on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "property-name",
          .description = "The name of the property for which to add a keyframe on",
          .types = "string",
          .mandatory = TRUE,
        },
        {
          .name = "timestamp",
          .description = "The timestamp of the keyframe",
          .types = "string or float",
          .mandatory = TRUE,
        },
        {NULL}
      }, "Remove a child from @container-name.", FALSE);



  gst_validate_register_action_type ("commit", "ges", _commit, NULL,
       "Commit the timeline.", FALSE);

  /*  *INDENT-ON* */
}


gboolean
ges_validate_activate (GstPipeline * pipeline, const gchar * scenario,
    gboolean * needs_setting_state)
{
  GstValidateOverride *o;
  GstValidateRunner *runner = NULL;
  GstValidateMonitor *monitor = NULL;

  ges_validate_register_action_types ();

  if (scenario) {
    if (g_strcmp0 (scenario, "none")) {
      gchar *scenario_name = g_strconcat (scenario, "->gespipeline*", NULL);
      g_setenv ("GST_VALIDATE_SCENARIO", scenario_name, TRUE);
      g_free (scenario_name);
    }
  }

  o = gst_validate_override_new ();
  gst_validate_override_change_severity (o,
      EVENT_SEEK_RESULT_POSITION_WRONG,
      GST_VALIDATE_REPORT_LEVEL_WARNING);
  gst_validate_override_register_by_name ("scenarios", o);

  runner = gst_validate_runner_new ();
  g_signal_connect (runner, "report-added",
      G_CALLBACK (_validate_report_added_cb), pipeline);
  monitor =
      gst_validate_monitor_factory_create (GST_OBJECT_CAST (pipeline), runner,
      NULL);

  gst_validate_reporter_set_handle_g_logs (GST_VALIDATE_REPORTER (monitor));

  g_object_get (monitor, "handles-states", needs_setting_state, NULL);
  *needs_setting_state = !*needs_setting_state;
  g_object_set_data (G_OBJECT (pipeline), MONITOR_ON_PIPELINE, monitor);
  g_object_set_data (G_OBJECT (pipeline), RUNNER_ON_PIPELINE, runner);

  return TRUE;
}

gint
ges_validate_clean (GstPipeline * pipeline)
{
  gint res = 0;
  GstValidateMonitor *monitor =
      g_object_get_data (G_OBJECT (pipeline), MONITOR_ON_PIPELINE);
  GstValidateRunner *runner =
      g_object_get_data (G_OBJECT (pipeline), RUNNER_ON_PIPELINE);

  if (runner)
    res = gst_validate_runner_printf (runner);

  gst_object_unref (pipeline);
  if (runner) {
    gst_object_unref (runner);
    if (monitor)
      gst_object_unref (monitor);
  }

  return res;
}

void
ges_validate_handle_request_state_change (GstMessage * message,
    GMainLoop * mainloop)
{
  GstState state;

  gst_message_parse_request_state (message, &state);

  if (GST_IS_VALIDATE_SCENARIO (GST_MESSAGE_SRC (message))
      && state == GST_STATE_NULL) {
    gst_validate_printf (GST_MESSAGE_SRC (message),
        "State change request NULL, " "quiting mainloop\n");
    g_main_loop_quit (mainloop);
  }
}

gint
ges_validate_print_action_types (const gchar ** types, gint num_types)
{
  ges_validate_register_action_types ();

  if (!gst_validate_print_action_types (types, num_types)) {
    GST_ERROR ("Could not print all wanted types");
    return 1;
  }

  return 0;
}

#else
static gboolean
_print_position (GstElement * pipeline)
{
  gint64 position, duration;

  if (pipeline) {
    gst_element_query_position (GST_ELEMENT (pipeline), GST_FORMAT_TIME,
        &position);
    gst_element_query_duration (GST_ELEMENT (pipeline), GST_FORMAT_TIME,
        &duration);

    g_print ("<position: %" GST_TIME_FORMAT " duration: %" GST_TIME_FORMAT
        "/>\r", GST_TIME_ARGS (position), GST_TIME_ARGS (duration));
  }

  return TRUE;
}

gboolean
ges_validate_activate (GstPipeline * pipeline, const gchar * scenario,
    gboolean * needs_setting_state)
{
  if (scenario) {
    GST_WARNING ("Trying to run scenario %s, but gst-validate not supported",
        scenario);

    return FALSE;
  }

  g_object_set_data (G_OBJECT (pipeline), "pposition-id",
      GUINT_TO_POINTER (g_timeout_add (200,
              (GSourceFunc) _print_position, pipeline)));

  *needs_setting_state = TRUE;

  return TRUE;
}

gint
ges_validate_clean (GstPipeline * pipeline)
{
  g_source_remove (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pipeline),
              "pposition-id")));

  gst_object_unref (pipeline);

  return 0;
}

void
ges_validate_handle_request_state_change (GstMessage * message,
    GMainLoop * mainloop)
{
  return;
}

gint
ges_validate_print_action_types (const gchar ** types, gint num_types)
{
  return 0;
}

#endif
