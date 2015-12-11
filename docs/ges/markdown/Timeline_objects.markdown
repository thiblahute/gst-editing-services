# The timeline objects

### [GESTimeline](GESTimeline.markdown)

#### [GESLayer](GESLayer.markdown)

##### [GESUriClip](GESUriClip.markdown)

- [GESVideoUriSource](GESVideoUriSource) - audio part of a clip [GESUriClip](GESUriClip)

- [GESAudioUriSource](GESAudioUriSource) - video part of a [GESUriClip](GESUriClip)

- [GESEffect](GESEffect) - Class representing an effect, to be applied on any [GESClip](GESClip) using [ges_container_add](ges_container_add)

##### [GESTitleClip](GESTitleClip.markdown)

##### [GESTestClip](GESTestClip.markdown)

##### [GESTransitionClip](GESTransitionClip.markdown)

#### [GESTrack](GESTrack.markdown)

##### [GESVideoTrack](GESVideoTrack.markdown)

##### [GESAudioTrack](GESAudioTrack.markdown)

#### [GESGroup](GESGroup.markdown)

### Advanced clips classes

Those objects are to used with much care as the user will
need to understand and handle how priorities work,
you most probably do not need them as you can use
[GESTitleClip](GESTitleClip) and [GESEffect](GESEffect)
instead.

##### [GESTextOverlayClip](GESTextOverlayClip.markdown)

##### [GESEffectClip](GESEffectClip.markdown)
