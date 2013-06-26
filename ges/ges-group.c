/* GStreamer Editing Services
 * Copyright (C) 2013 Thibault Saunier <thibault.saunier@collabora.com>
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
 * SECTION:ges-group
 * @short_description: Class that permits to group GESClip-s in a timeline,
 * letting the user manage it a single GESTimelineElement
 *
 * A #GESGroup is an object which controls one or more
 * #GESClips in one or more #GESLayer(s).
 *
 * To instanciate a group, you should use the ges_container_group method,
 * this will be responsible for deciding what subclass of #GESContainer
 * should be instaciated to group the various #GESTimelineElement passed
 * in parametter.
 */

#include "ges-group.h"
#include "ges.h"
#include "ges-internal.h"

#include <string.h>

#define parent_class ges_group_parent_class
G_DEFINE_TYPE (GESGroup, ges_group, GES_TYPE_CONTAINER);

struct _GESGroupPrivate
{
  gboolean reseting_start;

  guint32 min_layer_prio;
  guint32 max_layer_prio;

  /* This is used while were are setting ourselve a proper timing value,
   * in this case the value should always be kept */
  gboolean setting_value;
};

enum
{
  PROP_0,
  PROP_LAST
};

/* static GParamSpec *properties[PROP_LAST]; */

/****************************************************
 *              Our listening of children           *
 ****************************************************/
static inline void
_update_our_prios (GESGroupPrivate * priv, GESLayer * layer)
{
  priv->min_layer_prio = MIN (priv->min_layer_prio,
      ges_layer_get_priority (layer));
  priv->max_layer_prio = MAX (priv->min_layer_prio,
      ges_layer_get_priority (layer));

  gst_object_unref (layer);
}

static void
_child_changed_layer_cb (GESClip * clip,
    GParamSpec * arg G_GNUC_UNUSED, GESGroup * group)
{
  GESLayer *layer = ges_clip_get_layer (clip);
  GESGroupPrivate *priv = group->priv;

  if (layer == NULL) {
    GList *tmp;

    for (tmp = GES_CONTAINER_CHILDREN (group); tmp; tmp = tmp->next) {
      layer = ges_clip_get_layer (GES_CLIP (tmp->data));
      _update_our_prios (priv, layer);
    }

    /* FIXME Figure out what we want to do here! */
    return;
  }
  _update_our_prios (priv, layer);
}

/****************************************************
 *              GESTimelineElement vmethods         *
 ****************************************************/
static gboolean
_ripple (GESTimelineElement * group, GstClockTime start)
{
  gboolean ret = TRUE;

  return ret;
}

static gboolean
_ripple_end (GESTimelineElement * group, GstClockTime end)
{
  gboolean ret = TRUE;

  return ret;
}

static gboolean
_roll_start (GESTimelineElement * group, GstClockTime start)
{
  gboolean ret = TRUE;

  return ret;
}

static gboolean
_roll_end (GESTimelineElement * group, GstClockTime end)
{
  gboolean ret = TRUE;

  return ret;
}

static gboolean
_trim (GESTimelineElement * group, GstClockTime start)
{
  GList *tmp;
  GstClockTime last_child_end = 0;
  GESContainer *container = GES_CONTAINER (group);
  GESTimeline *timeline = GES_TIMELINE_ELEMENT_TIMELINE (group);
  gboolean ret = TRUE, expending = (start < _START (group));

  if (timeline == NULL) {
    GST_DEBUG ("Not in a timeline yet");

    return FALSE;
  }

  _ges_container_set_children_control_mode (container,
      GES_CHILDREN_IGNORE_NOTIFIES);
  for (tmp = GES_CONTAINER_CHILDREN (group); tmp; tmp = tmp->next) {
    GESTimelineElement *child = tmp->data;

    if (expending) {
      /* If the start if bigger, we do not touch it (in case we are expending)
       */
      if (_START (child) > _START (group))
        continue;
      ret &= ges_timeline_element_trim (child, start);
    } else {
      if (start > _END (child))
        ret &= ges_timeline_element_trim (child, _END (child));
      else if (_START (child) < start && _DURATION (child))
        ret &= ges_timeline_element_trim (child, start);

    }
  }

  for (tmp = GES_CONTAINER_CHILDREN (group); tmp; tmp = tmp->next) {
    if (_DURATION (tmp->data))
      last_child_end =
          MAX (GES_TIMELINE_ELEMENT_END (tmp->data), last_child_end);
  }

  GES_GROUP (group)->priv->setting_value = TRUE;
  _set_start0 (group, start);
  _set_duration0 (group, last_child_end - start);
  GES_GROUP (group)->priv->setting_value = FALSE;
  _ges_container_set_children_control_mode (container, GES_CHILDREN_UPDATE);

  return ret;
}

static gboolean
_set_start (GESTimelineElement * element, GstClockTime start)
{
  GList *tmp;
  gint64 diff = start - _START (element);
  GESContainer *container = GES_CONTAINER (element);

  if (GES_GROUP (element)->priv->setting_value == TRUE)
    /* Let GESContainer update itself */
    return GES_TIMELINE_ELEMENT_CLASS (parent_class)->set_start (element,
        start);


  _ges_container_set_children_control_mode (container,
      GES_CHILDREN_IGNORE_NOTIFIES);
  for (tmp = GES_CONTAINER_CHILDREN (element); tmp; tmp = tmp->next) {
    GESTimelineElement *child = (GESTimelineElement *) tmp->data;

    if (child != container->initiated_move && _END (child) > start)
      _set_start0 (child, _START (child) + diff);
  }
  _ges_container_set_children_control_mode (container, GES_CHILDREN_UPDATE);

  return TRUE;
}

static gboolean
_set_inpoint (GESTimelineElement * element, GstClockTime inpoint)
{
  return FALSE;
}

static gboolean
_set_duration (GESTimelineElement * element, GstClockTime duration)
{
  GList *tmp;
  GstClockTime last_child_end = 0, new_end;
  GESContainer *container = GES_CONTAINER (element);
  GESGroupPrivate *priv = GES_GROUP (element)->priv;

  if (priv->setting_value == TRUE)
    /* Let GESContainer update itself */
    return GES_TIMELINE_ELEMENT_CLASS (parent_class)->set_duration (element,
        duration);

  if (container->initiated_move == NULL) {
    gboolean expending = (_DURATION (element) < duration);

    new_end = _START (element) + duration;
    _ges_container_set_children_control_mode (container,
        GES_CHILDREN_IGNORE_NOTIFIES);
    for (tmp = GES_CONTAINER_CHILDREN (element); tmp; tmp = tmp->next) {
      GESTimelineElement *child = tmp->data;
      GstClockTime n_dur;

      if ((!expending && _END (child) > new_end) ||
          (expending && (_END (child) >= _END (element)))) {
        n_dur = MAX (0, ((gint64) (new_end - _START (child))));
        _set_duration0 (child, n_dur);
      }
    }
    _ges_container_set_children_control_mode (container, GES_CHILDREN_UPDATE);
  }

  for (tmp = GES_CONTAINER_CHILDREN (element); tmp; tmp = tmp->next) {
    if (_DURATION (tmp->data))
      last_child_end =
          MAX (GES_TIMELINE_ELEMENT_END (tmp->data), last_child_end);
  }

  priv->setting_value = TRUE;
  _set_duration0 (element, last_child_end - _START (element));
  priv->setting_value = FALSE;

  return FALSE;
}

/****************************************************
 *                                                  *
 *  GESContainer virtual methods implementation     *
 *                                                  *
 ****************************************************/

static void
_get_priorty_range (GESContainer * group, guint32 * min_priority,
    guint32 * max_priority)
{
  *min_priority = GES_GROUP (group)->priv->min_layer_prio;
  *max_priority = GES_GROUP (group)->priv->max_layer_prio;
}

static void
_child_added (GESContainer * group, GESTimelineElement * child)
{
  GList *children, *tmp;

  GESGroupPrivate *priv = GES_GROUP (group)->priv;
  GstClockTime last_child_end = 0, first_child_start = G_MAXUINT64;

  children = GES_CONTAINER_CHILDREN (group);

  for (tmp = children; tmp; tmp = tmp->next) {
    last_child_end = MAX (GES_TIMELINE_ELEMENT_END (tmp->data), last_child_end);
    first_child_start =
        MIN (GES_TIMELINE_ELEMENT_START (tmp->data), first_child_start);
  }

  priv->setting_value = TRUE;
  if (first_child_start != GES_TIMELINE_ELEMENT_START (group)) {
    _ges_container_set_children_control_mode (group,
        GES_CHILDREN_UPDATE_OFFSETS);
    _set_start0 (GES_TIMELINE_ELEMENT (group), first_child_start);
  }

  if (last_child_end != GES_TIMELINE_ELEMENT_END (group)) {
    _set_duration0 (GES_TIMELINE_ELEMENT (group),
        last_child_end - first_child_start);
  }
  priv->setting_value = FALSE;

  _ges_container_set_children_control_mode (group, GES_CHILDREN_UPDATE);

  g_signal_connect (G_OBJECT (child), "notify::layer",
      G_CALLBACK (_child_changed_layer_cb), group);
}

static guint32
_compute_height (GESContainer * container)
{
  GESGroupPrivate *priv = GES_GROUP (container)->priv;

  return priv->max_layer_prio - priv->min_layer_prio + 1;
}

static void
_child_removed (GESContainer * group, GESTimelineElement * child)
{
  GList *children;
  GstClockTime first_child_start;

  _ges_container_sort_children (group);

  children = GES_CONTAINER_CHILDREN (group);

  if (children == NULL) {
    GST_FIXME_OBJECT (group, "Auto destroy myself?");
    return;
  }

  first_child_start = GES_TIMELINE_ELEMENT_START (children->data);
  if (first_child_start > GES_TIMELINE_ELEMENT_START (group)) {
    _ges_container_set_children_control_mode (group,
        GES_CHILDREN_UPDATE_OFFSETS);
    _set_start0 (GES_TIMELINE_ELEMENT (group), first_child_start);
    _ges_container_set_children_control_mode (group, GES_CHILDREN_UPDATE);
  }
}

static GList *
_ungroup (GESContainer * group, gboolean recursive)
{
  GList *ret = NULL;

  return ret;
}

static GESContainer *
_group (GList * containers)
{
  GList *tmp;
  GESTimeline *timeline = NULL;
  GESContainer *ret = g_object_new (GES_TYPE_GROUP, NULL);

  for (tmp = containers; tmp; tmp = tmp->next) {
    if (!timeline)
      timeline = GES_TIMELINE_ELEMENT_TIMELINE (tmp->data);
    else if (timeline != GES_TIMELINE_ELEMENT_TIMELINE (tmp->data)) {
      g_object_unref (ret);

      return NULL;
    }

    ges_container_add (ret, tmp->data);
  }

  ges_timeline_element_set_timeline (GES_TIMELINE_ELEMENT (ret), timeline);

  return ret;

}


/****************************************************
 *                                                  *
 *    GObject virtual methods implementation        *
 *                                                  *
 ****************************************************/
static void
ges_group_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_group_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_group_class_init (GESGroupClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESContainerClass *container_class = GES_CONTAINER_CLASS (klass);
  GESTimelineElementClass *element_class = GES_TIMELINE_ELEMENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESGroupPrivate));

  object_class->get_property = ges_group_get_property;
  object_class->set_property = ges_group_set_property;

  element_class->ripple = _ripple;
  element_class->ripple_end = _ripple_end;
  element_class->roll_start = _roll_start;
  element_class->roll_end = _roll_end;
  element_class->trim = _trim;
  element_class->set_duration = _set_duration;
  element_class->set_inpoint = _set_inpoint;
  element_class->set_start = _set_start;
  /* TODO implement the deep_copy Virtual method */

  container_class->get_priority_range = _get_priorty_range;
  container_class->child_added = _child_added;
  container_class->child_removed = _child_removed;
  container_class->ungroup = _ungroup;
  container_class->group = _group;
  container_class->grouping_priority = 0;
  container_class->compute_height = _compute_height;
}

static void
ges_group_init (GESGroup * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_GROUP, GESGroupPrivate);

  self->priv->setting_value = FALSE;
}
