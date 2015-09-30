Architecture
============

A GES Timeline is composed of one or more layers, and produces data on one or more tracks.

A track represents a media type, for example audio, video or text. For the moment only
audio and video tracks are supported.

Layers contain clips, and represent the relative priorities of these clips.

## Layers

Let's imagine a timeline that contains two layers:

This code creates a timeline:

In C:
``` lang=c
int main (int argc, char **argv)
{
    GESTimeline *timeline;

    timeline = ges_timeline_new();
}
```

In python:
``` lang=python
from gi.repository import GES

if __name__=="__main__":
    timeline = GES.Timeline()
```

### Timeline, duration : 0 seconds

```
+--------------------------------------------------------+
|                                                        |
|         +-------------------------------------------+  |
|         |                                           |  |
| Layer 1 |                                           |  |
|         |                                           |  |
|         +-------------------------------------------+  |
|                                                        |
|         +-------------------------------------------+  |
|         |                                           |  |
| Layer 2 |                                           |  |
|         |                                           |  |
|         +-------------------------------------------+  |
|                                                        |
+--------------------------------------------------------+
```

If we add two clips in layer 1 and layer 2, each starting at 0 with a duration
of 10 seconds, our timeline now looks like this:

### Timeline, duration : 10 seconds

```
+--------------------------------------------------------+
|                                                        |
|         +-------------------------------------------+  |
|         | +=======================================+ |  |
| Layer 1 | |              Clip 1                   | |  |
|         | +=======================================+ |  |
|         +-------------------------------------------+  |
|                                                        |
|         +-------------------------------------------+  |
|         | +=======================================+ |  |
| Layer 2 | |              Clip 2                   | |  |
|         | +=======================================+ |  |
|         +-------------------------------------------+  |
|                                                        |
+--------------------------------------------------------+
```

In that timeline, Clip 2 is said to have the highest "priority". In the case of
video streams, it means that it will be rendered on top of Clip 1, which in
turn signifies that if Clip 1 and Clip 2 share the same width and height, Clip
1 will be completely invisible.

In the case of video streams, one can therefore think of the layer priority as
a z-index.

## Tracks

Let's continue with that timeline. For now it indeed mixes various layers
together, but it doesn't output anything. That's what Tracks are for.

If we add a video track to our timeline, it can be represented that way:

Timeline, duration : 10 seconds

```
+--------------------------------------------------------+
|                                                        |
|         +-------------------------------------------+  |
|         | +=======================================+ |  |
| Layer 1 | |              Clip 1                   | |  |
|         | +=======================================+ |  |
|         +-------------------------------------------+  |
|                                                        |--------> Video Track
|         +-------------------------------------------+  |
|         | +=======================================+ |  |
| Layer 2 | |              Clip 2                   | |  |
|         | +=======================================+ |  |
|         +-------------------------------------------+  |
|                                                        |
+--------------------------------------------------------+
```

Assuming both clips contain video streams, our timeline will now output the
result of their compositing together on the video track. Any audio streams they
contain are ignored.

One can of course add an audio track to get audio output.

Having multiple tracks with the same type, for example 3 audio tracks, is
useful if one wants to link the timeline to a container that can store various
languages, for example a DVD creator might store English, Dutch and French
audio tracks.

By default [ges-launch-1.0](ges-launch.markdown) will assume the user wants
exactly one audio track and one video track, this can be modified through the
`--track-types` argument to `ges-launch-1.0`.


Timeline and TimelinePipeline
-----------------------------

The most top-level object encapsulating every other object is the
[GESTimeline](#GESTimeline). It is the central object for any editing
project.

The `GESTimeline` is a `GstElement`. It can therefore be used in any
GStreamer pipeline like any other object.

Tracks and Layers
-----------------

The GESTimeline can contain two types of objects (seen in ?):

-   Layers - Corresponds to the user-visible arrangement of clips, and
    what you primarily interact with as an application developer. A
    minimalistic timeline would only have one layer, but a more complex
    editing application could use as many as needed.

-   Tracks - Corresponds to the output streams in GStreamer. A typical
    GESTimeline, aimed at a video editing application, would have an
    audio track and a video track. A GESTimeline for an audio editing
    application would only require an audio track. Multiple layers can
    be related to each track.

![Layers and Tracks](layer_track_overview.png)

In order to reduce even more the amount of GStreamer interaction the
application developer has to deal with, a convenience GstPipeline has
been made available specifically for Timelines :
[GESPipeline](#GESPipeline).
