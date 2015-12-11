# Overview of the main GStreamer Editing Services components

## [GESTimeline](GESTimeline.markdown)

The most top-level object encapsulating every other object is the
GESTimeline. It is the central object for any editing project.

A GES Timeline is composed of one or more [layers](GESLayer), and produces data on one or
more [output tracks](GESTrack).

A track represents an output with a specic media type, for example [audio](GESAudioTrack) or [video](GESVideoTrack).

Layers contain clips, and represent the relative priorities of these clips.

## [GESLayer](GESLayer.markdown)

Let's imagine a timeline that contains two layers:

__Timeline, duration : 0 seconds__

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

__Timeline, duration : 10 seconds__

```
+--------------------------------------------------------+
|                                                        |
|         +-------------------------------------------+  |
|         | +=======================================+ |  |
| Layer 1 | |              Clip 1                   | |  |
|(prio=0) | +=======================================+ |  |
|         +-------------------------------------------+  |
|                                                        |
|         +-------------------------------------------+  |
|         | +=======================================+ |  |
| Layer 2 | |              Clip 2                   | |  |
|(prio=1) | +=======================================+ |  |
|         +-------------------------------------------+  |
|                                                        |
+--------------------------------------------------------+
```

In that timeline, Clip 1 is said to have the highest "priority". In the case of
video streams, it means that it will be rendered on top of Clip 2, which in
turn means that if Clip 2 and Clip 1 share the same width and height, Clip
2 will be completely invisible.

In the case of video streams, one can therefore think of the layer priority as
a z-index (where the top layer has a priority of 0).

## [GESTrack](GESTrack.markdown)

Let's continue with that timeline. For now it indeed mixes various layers
together, but it doesn't output anything. That's what Tracks are for.

If we add a [video track](GESVideoTrack) to our timeline, it can be represented
this way:

__Timeline, duration : 10 seconds__

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
If you want to create a timeline with an audio and video outputs, you shoud
use the [ges_timeline_new_audio_video](ges_timeline_new_audio_video) convenience
function which will create the following empty timeline:

```
+------------------- GESTimeline ------------------------+
|                                                        |
|                                                        |--------> Video Track
|                                                        |
|                                                        |
|                                                        |--------> Audio Track
|                                                        |
+--------------------------------------------------------+
```

## [GESPipeline](GESPipeline.markdown)

In order to reduce the amount of GStreamer interaction the application
developer has to deal with, a [convenience GstPipeline](GESPipeline) has been made available
specifically for playing or rendering [timelines](GESTimeline).

## Create and play an audio video timeline with one 'file' [clip](GESUriClip) 

Now let's implement the following example using media files for the clip:
```
+-------------------------------------------------------+
|                                                       |
|        +-------------------------------------------+  |--------> Audio Output Track
|        | +=======================================+ |  |
| Layer  | |              Clip 1                   | |  |
|        | +=======================================+ |  |
|        +-------------------------------------------+  |--------> Video Output Track
|                                                       |
+-------------------------------------------------------+
```

{{ examples/play_timeline_with_one_clip.markdown }}

## [Asset management](GESAsset.markdown)

In order to help you handle the various
[assets](https://en.wikipedia.org/wiki/Digital_asset) (audio, video, images,
...) you have, GES offers an [high level API](GESAsset). This API makes it easy to
discover the content of media files (asynchronously if needed) and represent them
with detailed information. You can then use that abstraction to directly add
clips to the timeline ([extract](ges_asset_extract) the asset) using the convenience
[ges_layer_add_asset](ges_layer_add_asset) method.

## [Project management](GESProject.markdown)

To handle the various assets you have as part of an editing project, GES offers [project](GESProject)
object that represents the relation between the timeline, the various assets and other components of
GES. Project makes it simple to [serialize](ges_project_save) a timeline and the various assets used
in it or [load](ges_project_load) a project from a supported serialisation file format.
