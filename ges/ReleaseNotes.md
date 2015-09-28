* New Non Linear Engine (Nle) replacing Gnonlin, it reuses big parts of Gnonlin but
  the multi threading logic has been completely reworked to fix long term thread safety
  issues. The API has been sensibly reworked which explains why we renamed it.
  Gnonlin is now deprecated and Nle should be used instead. We developed Nle as
  part of the GStreamer Editing Services
* GES now uses the new Audiomixer and Compositor elements by default to do the mixing
