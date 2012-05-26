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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _GES_TRACK_OBJECT
#define _GES_TRACK_OBJECT

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-types.h>
#include <ges/ges-timeline-object.h>
#include <ges/ges-track.h>
#include "../gnl/gnlobject.h"

G_BEGIN_DECLS

#define GES_TYPE_TRACK_OBJECT ges_track_object_get_type()

#define GES_TRACK_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_TRACK_OBJECT, GESTrackObject))

#define GES_TRACK_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_TRACK_OBJECT, GESTrackObjectClass))

#define GES_IS_TRACK_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_TRACK_OBJECT))

#define GES_IS_TRACK_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_TRACK_OBJECT))

#define GES_TRACK_OBJECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_TRACK_OBJECT, GESTrackObjectClass))

typedef struct _GESTrackObjectPrivate GESTrackObjectPrivate;

/**
 * GESTrackObject:
 *
 * The GESTrackObject base class.
 */
struct _GESTrackObject {
  GnlObject parent;

  GESTrackObjectPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESTrackObjectClass:
 * @gnlobject_factorytype: name of the GNonLin GStElementFactory type to use.
 * @create_gnl_object: method to create the GNonLin container object.
 * @create_element: method to return the GstElement to put in the gnlobject.
 * @list_children_properties: method to get children properties that user could
 *                            like to configure.
 *                            The default implementation will create an object
 *                            of type @gnlobject_factorytype and call
 *                            @create_element. Since: 0.10.2
 *
 * Subclasses can override the @create_gnl_object method to override what type
 * of GNonLin object will be created.
 */ 
struct _GESTrackObjectClass {
  /*< private >*/
  GnlObjectClass parent_class;

  /*< public >*/
  /* virtual methods for subclasses */
  const gchar  *gnlobject_factorytype;
  GnlObject*  (*create_gnl_object)        (GESTrackObject * object);
  GstElement*  (*create_element)           (GESTrackObject * object);

  /*< private >*/
  /* signals (currently unused) */
  void  (*changed)  (GESTrackObject * object);

  /*< public >*/
  /* virtual methods for subclasses */
  GHashTable*  (*get_props_hastable)       (GESTrackObject * object);
  GParamSpec** (*list_children_properties) (GESTrackObject * object,
              guint *n_properties);
  /*< private >*/
  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING - 2];
};

GType ges_track_object_get_type               (void);

gboolean ges_track_object_set_track           (GESTrackObject * object,
              GESTrack * track);
GESTrack* ges_track_object_get_track          (GESTrackObject * object);

void ges_track_object_set_timeline_object     (GESTrackObject * object,
                                               GESTimelineObject * tlobject);
GESTimelineObject *
ges_track_object_get_timeline_object          (GESTrackObject* object);

GnlObject * ges_track_object_get_gnlobject   (GESTrackObject * object);

GstElement * ges_track_object_get_element     (GESTrackObject * object);

void ges_track_object_set_locked              (GESTrackObject * object,
                                               gboolean locked);

gboolean ges_track_object_is_locked           (GESTrackObject * object);

GParamSpec **
ges_track_object_list_children_properties     (GESTrackObject *object,
                                               guint *n_properties);

gboolean ges_track_object_lookup_child        (GESTrackObject *object,
                                               const gchar *prop_name,
                                               GstElement **element,
                                               GParamSpec **pspec);

void
ges_track_object_get_child_property_by_pspec (GESTrackObject * object,
                                              GParamSpec * pspec,
                                              GValue * value);

void
ges_track_object_get_child_property_valist   (GESTrackObject * object,
                                              const gchar * first_property_name,
                                              va_list var_args);

void ges_track_object_get_child_property     (GESTrackObject *object,
                                              const gchar * first_property_name,
                                              ...) G_GNUC_NULL_TERMINATED;

void
ges_track_object_set_child_property_valist   (GESTrackObject * object,
                                              const gchar * first_property_name,
                                              va_list var_args);

void
ges_track_object_set_child_property_by_pspec (GESTrackObject * object,
                                              GParamSpec * pspec,
                                              GValue * value);

void ges_track_object_set_child_property     (GESTrackObject * object,
                                              const gchar * first_property_name,
                                              ...) G_GNUC_NULL_TERMINATED;

GESTrackObject * ges_track_object_copy       (GESTrackObject * object,
                                              gboolean deep);

gboolean
ges_track_object_edit                        (GESTrackObject * object,
                                              GList *layers, GESEditMode mode,
                                              GESEdge edge, guint64 position);

void ges_track_object_set_max_duration        (GESTrackObject * object,
                                               guint64 maxduration);
guint64
ges_track_object_get_max_duration (GESTrackObject * object);

G_END_DECLS
#endif /* _GES_TRACK_OBJECT */
