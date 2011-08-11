/* GStreamer Editing Services
 * Copyright (C) 2011 Thibault Saunier <thibault.saunier@collabora.com>
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

/**
 * SECTION:ges-timeline-group
 * @short_description: Permit to group #GESTimelineObject-s in layer
 * agnostic way.
 *
 * A #GESTimelineGroup is an object that permits to group #GESTimelineObject
 * so you can use them as one single object.
 *
 * Since: 0.10.3
 */

#include "ges-internal.h"
#include "gesmarshal.h"
#include "ges.h"
#include "ges-timeline-object.h"
#include "ges-timeline-group.h"

G_DEFINE_TYPE (GESTimelineGroup, ges_timeline_group, GES_TYPE_TIMELINE_OBJECT);

/* Mappings */
typedef struct
{
  GESTimelineObject *object;

  gint64 start_offset;
  gint64 duration_offset;
  gint64 inpoint_offset;
  gint32 priority_offset;


  guint start_notifyid;
  guint duration_notifyid;
  guint inpoint_notifyid;
  guint priority_notifyid;

} ObjectMapping;

struct _GESTimelineGroupPrivate
{
  /*  dummy variable */
  GList *mappings;
  GList *group_by_layer;
};

enum
{
  PROP_0,
};

enum
{
  OBJECT_ADDED,
  OBJECT_REMOVED,
  LAST_SIGNAL
};

static guint ges_timeline_group_signals[LAST_SIGNAL] = { 0 };

static ObjectMapping *
find_object_mapping (GESTimelineGroup * group, GESTimelineObject * child)
{
  GList *tmp;

  for (tmp = group->priv->mappings; tmp; tmp = tmp->next) {
    ObjectMapping *map = (ObjectMapping *) tmp->data;
    if (map->object == child)
      return map;
  }

  return NULL;
}

/* GCompareFunc that aims at keeping TimelineObjects properly
 * sorted in a GList*/
static gint
mapping_start_compare (ObjectMapping * mapa, ObjectMapping * mapb)
{
  GESTimelineObject *a = mapa->object;
  GESTimelineObject *b = mapb->object;
  if (a->start == b->start) {
    if (a->priority < b->priority)
      return -1;
    if (a->priority > b->priority)
      return 1;
    return 0;
  }
  if (a->start < b->start)
    return -1;
  if (a->start > b->start)
    return 1;
  return 0;
}

static void
calculate_group_values (GESTimelineGroup * group, GESTimelineObject * object,
    gboolean layer_aware)
{
  if (layer_aware &&
      GES_TIMELINE_OBJECT_PRIORITY (object) <
      GES_TIMELINE_OBJECT_PRIORITY (group))
    group->parent.priority = object->priority;

  if (object->start < group->parent.start)
    group->parent.start = object->start;

  if (object->start + object->duration >
      group->parent.start + group->parent.duration)
    group->parent.duration = object->start + object->duration;

  /* TODO: Implement the in-point */
}

/*
 * PROPERTY NOTIFICATIONS FROM TIMELINE OBJECTS
 */
static void
timeline_object_start_changed_cb (GESTimelineObject * child,
    GParamSpec * arg G_GNUC_UNUSED, GESTimelineGroup * group)
{
  ObjectMapping *map;

  map = find_object_mapping (group, child);
  if (G_UNLIKELY (map == NULL))
    /* something massively screwed up if we get this */
    return;

  if (child->start < group->parent.start) {
    g_object_set (G_OBJECT (group), "start", child->start, NULL);
  }


}

static void
timeline_object_inpoint_changed_cb (GESTrackObject * child,
    GParamSpec * arg G_GNUC_UNUSED, GESTimelineObject * object)
{

  return;
}

static void
timeline_object_duration_changed_cb (GESTrackObject * child,
    GParamSpec * arg G_GNUC_UNUSED, GESTimelineObject * object)
{
  return;

}

static ObjectMapping *
create_mapping (GESTimelineGroupPrivate * priv, GESTimelineObject * object)
{
  ObjectMapping *mapping;

  mapping = g_slice_new0 (ObjectMapping);
  mapping->object = object;

  mapping->start_notifyid =
      g_signal_connect (G_OBJECT (object), "notify::start",
      G_CALLBACK (timeline_object_start_changed_cb), object);
  mapping->duration_notifyid =
      g_signal_connect (G_OBJECT (object), "notify::duration",
      G_CALLBACK (timeline_object_duration_changed_cb), object);
  mapping->inpoint_notifyid =
      g_signal_connect (G_OBJECT (object), "notify::inpoint",
      G_CALLBACK (timeline_object_inpoint_changed_cb), object);

  priv->mappings = g_list_insert_sorted (priv->mappings, mapping,
      (GCompareFunc) mapping_start_compare);

  return mapping;
}

static void
update_height (GESTimelineGroup * group)
{
  GList *tmp, *tmpmap;
  guint32 min_prio = G_MAXUINT32, max_prio = 0;
  GESTimelineObject *object;

  /* Go over all childs and check if height has changed */
  for (tmpmap = group->priv->mappings; tmpmap; tmpmap = tmpmap->next) {
    GList *tckobjects;

    object = ((ObjectMapping *) tmpmap)->object;
    tckobjects = ges_timeline_object_get_track_objects (object);
    /* Go over all childs and check if height has changed */
    for (tmp = tckobjects; tmp; tmp = tmp->next) {
      guint tck_priority =
          ges_track_object_get_priority (GES_TRACK_OBJECT (tmp->data));

      if (tck_priority < min_prio)
        min_prio = tck_priority;
      if (tck_priority > max_prio)
        max_prio = tck_priority;
      g_object_unref (tmp->data);
    }
    g_list_free (tckobjects);
  }

  /* FIXME : We only grow the height */
  if (group->parent.height < (max_prio - min_prio + 1))
    g_object_set (group, "height", max_prio - min_prio + 1, NULL);
}

static gboolean
ges_timeline_group_add_internal (GESTimelineGroup * group,
    GESTimelineObject * object, const gboolean emit_signal,
    gboolean layer_aware)
{
  create_mapping (group->priv, object);

  if (layer_aware)
    update_height (group);

  return TRUE;
}


static void
ges_timeline_group_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_timeline_group_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_timeline_group_finalize (GObject * object)
{
  G_OBJECT_CLASS (ges_timeline_group_parent_class)->finalize (object);
}

static void
ges_timeline_group_class_init (GESTimelineGroupClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTimelineGroupPrivate));

  object_class->get_property = ges_timeline_group_get_property;
  object_class->set_property = ges_timeline_group_set_property;
  object_class->finalize = ges_timeline_group_finalize;

  /**
   * GESTimelineGroup::object-removed:
   * @group: the #GESTimelineGroup
   * @object: the #GESTimelineObject that was removed from the group
   *
   * Will be emitted after the object was removed from the group.
   *
   * Since: 0.10.3
   */
  ges_timeline_group_signals[OBJECT_REMOVED] =
      g_signal_new ("object-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, ges_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, GES_TYPE_TIMELINE_OBJECT);

  /**
   * GESTimelineGroup::object-added:
   * @group: the #GESTimelineGroup
   * @object: the #GESTimelineObject that was added to the group
   *
   * Will be emitted after the object was added to the group.
   *
   * Since: 0.10.3
   */
  ges_timeline_group_signals[OBJECT_ADDED] =
      g_signal_new ("object-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, ges_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, GES_TYPE_TIMELINE_OBJECT);
}

static void
ges_timeline_group_init (GESTimelineGroup * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_TIMELINE_GROUP, GESTimelineGroupPrivate);

  /* Height == 0 means that the group is layer agnostic */
  self->parent.height = 0;
}

/**
 * ges_timeline_group_new_from_many:
 * @objects: (element-type GES.TimelineGroup)(transfer none) A #GList of #GESTimelineObject-s
 *
 * Creates a new #GESTimelineGroup from the #GESTimelineObject-s you passed
 * in parameters
 *
 * Returns: (transfer full): the newly created #GESTimelineGroup, resulting
 * from the linking of the objects passed in parameter or %NULL if something
 * went wrong.
 *
 * Since: 0.10.3
 */
GESTimelineGroup *
ges_timeline_group_new_from_many (GList * objects)
{
  GESTimelineGroupPrivate *priv;
  GESTimelineObject *object;
  GESTimelineGroup *new_object, *gpe;
  GList *tmpobjects, *tmpgroups, *tmp;
  gboolean layer_found;
  guint64 start = G_MAXUINT64, objend = 0, end = 0;

  GST_DEBUG ("Creating a new timeline group object");
  new_object = g_object_new (GES_TYPE_TIMELINE_GROUP, NULL);
  priv = new_object->priv;

  for (tmpobjects = objects; tmpobjects; tmpobjects = g_list_next (tmpobjects)) {
    GESTimelineLayer *objlayer;

    object = GES_TIMELINE_OBJECT (tmpobjects->data);
    objlayer = ges_timeline_object_get_layer (object);

    if (objlayer) {
      /*  We keep a list of group of object by layers */
      for (tmpgroups = priv->group_by_layer; tmpgroups;
          tmpgroups = g_list_next (tmpgroups)) {
        GESTimelineLayer *layer;
        gpe = GES_TIMELINE_GROUP (tmpgroups->data);

        layer = ges_timeline_object_get_layer (GES_TIMELINE_OBJECT (gpe));

        /* We found a group in the same layer as the current object,
         * we add this object to this group */
        if (layer == objlayer) {
          ges_timeline_group_add_internal (gpe, object, FALSE, TRUE);

          calculate_group_values (gpe, object, TRUE);

          layer_found = TRUE;
        }
      }
    } else {
      ges_timeline_group_add_internal (new_object, object, FALSE, FALSE);

      layer_found = TRUE;
    }

    /* We create a new group for this layer */
    if (!layer_found) {
      gpe = g_object_new (GES_TYPE_TIMELINE_GROUP, NULL);
      ges_timeline_group_add_internal (gpe, object, FALSE, TRUE);
      calculate_group_values (gpe, object, TRUE);
    }

    /* We check what the properties of the new object should be */
    if (object->start < start)
      start = object->start;

    objend = object->start + object->duration;
    if (objend > end)
      end = objend;

    create_mapping (priv, object);
  }

  /* We set the properties */
  new_object->parent.start = start;
  new_object->parent.duration = end - start;

  for (tmp = priv->mappings; tmp; tmp = g_list_next (tmp)) {
    ObjectMapping *map = tmp->data;
    object = map->object;
  }

  return new_object;
}
