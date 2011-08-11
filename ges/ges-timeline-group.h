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
 */

#ifndef _GES_TIMELINE_GROUP
#define _GES_TIMELINE_GROUP

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-types.h>

G_BEGIN_DECLS

#define GES_TYPE_TIMELINE_GROUP ges_timeline_object_get_type()

#define GES_TIMELINE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TIMELINE_GROUP, GESTimelineGroup))

#define GES_TIMELINE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TIMELINE_GROUP, GESTimelineGroupClass))

#define GES_IS_TIMELINE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TIMELINE_GROUP))

#define GES_IS_TIMELINE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TIMELINE_GROUP))

#define GES_TIMELINE_GROUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TIMELINE_GROUP, GESTimelineGroupClass))

typedef struct _GESTimelineGroupPrivate GESTimelineGroupPrivate;

struct _GESTimelineGroup {
  /*< private >*/
  GESTimelineObject parent;

  GESTimelineGroupPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESTimelineGroupClass {
  /*< private >*/
  GESTimelineObjectClass parent_class;

  /*< private >*/
  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GESTimelineGroup * ges_timeline_group_new_from_many (GList *objects);
gboolean ges_timeline_group_add (GESTimelineGroup *group,
      GESTimelineObject *object);
gboolean ges_timeline_group_remove (GESTimelineGroup *group,
      GESTimelineObject *object);
GList * ges_timeline_group_get_children             (GESTimelineGroup *group,
      gboolean recursive);
GList * ges_timeline_group_release_objects (GESTimelineGroup *group,
      gboolean recursive);
GESTimelineGroup * ges_timeline_group_new ();

GType ges_timeline_group_get_type (void);

G_END_DECLS

#endif /* _GES_TIMELINE_GROUP */

