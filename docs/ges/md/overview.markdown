# Overview of the main GStreamer Editing Services components

## [GESTimeline](GESTimeline)

The most top-level object encapsulating every other object is the
GESTimeline. It is the central object for any editing project.

A GES Timeline is composed of one or more [layers](GESLayer), and produces data on one or
more [output tracks](GESTrack).

A track represents an output with a specic media type, for example audio or video.

Layers contain clips, and represent the relative priorities of these clips.

## [GESLayer](GESLayer)

Let's imagine a timeline that contains two layers:

## Timeline, duration : 0 seconds

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

## Timeline, duration : 10 seconds

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

## [GESTrack](GESTrack)

Let's continue with that timeline. For now it indeed mixes various layers
together, but it doesn't output anything. That's what Tracks are for.

If we add a [video track](GESVideoTrack) to our timeline, it can be represented
this way:

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

## [Asset management](GESAsset)

In order to help you handle the various
[assets](https://en.wikipedia.org/wiki/Digital_asset) (audio, video, images,
...) you have, mainly media files (Audio and Video files), GES offers an [high
level API](GESAsset).

## [Project management](GESProject)

For a sensibly more advanced usage of the API we offer a [project](GESProject) object
which gives a coherent view around the various component of GES. It makes it simple
to [serialize](ges_project_save) a timeline and the various assets used in the project.

## [Play](GESPipeline) or [render](GESPipelineFlags) the timeline 

In order to reduce the amount of GStreamer interaction the application
developer has to deal with, a [convenience GstPipeline](GESPipeline) has been made available
specifically for playing or rendering [timelines](GESTimeline).
