/* GStreamer
 * Copyright (C) 2001 Wim Taymans <wim.taymans@gmail.com>
 *               2004-2008 Edward Hervey <bilboed@bilboed.com>
 *
 * gnlobject.h: Header for base GnlObject
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


#ifndef __GNL_OBJECT_H__
#define __GNL_OBJECT_H__

#include <gst/gst.h>

#include "gnltypes.h"

G_BEGIN_DECLS
#define GNL_TYPE_OBJECT \
  (gnl_object_get_type())
#define GNL_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GNL_TYPE_OBJECT,GnlObject))
#define GNL_OBJECT_CAST(obj) ((GnlObject*) (obj))
#define GNL_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GNL_TYPE_OBJECT,GnlObjectClass))
#define GNL_OBJECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNL_TYPE_OBJECT, GnlObjectClass))
#define GNL_IS_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GNL_TYPE_OBJECT))
#define GNL_IS_OBJECT_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GNL_TYPE_OBJECT))

/**
 * GnlObjectFlags:
 * @GNL_OBJECT_IS_SOURCE:
 * @GNL_OBJECT_IS_OPERATION:
 * @GNL_OBJECT_IS_EXPANDABLE: The #GnlObject start/stop will extend accross the full composition.
 * @GNL_OBJECT_LAST_FLAG:
*/

typedef enum
{
  GNL_OBJECT_SOURCE = (GST_BIN_FLAG_LAST << 0),
  GNL_OBJECT_OPERATION = (GST_BIN_FLAG_LAST << 1),
  GNL_OBJECT_EXPANDABLE = (GST_BIN_FLAG_LAST << 2),
  /* padding */
  GNL_OBJECT_LAST_FLAG = (GST_BIN_FLAG_LAST << 5)
} GnlObjectFlags;


#define GNL_OBJECT_IS_SOURCE(obj) \
  (GST_OBJECT_FLAG_IS_SET(obj, GNL_OBJECT_SOURCE))
#define GNL_OBJECT_IS_OPERATION(obj) \
  (GST_OBJECT_FLAG_IS_SET(obj, GNL_OBJECT_OPERATION))
#define GNL_OBJECT_IS_EXPANDABLE(obj) \
  (GST_OBJECT_FLAG_IS_SET(obj, GNL_OBJECT_EXPANDABLE))

/**
 * GNL_OBJECT_START:
 * @obj: a #GnlObject
 *
 * The start position of the object (in nanoseconds).
 */
#define GNL_OBJECT_START(obj) (GNL_OBJECT_CAST (obj)->start)

/**
 * GNL_OBJECT_STOP:
 * @obj: a #GnlObject
 *
 * The stop time of the object (in nanoseconds).
 */
#define GNL_OBJECT_STOP(obj) (GNL_OBJECT_CAST (obj)->stop)

/**
 * GNL_OBJECT_DURATION:
 * @obj: a #GnlObject
 *
 * The duration of the object (in nanoseconds).
 */
#define GNL_OBJECT_DURATION(obj) (GNL_OBJECT_CAST (obj)->duration)

/**
 * GNL_OBJECT_INPOINT:
 * @obj: a #GnlObject
 *
 * The inpoint position of the object (in nanoseconds).
 */
#define GNL_OBJECT_INPOINT(obj) (GNL_OBJECT_CAST (obj)->inpoint)

/**
 * GNL_OBJECT_MEDIA_STOP:
 * @obj: a #GnlObject
 *
 * The media stop of the object (in nanoseconds).
 */
#define GNL_OBJECT_MEDIA_STOP(obj) (GNL_OBJECT_CAST (obj)->media_stop)

/**
 * GNL_OBJECT_MEDIA_DURATION:
 * @obj: a #GnlObject
 *
 * The media duration the object (in nanoseconds).
 */
#define GNL_OBJECT_MEDIA_DURATION(obj) (GNL_OBJECT_CAST (obj)->media_duration)

/**
 * GNL_OBJECT_PRIORITY:
 * @obj: a #GnlObject
 *
 * The priority of the object (in nanoseconds).
 */
#define GNL_OBJECT_PRIORITY(obj) (GNL_OBJECT_CAST (obj)->priority)

/**
 * GNL_OBJECT_ACTIVE:
 * @obj: a #GnlObject
 *
 * Whether @object is active or not
 */
#define GNL_OBJECT_ACTIVE(obj) (GNL_OBJECT_CAST (obj)->active)

struct _GnlObject
{
  GstBin parent;

  /* Time positionning */
  GstClockTime start;
  GstClockTimeDiff duration;

  /* read-only */
  GstClockTime stop;

  GstClockTime inpoint;
  GstClockTimeDiff media_duration;

  /* read-only */
  GstClockTime media_stop;

  /* read-only */
  gdouble rate;
  /* TRUE if rate == 1.0 */
  gboolean rate_1;

  /* priority in parent */
  guint32 priority;

  /* active in parent */
  gboolean active;

  /* Filtering caps */
  GstCaps *caps;

  /* current segment seek <RO> */
  gdouble segment_rate;
  GstSeekFlags segment_flags;
  gint64 segment_start;
  gint64 segment_stop;
};

struct _GnlObjectClass
{
  GstBinClass parent_class;

  /* virtual methods for subclasses */
  gboolean (*prepare) (GnlObject * object);
  gboolean (*cleanup) (GnlObject * object);

  /* Let subclasses know about our properties changes without passing
   * through the signaling system */
  void (*duration_changed) (GnlObject *object, GstClockTime duration);
  void (*media_duration_changed) (GnlObject *object, GstClockTime mduration);
  void (*start_changed) (GnlObject *object, GstClockTime start);
  void (*inpoint_changed) (GnlObject *object, GstClockTime mstart);
  void (*priority_changed) (GnlObject *object, guint32 priority);
  void (*active_changed) (GnlObject *object, gboolean active);

};

GType gnl_object_get_type (void);

gboolean
gnl_object_to_media_time (GnlObject * object, GstClockTime otime,
			  GstClockTime * mtime);

gboolean
gnl_media_to_object_time (GnlObject * object, GstClockTime mtime,
			  GstClockTime * otime);

void
gnl_object_set_caps           (GnlObject * object, const GstCaps * caps);

void gnl_object_set_duration       (GnlObject *object, GstClockTime duration);
void gnl_object_set_media_duration (GnlObject *object, GstClockTime mduration);
void gnl_object_set_start          (GnlObject *object, GstClockTime start);
void gnl_object_set_inpoint    (GnlObject *object, GstClockTime mstart);
void gnl_object_set_priority       (GnlObject *object, guint32 priority);
void gnl_object_set_active         (GnlObject *object, gboolean active);

G_END_DECLS
#endif /* __GNL_OBJECT_H__ */
