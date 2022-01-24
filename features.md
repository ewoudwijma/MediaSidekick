[__Home__](https://ewoudwijma.github.io/MediaSidekick/)
[__About__](about.md)
[__Download__](download.md)
[__Features__](features.md)
[__Roadmap__](roadmap.md)
[__Edit__](https://github.com/ewoudwijma/MediaSidekick/edit/gh-pages/features.md)

# Features

…Work in progress…

# Video files

## Trim

Files can be trimmed based on the clips made for this file. For each clip, a new file will be created with the same filename plus the offset in milliseconds from the original file added to the name…

The new file contains the specific clip plus some seconds before and after. Also all metadata from the source file will be copied (incl location, camera and author data)

picture...

Trimming is useful for:

- Initially, to remove waste from recordings e.g. camera put on before action or put off after action.
- Finally, to archive only the source of clips made.

# Smart rename

Files can be renamed based on metadata. For example:

- Create date and time (mandatory)
- GPS coordinates (configurable)
- Make/model/author (configurable)
- …
  
By this, your folders will contain meaningful names instead of GOPRO234.mp4 etc.

Related files will also be renamed:

- Trimmed files
- Clips (.srt)

# Clips

Clips are parts of a video file defined by an in point and an out point.

The following information can be captured for each clip:

- Rate
- Tag
- Order
- Video editor presets
- Clip information is stored in a separate files per video file with extension .srt.

# Filters

Filter clips on ratings and tags

# Properties

- Compare and update
- Dates
- Location
- Camera
- Author data

# Timeline

- Auto generated
- Shows all filtered clips in a timeline
- Add transition

# Export

- Export lossless, encoded or video editor project file (currently Shotcut and Adobe Premiere supported) from timeline
- Lossless and encoded: metadata from source will be copied.
- Filter clips on ratings and tags
