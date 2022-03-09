# DGIndex

<small>This repository is a mirror of the original source shared by Donald Graft at https://rationalqm.us/dgmpgdec/dgmpgdec.html</small>

DGIndex, part of the DGMPGDec package, is primarily designed to create an index of an MPEG video stream, containing the location of each frame in the input stream, and some additional information about each frame.

*Note: Donald Graft did not release source code for v2.0.0.6, so I cannot fork the changes here. Maybe one day it's source code will drop.*

This index, or project file, is used by the companion [Avisynth](http://www.avisynth.org) filter DGDecode to provide frame-accurate serving of the video via an Avisynth script. DGIndex is able to decode and index most MPEG1/2 streams including elementary streams, program streams, VOBs, VCDs, SVCDs, PVA files, and transport streams. Additional features include: video demuxing (m1v/m2v), audio demuxing (ac3, dts, aac, mpa, and lpcm), optimized iDCTs, luminance filtering, cropping, and more.

DGIndex is an evolution of Chia-chen Kuo's DVD2AVI and is baselined off DVD2AVI v1.77.3.
The evolved version was renamed to DGIndex to avoid naming confusions, to reflect the significant divergence of functionality, and to clearly link it with neuron2's companion tools in DGMPGDec. Neuron2 wants to take great pains to acknowledge the origins of DGIndex as described in the Credits section below.

You can get the latest binaries and source code of DGMPGDec at [https://rationalqm.us/dgmpgdec/dgmpgdec.html](https://rationalqm.us/dgmpgdec/dgmpgdec.html).

DGIndex is free software distributed under the terms of the GNU GPL v2 license. You must agree to the terms of the license before using the program or its source code. Please see the [License](#License) section for details.

This document is a reference manual for DGIndex. Please refer to the accompanying documents for quick start information, frequently asked questions, and guidance on actual usage of DGIndex in typical scenarios.

## Table of Contents

| Header                                                                                         | Sections                                                         |
| ---------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- |
| [File Menu](#file-menu)                                                                        | Open                                                             |
|                                                                                                | Load Project                                                     |
|                                                                                                | Close                                                            |
|                                                                                                | Save Project                                                     |
|                                                                                                | Save Project and Demux Video                                     |
|                                                                                                | Demux Audio-Only Stream                                          |
|                                                                                                | Save BMP                                                         |
|                                                                                                | Preview, Play, Stop, Pause/Resume                                |
|                                                                                                | Exit                                                             |
| [Stream Menu](#stream-menu)                                                                    | Detect PIDs: PAT/PMT                                             |
|                                                                                                | Detect PIDs: PSIP                                                |
|                                                                                                | Detect PIDs: Raw                                                 |
|                                                                                                | Set PIDs                                                         |
| [Video Menu](#video-menu)                                                                      | iDCT Algorithm                                                   |
|                                                                                                | Field Operation                                                  |
|                                                                                                | YUV->RGB                                                         |
|                                                                                                | Luminance Filter                                                 |
|                                                                                                | Cropping Filter                                                  |
|                                                                                                | HD Display                                                       |
|                                                                                                | Copy Frame to Clipboard                                          |
| [Audio Menu](#audio-menu)                                                                      | Output Method                                                    |
|                                                                                                | Select Track(s)                                                  |
|                                                                                                | Dolby Digital Decode                                             |
|                                                                                                | 48 -> 44.1KHz                                                    |
|                                                                                                | Normalization                                                    |
| [Options Menu](#options-menu)                                                                  | Process Priority                                                 |
|                                                                                                | Loop Playback                                                    |
|                                                                                                | Playback Speed                                                   |
|                                                                                                | AVS Template                                                     |
|                                                                                                | Use Full Paths                                                   |
|                                                                                                | Force Open GOPs in D2V File                                      |
|                                                                                                | Log Quant Matrices                                               |
|                                                                                                | Log Timestamps                                                   |
|                                                                                                | Force Fusion-Style Audio                                         |
|                                                                                                | Enable Info Log                                                  |
| [Tools Menu](#tools-menu)                                                                      | Analyze Sync                                                     |
|                                                                                                | Fix D2V                                                          |
|                                                                                                | Parse D2V                                                        |
| [Help Menu](#help-menu)                                                                        | SIMD, VFAPI Plug-In                                              |
|                                                                                                | Quick Start Guide, DGIndex User Manual, and DGDecode User Manual |
| [Display Window](#display-window)                                                              | Navigation                                                       |
|                                                                                                | Selection                                                        |
|                                                                                                | Splash Screen                                                    |
| [Information Panel](#information-panel)                                                        | Video Section                                                    |
|                                                                                                | Audio Section                                                    |
|                                                                                                | Status Section                                                   |
| [Appendix A: D2V File Structure](#appendix-a-d2v-file-structure)                               |                                                                  |
| [Appendix B: Legacy Command-Line Interface](#appendix-b-legacy-command-line-interface)         |                                                                  |
| [Appendix C: Unix-Style Command-Line Interface](#appendix-c-unix-style-command-line-interface) |                                                                  |
| [Appendix D: "AC3 5.1" versus "AC3 3/2"](#appendix-d-ac3-5-1-versus-ac3-3-2)                   |                                                                  |
| [Appendix E: Auxiliary INI File Options](#appendix-e-auxiliary-ini-file-options)               |                                                                  |
| [Credits](#credits)                                                                            |                                                                  |
| [License](#license)                                                                            |                                                                  |

## File Menu

### Open

Use this option to open one or more source files. If you open more than one file, their contents will be concatenated in the order in which the files are listed in the File List dialog. Demultiplexing or frame serving will produce a single output stream that is a concatenation of the source files. When you open multiple files, all the files must be of the same type; e.g., you cannot open MPEG1 and MPEG2 files at the same time, you cannot open program streams and transport streams at the same time, etc.

Clicking on File\Open causes one of two things to happen:

1. If no files have yet been selected, the Open dialog will appear.  
   Multiple files may be selected by holding the SHIFT or CTRL keys during file selection.  
   Click "Open" to proceed to the File List dialog.
2. If one or more files have already been selected, the File List dialog will open.

Files can also be loaded by dragging-and-dropping them onto the DGIndex window. This causes the File List to be opened with the list containing the drag-and-dropped files.

When the File List dialog opens with newly selected files, it sorts them (using an intelligent multifield sorting algorithm) and lists the selected files. In rare cases, the order produced may not be what the user requires. This can be corrected by using the list editing buttons. The following buttons are available for list management:

- **Add** - Open the file selection dialog.
- **Up** - Move the selected item up one position in the list.
- **Down** - Move the selected item down one position in the list.
- **Del** - Remove the selected item from the list.
- **Del All** - Remove all items from the list.

When the list is arranged satisfactorily, click "OK" and the contents of the File List will be loaded into DGIndex as a continuous sequence.

Sometimes due to cutting or editing, video streams may start with several frames that cannot be decoded properly (because their reference frames are missing). When this is the case for the first file in your file list, replaces the first few frames with copies of the first good decodable frame.

### Close

Use this option to close all open input files.

### Load Project

Use this option to open existing DGIndex (d2v) project files.

When a project file is loaded, DGIndex opens the project's files in the correct order and restores the following settings from the project file:

- IDCT Algorithm
- Field Operation
- YUV -> RGB Scale
- Luminance Filter
- Clipping
- Timeline selection points

Options not listed above, such as Audio options, are not restored. They remain at the values they had prior to the loading of the project file.

### Save Project

Use this option to generate a DGIndex project file (d2v file). The project file is used by DGDecode (or DGVfapi) to frame-serve the video.

DGIndex builds a project file containing the paths to the source files, the video settings, and MPEG indexing information. For more information on the structure of d2v files, refer to Appendix A.

When the project is saved, the timeline selection points are honored, that is, the served video and the demultiplexed audio will contain only the portions of the input streams selected by the timeline [selection](#Selection) points.

Generation of the project file can take several minutes depending on the size of the project and the speed of your computer.

Note that the source file names and path locations cannot be changed after a project file is generated. If you need to change their names or move them, you will either have to regenerate the project file using Save Project after moving them, or you will have to edit the paths given in the top of the d2v file.

### Save Project and Demux Video

Use this option to generate a DGIndex project file (d2v file) as described in the section "Save Project" above, and at the same time demultiplex the video to a separate MPEG elementary stream file (m1v or m2v file). Elementary stream files are sometimes needed for use with other applications (such as DGPulldown and DVD authoring programs). DGIndex provides this functionality as a convenient tool for the sophisticated user; elementary streams are not required for frame serving via DGDecode (or DGVfapi).

The elementary stream file will be generated in the same directory as the project (d2v) file, and its name will be created by concatenating the base name of the project file with ".demuxed.m1v" for MPEG1 streams, or ".demuxed.m2v" for MPEG2 streams. For example, if the project file is named "movie.d2v" and it contains MPEG2 video, the elementary stream file will be named "movie.demuxed.m2v".

Note that demultiplexing always generates a single elementary stream file containing all the video of the project, even if the project file refers to multiple source files.

### Demux Audio-Only Stream

Some streams contain only audio, and so it is not possible to do Save Project. To allow for demuxing the audio from such streams, this option is provided. It may also be used on streams that contain video (though it makes no sense to do that instead of Save Project), but the selection range is ignored and the entire stream is demuxed, and no delay value is reported for the audio.

Also note that you cannot decode audio to WAV using this option. You must use Save Project.

The demuxed audio file will be generated in the same directory as the source stream file(s), and its name will be created by using the base name of the first loaded source file.

Note that demultiplexing always generates a single elementary stream file containing all the audio of the project, even if the project file refers to multiple source files.

### Save BMP

Use this option to save the current frame on display in the DGIndex window to a Windows BMP file. When this option is selected, a file dialog will open that allows you to specify the desired name and location for the BMP file.

### Preview, Play, Stop, Pause/Resume

Use these options to control video playback in the display window. Keyboard hotkeys are given in parentheses.

- Preview (F5) - Plays from the beginning to the end of the selected timeline range.
- Play (F6) - Plays from the current position on the timeline.
- Stop (ESC) - Stops a running Play or Preview.
- Pause/Resume (SPACE) - Pauses/resumes a running Play or Preview.

Play and Preview play only video and not audio.

The video playback speed is controlled by the playback speed control options available via the [Options/Playback](#PlaybackSpeed) Speed menu.

The Play command does not decode frames prior to the current position to obtain reference frames, so you may see macroblocking on the first few frames of a Play operation if you jump around on the timeline. Note that this is purely a display quirk in DGIndex, it never occurs in the frame-served video or in demultiplexed video files.

Play and Preview automatically pop up the [Information Panel](#InformationPanel).

### Exit

Use this option to close all open files and terminate DGIndex.

[back to top][top]

## Stream Menu

### Detect PIDs: PAT/PMT

DGIndex can open transport streams. Transport streams can contain multiple programs, each of which contains audio and video streams. The Stream menu allows the user to list the programs and to select the desired program to be decoded. The stream menu is disabled if the input stream is not a transport stream. Transport streams periodically send data tables called the PAT/PMT tables that specify the programs and tags (PIDs) for the associated audio and video streams. By setting the PIDs for the desired audio and video streams, you thereby select the desired program.

Use the "Detect PIDs: PAT/PMT" option to list the programs and their associated streams, and to select the desired streams to be decoded. DGIndex will parse the first input file for the PAT/PMT tables and, if found, will display the programs and their PIDs in a list box. If PAT/PMT tables cannot be found, an error message will be shown instead, in which case the list box should be closed and the "Detect PIDs: Raw" option used instead.

To select the desired video stream, highlight it by selecting it with a mouse click. Then click on the "Set Video" button. To select the desired audio stream, highlight it by selecting it with a mouse click. Then click on the "Set Audio" button. To select the desired PCR stream, highlight it by selecting it with a mouse click. Then click on the "Set PCR" button.When you have selected your PIDs, click on the "Done" button to close the list box. The video should now be displayed in the DGIndex window.
Note that unless you use the Log Timestamps function, the PCR PID is not used and can be ignored.

The PIDs are stored in the DGIndex.ini file and so will be retained across DGIndex sessions.

Note that selecting an audio stream as video, or vice versa, will produce undefined operation and may crash DGIndex. For that reason, both PIDs come up by default to 0x02, which is a reserved value. However, because the DGIndex.ini file retains PIDs from a previous session, you may run into this. If you get strange behavior or crashing, then manually set the audio and video PIDs to 0x02 using the "Set PIDs" option before loading your input file(s), and then detect and set your PIDs normally.

### Detect PIDs: PSIP

This option is siliar to "Detect PIDs: PAT/PMT", but instead of reading PAT/PMT tables, it reads PSIP tables. PSIP tables are found in terrestrial ATSC transmissions. Future versions will enhance the PSIP support to add display of the program guide, etc.

### Detect PIDs: Raw

Rarely, a transport stream may lack PAT/PMT tables. In such cases, the PIDs of the audio and video streams can be displayed and selected using the "Detect PIDs: Raw" option. Because raw PID detection does not show which audio and video streams go together, some trial and error may be required to find which audio and video PIDs belong together (commonly encountered pairs of associated video/audio PIDs are 11/14, 21/24, and 31/34). For that reason "Detect PIDs: PAT/PMT" should always be tried first.

The operation of the "Detect PIDs: Raw" list box is the same as described above for the "Detect PIDs: PAT/PMT" list box.

### Set PIDs

Use this option to manually specify PIDs using hexadecimal notation, or to query the selected PIDs. If you use this dialog to specify your PIDs, DGIndex will not detect PIDs and will simply use the specified PIDs to select the audio and video streams.

Note that selecting an audio stream as video, or vice versa, will produce undefined operation and may crash DGIndex. If you get strange behavior or crashing, then manually set the audio and video PIDs to 0x02 using the "Set PIDs" option before loading your input file(s), and then detect and set your PIDs normally.

[back to top][top]

## Video Menu

### iDCT Algorithm

Specify which iDCT algorithm will be used by DGIndex and DGDecode.

- **32-bit MMX** (1 in INI file, D2V file and CLI)
- **32-bit SSE MMX** (2)
- **32-bit SSE2 MMX** (3)
- **64-bit Floating Point** (4)
- **IEEE-1180 Reference** (5)
- **Skal SSE MMX** (6)
- **Simple MMX** (7)

Algorithms that are not supported by your processor are automatically deleted from the menu, so you may not see all of the above options.

The iDCT algorithm that you should use depends primarily on what CPU you have, and to a lesser degree on the desired iDCT accuracy. All of the available options are IEEE-1180 compliant. For more information on iDCTs please see the DGDecode manual.

Quality: **IEEE-1180 Reference** > **64-bit Floating Point** > Remaining iDCTs.

Speed: **SSE2MMX** is the fastest. **IEEE-1180 Reference** is the slowest.

Note that the selected iDCT algorithm is placed in the D2V file. DGDecode can be configured to use this setting (default) or it can be overridden via the "idct" parameter.

This setting is also stored in the DGIndex.ini file and therefore is saved across DGIndex sessions. If you edit the DGIndex.ini file, or the D2V file, to specify an iDCT algorithm that is not supported by your processor, then DGIndex and DGDecode will demote your setting to the next available setting that is supported by your processor.

### Field Operation

MPEG2 video provides a syntax element (RFF flags) that allows a single field of any frame to be repeated automatically during display ("pulled down"). The extra fields do not occupy space in the MPEG2 stream but are simply created by copying when the stream is decoded and displayed. This facility is most often used to display film rate clips (23.976 fps) at the NTSC rate (29.97 fps), a process called 3:2 pulldown. In practice, however, pulldown in various different patterns is commonly encountered. It can be used, for example, to display 25 fps PAL at the NTSC rate. You can learn about telecining and pulldown at [this web site (archive.org link)](https://web.archive.org/web/20080808061034/http://www.dvdfile.com/news/special_report/production_a_z/3_2_pulldown.htm).
Understanding the theory and practice required to properly set this option for a given application is nontrivial. Here we can only define the operations and give some general guidance. There is ample additional material available in existing guides.

The Field Operation setting allows the user to specify how the pulldown (RFF) flags are to be handled.

- **Honor Pulldown Flags** - The pulldown flags (if any) are obeyed and the fields are repeated. This means that the frame-served video will appear exactly as it is intended to appear on the final display device. Therefore, if you have a 3:2 pulled-down clip, you'll get the standard repeating pattern of 3 progressive frames followed by 2 interlaced frames. If you have PAL or MPEG1, you'll just get the encoded pictures with no repeated fields (because PAL and MPEG1 have no RFF flags to honor). The frame rate will always be the same as the display frame rate of the source.
- **Ignore Pulldown Flags** - The pulldown flags are ignored. This allows one to obtain the raw encoded MPEG pictures, with no repeated fields. However, because repeated fields intended for display are ignored and not displayed, the resulting frame rate may differ from the source frame rate. It may even vary throughout the clip, due to irregular patterns of pulldown flags. If the pulldown is irregular, use of this option will cause the audio-video sync to change at different parts of the clip, and most likely sync will not be acceptable. This option is mostly intended for power users, who would use it as a diagnostic aid for inspecting the encoded MPEG pictures. Although this option ignores the flags, they are still stored in the D2V file although DGDecode will also ignore them.
- **Force Film** - This option is intended for the special case of film content encoded at 23.976 fps but pulled down for display at 29.97 fps, i.e., 3:2 pulldown. This option will restore the 23.976 fps film rate while keeping audio and video in sync throughout the clip. This option works by ignoring pulldown flags and inserting/removing frames to maintain a constant and sync'ed output stream. Do not use this option on non-3:2 pulled-down material, and, therefore, never use it on PAL or MPEG1. Also do not use this option on streams containing frame repeats.

The Field Operation is perhaps the most important option to understand when using DGIndex. Most users should first preview the project using the **Honor Pulldown Flags** option. If the clip is not PAL or MPEG1, and the Video Type field in the Information window indicates Film 95% or higher, it is likely that the clip can be treated as 3:2 pulldown material, and so the **Force Film** option should be selected for generation of the D2V file.

If the percentage is much lower than 95% or the Video Type indicates a Video percentage, then you should leave this option set to **Honor Pulldown Flags** for generation of the D2V file. If the result is combed (because the source is interlaced, hybrid interlaced/3:2, field blended, etc.), you may want to apply a deinterlacer or inverse telecine (IVTC) filter in your Avisynth script.

If the film percentage is low but still mostly film, you can try using **Force Film** and see what you get. You may find some stray combed frames in the output. You can fix those by post-processing with FieldDeinterlace(full=false). If the result is satisfactory to you, then fine. If not, there are more advanced ways to handle hybrid, hard-telecined, field-blended, and other pathological video. These more advanced methods almost always require the video to be served with the **Honor Pulldown Flags** option.

When assessing the FILM percentage using a preview operation, it is important to use a lengthy portion of the film itself, because introductions and credits are sometimes rendered differently than the main movie content.

Almost invariably, if you know the source is not mostly 3:2 pulled-down NTSC, you will want to select **Honor Pulldown Flags** and then post-process as required. Keep in mind that if there are no pulldown flags, there is nothing to honor, so this option just delivers the encoded frames.

The **Ignore Pulldown Flags** option should be necessary only for advanced users who want to see just the exact coded pictures in the MPEG stream.

You should also read [this article](http://www.doom9.org/ivtc-tut.htm) for additional information on using **Force Film** vs. external IVTC.

Note that the **Honor Pulldown Flags** option was formerly called **None**, and the **Ignore Pulldown Flags** option was formerly called **Raw Encoded Frames**.

### YUV->RGB

When you use DGVfapi to frame serve, the video is always converted to RGB. This option specifies how the colorspace conversion to RGB is to be performed.

- **PC scale** - Map output RGB to full range: YUV [16, 235(Y)/240(UV)] -> RGB [0, 255]
- **TV scale** - Map output RGB to clipped range: YUV [16, 235(Y)/240(UV)] -> RGB [16, 235]

This option affects conversion to RGB, which occurs in only two places in DGMPGDec: 1) in the display window of DGIndex, and 2) the video served by DGVfapi. Generally, unless you have a good reason to clip the RGB, you should select the **PC scale** option.

Two well-known situations for using TV scale are as follows:

1. You are using DGVfapi to serve video to TMPGEnc and you have the **Output YUV Data As Basic YCbCr Not CCIR601** option checked (it's _unchecked_ by default).
2. You are using DGVfapi to serve video to CCE with the **Luminance Level 0 To 255** option.

### HD Display

This option allows you to choose how you would like to display large (HD) videos. When a video is opened whose width is greater than 800 or whose height is greater than 600, the way the video is displayed is controlled by this option; otherwise, this option is ignored. The display choices are:

- **Full Sized** - The video is shown at full size (but limited by the desktop size).
- **Shrink by Half** - The video is shrunk in half so that it is all visible.
- **Top Left** - The top left quadrant of the video is displayed.
- **Top Right** - The top right quadrant of the video is displayed.
- **Bottom Left** - The bottom left quadrant of the video is displayed.
- **Bottom Right** - The bottom right quadrant of the video is displayed.

This option is useful for inspecting the interlace structure of HD videos. If the display is shrunk by half, then the interlacing cannot be seen. To assess the interlacing, you can choose a quadrant to view and then single step through the video.

This option setting is stored in the INI file and so is retained across DGIndex invocations.

### Luminance Filter

Use this option to perform luminance adjustments.

Use the checkbox in the upper left corner to Enable or Disable this filter. Position the Gamma and Offset controls until desired results are achieved. DGIndex stores the luminance adjustment settings in the D2V file and DGDecode reads them and applies the same luminance adjustment to the served video, so that what you see in the DGIndex window is what you get in the served video.

Gamma performs a non-linear adjustment, while offset just adds or subtracts luminance for all the pixels. It is beyond the scope of this manual to describe the concept of gamma. Google is your friend.

This function is unrelated to, and should not be confused with, DGDecode's bundled Avisynth filter called LumaYV12().

### Cropping Filter

Use this option to crop your video.

Use the checkbox in the upper left corner to Enable or Disable this filter. Position the Left, Right, Top, and Bottom controls until desired results are achieved. DGIndex stores the cropping settings in the D2V file and DGDecode reads them and applies the same cropping to the served video, so that what you see in the DGIndex window is what you get in the served video.

Note that for backward compatibility, the cropping settings are referred to as "Clipping" in the D2V file.

### Copy Frame to Clipboard

Use this option to copy the frame on display to the clipboard. It may then be pasted into suitable graphics applications.

[back to top][top]

## Audio Menu

### Output Method

Use this option to control audio processing.

- **Disable** - Disables audio processing.
- **Demux Tracks** - Demultiplex (demux) the selected track(s).
- **Demux All Tracks** - Demultiplex all the available audio tracks.
- **Decode AC3 Track to WAV** - Decode the selected AC3 track to WAV.

DGIndex can demultiplex the following types of audio: LPCM, AC3, MPA, DTS, AAC.
Only AC3 tracks can be decoded to WAV. Use an external audio utility, such as BeSweet, to decode other audio types.

Demuxed audio files will be named after the project name and additional parameters of the audio format.
For example: "MyProject T80 3_2ch 448Kbps DELAY -248ms.ac3".

When LPCM is demuxed from a program stream, the raw PCM is repackaged as a standard WAV file. When LPCM is demuxed from a transport stream, usually an M2TS (Blu-Ray) stream, the raw PCM is stored in the output file. You can convert this to a standard wave file using the popular SOX audio processor tool, which you can get here:

http://sourceforge.net/projects/sox/

A typical command line would be as follows (although you need to check your audio parameters in the Info dialog to choose the correct SOX options):

    sox -B -r48000 -t .raw -c 6 -2 -s "Blu-ray PCM PID 1101 DELAY 0ms.pcm" "out.wav"

A future version of DGIndex may integrate the WAV packaging functionality for transport streams.

An important number is contained in the filename of the demultiplexed audio file: the delay correction for the audio relative to the video. In the example above, the audio delay correction is reported as -248 milliseconds. That means that the audio should be advanced by 248 milliseconds. The number -248 could thus be entered directly into VirtualDub's "Audio Skew Correction" edit box to achieve audio/video synchronization. Note that when AC3 tracks are decoded to WAV, their filenames do not contain a delay correction value because the correction is applied during decoding.

### Select Track(s)

Use this option to select the audio track(s) to be operated on by **Demux Tracks** and **Decode AC3 Track to WAV**.

If **Demux Tracks** is selected, one or more tracks can be selected. If **Decode AC3 Track to WAV** is selected, only one track should be selected. If **Demux All Tracks** is selected, this option is ignored.

The desired tracks are selected by specifying a comma-separated list of audio id's. The audio id's are obtained from the Audio group box of the Information dialog after running a preview operation. For example, the Info dialog may show this:

    80: AC3 2/0 384
    c0: MPA L2 2ch 48 256
    c1: MPA L2 mono 48 128

If you wanted to demux only the MPA tracks, you would type this in the Select Track(s) edit box:

    c0,c1

Note that for transport and PVA streams, only one audio stream can be demultiplexed at a time, so audio id = 0 is always used. In fact, in these cases, it makes little sense to specify the track individually, and **Demux All Tracks** is more convenient. The same thing applies for decoding AC3 to WAV; the selected track ID should always be 0 for transport or PVA streams.

### Dolby Ditital Decode

This submenu applies only when the audio output method is selected as **Decode AC3 Track to WAV**. It affects how the AC3 stream is converted to WAV format.

**Dynamic Range Control (DRC)** Tim Carroll writes as follows about DRC:

    "Different listening environments present a wide variety of dynamic range requirements. Obviously a quiet movie does not fit well into a noisy environment, nor does a loud movie fit into a quiet environment. The classic solution has been to dramatically reduce the dynamic range of the audio prior to transmission, then the audio level can be set once by each viewer to suit his or her environment. The unfortunate side effect is that audio impact is lost. Explosions, dialogue, and background grasshoppers are all reproduced at the same loudness and the program can sound, well, flat -- to say the least.

    Happily, there is another solution. Dolby Digital provides a Dynamic Range Control (DRC) system that is rather unique. Based on a pre-selected DRC profile, the Dolby Digital encoders calculate and send DRC metadata along with the original audio signal. The DRC metadata can then be applied to the signal by the decoder to reduce the signal's dynamic range. On many decoders, DRC can optionally be scaled back or even disabled so that the original dynamic range of the audio is delivered.

    This unique consumer-side dynamic range processing allows the kitchen DTV set to have restricted dynamic range so that quiet audio can be heard above background noise, while simultaneously the large DTV set in the family room can have unrestricted dynamic range and can stomp on the background noise (and possibly the neighbors). DRC helps to provide the best possible presentation of program content in virtually any listening environment, regardless of the quality of the equipment, number of channels, or ambient noise level."

DGIndex provides four possible settings for DRC, as described below. Note that some AC3 encodings may not provide DRC metadata, and so the setting may appear to have no effect.

- **Off** - Disables DRC processing and thereby delivers the full dynamic range as encoded in the audio stream.
- **Light** - Reduces the dynamic range by a small amount.
- **Normal** - Reduces the dynamic range by a moderate amount.
- **Heavy** - Reduces the dynamic range by a large amount.

Dolby Surround Downmix AC3 supports up to 6 channels but when decoding to WAV only 2 channels (left and right stereo) can be output, so a process of downmixing is applied. This option selects which of two possible methods for downmixing is to be used. The first is used when this option is unchecked: a simple stereo downmix (called a left-only/right-only, or Lo/Ro, downmix) suitable for playback on a two-channel stereo system. The second is used when this option is checked: a surround-compatible downmix (left-total/right-total, or Lt/Rt, downmix) of the multichannel source program suitable for downstream Dolby Surround Pro Logic or other matrix decoding, usually by a Pro Logic home theater decoder.

Tim Carroll writes:

    "The only difference between the downmix modes is how the surround channels are handled. The Lt/Rt downmix sums the surround channels, attenuates them 3 dB (i.e., multiply by 0.707) and adds them out-of-phase to the left channel and in-phase to the right channel. This allows a Pro Logic home theater decoder to produce L/C/R/S channels when connected to a stereo set-top box or DTV receiver. Conversely, the Lo/Ro downmix adds the right and left surround channels discretely to the left and right speaker channels. This preserves the stereo separation for stereo-only monitoring and allows for generation of a mono-compatible signal. What about the LFE channel? Well, it is simply discarded."

If you are wondering why the LFE channel is discarded, please refer to [Appendix D](#AppendixD).

So, check this option if you plan to play the decoded audio on a Pro Logic home theater decoder. Leave it unchecked if you plan to play the decoded audio on a standard left/right stereo component. Note that leaving this option unchecked does not mean that downmixing will not be performed. It just specifies the kind of downmixing that is done, as described above.

**Pre-Scale Decision** is a more refined method of normalization than the one originally implemented in DGIndex (see [Normalization](#AudioNorm) below). It is designed to reduce quantization errors of Dolby Digital decoding. To use it, load your input file(s) and select audio method **Decode AC3 Track to WAV**. Then set your normalization amount using the Normalization dialog slider (don't enable Normalization with its check box, just set the percentage with the slider). Finally, check the **Pre-Scale Decision** menu option. Immediately, a pass will be made over the input files and a gain factor (pre-scale ratio) will be determined and stored for later use. This gain factor is such that when the audio is amplified by this factor, the highest sound peak will be set to the percentage of the maximum possible audio level set by the normalization selected. So, if your normalization is set to 50%, the loudest sound will be set to 50% of maximum.

The calculated gain is shown in the Info box of the Information Window and is applied to later decoding (when Save Project is performed), as long as it is left checked. When the prescale decision is complete, the Normalization option is automatically disabled if it was enabled and should not be subsequently enabled, because the calculated pre-scale ratio will be used.

Note that if you change the selected tracks using the Select Track(s) dialog, then the Pre-Scale Decision item is automatically unchecked.

### 48 -> 44.1KHz

This submenu applies only when the audio output method is selected as **Decode AC3 Track to WAV**. It affects how the AC3 stream is converted to WAV format.

When this option is set to a value other than "Off", the audio will be downconverted from 48KHz to 44.1KHz. This conversion can take a lot of time. The quality settings allow you to trade off quality against conversion time. Use "Low" for lowest quality but fastest conversion. Use "UltraHigh" for best quality but slowest conversion. "Mid" and "High" provide intermediate tradeoffs.

### Normalization

This submenu applies only when the audio output method is selected as **Decode AC3 Track to WAV**. It affects how the AC3 stream is converted to WAV format.

This option normalizes the audio but is not as refined as the [Pre-Scale Decision](#PreScale) method described above. To use it, set the desired normalization level and then check the check box in the corner of the dialog box. Nothing happens immediately. Subsequently, when a Save Project operation is performed, two passes will be made. The first creates the project in the normal way; the second opens the created audio file and normalizes it. Normalization means that the audio is scaled such that the loudest sound in the input files is adjusted to be the percentage of the maximum volume given by the Normalization slider value.

The [Pre-Scale Decision](#PreScale) method is preferred for two reasons: 1) quantization errors are reduced, and 2) after a first pass is made, all subsequent Save Project operations require only one pass, while with the Normalization method of this section two passes are always made every time Save Project is performed.

[back to top][top]

## Options Menu

### Process Priority

Specify what CPU usage priority Windows will assign to DGIndex.

- **High**
- **Normal**
- **Low**

Use this option if you are running multiple applications at once, and want to prioritize (or de-prioritize) DGIndex for CPU usage.

### Loop Playback

When this option is selected and playback during a Preview or Play operation reaches the end of the timeline, playback will automatically start again at the beginning of the timeline.

### Playback Speed

Specify the playback speed for play and preview operations.

- **Single Step** - Step forward one frame at a time (see description below).
- **Super Slow** - 5 frames/sec.
- **Slow** - 10 frames/sec.
- **Normal** - The frame rate of the stream.
- **Fast** - Double the frame rate of the stream (two times Normal).
- **Maximum** - The fastest possible rate that your computer can perform.

The single step mode is controlled by the right arrow button (>) on the timeline. Please be aware that due to orphaned B frames, the first few frames of a playback operation, including a single-stepping operation, may show artifacts. They will not show up in the frame-served video, so don't worry! To ameliorate this effect, start your single-stepping operation a little bit ahead of the frames you desire to view. Also note that during a single-stepping operation, you cannot navigate on the timeline. To single-step from a new location, first exit the play operation, then reposition on the timeline in the normal way, and restart the play operation. Finally, single-stepping does not give you per-frame granularity for cuts and timeline positioning; it just allows you to pace the playback so that individual frames can be examined.

### AVS Template

DGIndex can automatically generate an Avisynth script for serving your project. It requires you to specify a template file, such as the following one:

    loadplugin("...\DGDecode.dll")
    loadplugin("...\Decomb.dll")
    mpeg2source("__vid__",cpu=6)
    fielddeinterlace()

This is any normal AVS script, except that the D2V file name is replaced with `__vid__` (that's two underscores before "vid" and two after). When DGIndex creates the AVS script, it replaces `__vid__` with the correct D2V file name.

To select and enable a template, use the Options/AVS Template pulldown menu item to set your template file as the active template file. After performing a Save Project operation, DGIndex uses this file as a template and inserts the right file name whenever it sees `__vid__`.

You may also use the `__aud__` specifier to generate the audio file name. Note that if you are generating more than one audio file, this specifier will refer to the first audio file that is opened. Therefore, to be sure to get the desired audio file, process just one audio stream.

You may also use the `__del__` specifier to generate the delay value for use in `DelayAudio(__del__)`.

DGIndex creates the AVS script file only if it does not already exist. The script file is created in the same directory as the d2v file.

The AVS Template dialog box displays the current active template file. If no file is shown, then a template is not being used. The dialog box provides three possible actions:

1. **Don't Use Template**: Choose this action if you do not want DGIndex to automatically create AVS scripts.
2. **Change Template File**: Choose this action to select your template file and make it the active template for AVS generation. This action pops up a file dialog which you use to select your template file.
3. **Keep Current Template**: Choose this action if you want to keep the current template file.

This option setting is stored in the INI file and so is retained across DGIndex invocations. When an INI file is not present, the AVS Template path defaults to a file named "template.avs" and located in the same directory as that of the executed DGIndex.exe.

### Use Full Paths

D2V files and AVS files created via an AVS template contain references to audio and video files. This option controls whether they should be stored as relative paths or full paths. Historically, DGIndex has always used full paths. When this option is disabled, the D2V file and generated AVS scripts will suppress the full path information and provide just the relative paths; everything is relative to the location of the D2V file.

The option to continue using full paths was retained to provide backward compatibility to third-party applications.

This option setting is stored in the INI file and so is retained across DGIndex invocations.

### Force Open GOPs in D2V File

Rarely, a stream may be encoded incorrectly, such that the GOPs are marked as closed when they are really open. This can cause you to see blocking artifacts when doing random access in the frame-served video. To correct these cases, the **Force Open GOPs in D2V File** option is provided. When it is checked, GOPs are all marked as open in the D2V file, and the actual open/closed GOP indications in the stream are ignored. This cures the macroblocking but makes random access slower. Because the problem is so rare, and the cure reduces performance for normal streams, the cure is not applied automatically, but is presented as an option.

In practice, if you see blocking artifacts when doing random access while frame-serving, enable the **Force Open GOPs in D2V File** option and see if the blocking is fixed.

This option setting is stored in the INI file and so is retained across DGIndex invocations.

### Log Quant Matrices

When MPEG2 files are created, configurable data structures called quantization matrices are used to control how the video is compressed. Often, different matrices work better for different kinds of video. Power users often like to know what matrices have been used to encode their streams. This option allows you to dump all the quantization matrices found in the stream.

To dump the matrices, enable this option and then perform a Save Project operation. A text file will be created that contains the quantization matrices. For example, if your d2v file is named "movie.d2v", the dump file will be named "movie.quants.txt".

Note that only stream events that change the active quant matrices are logged. So, for example, if you have the same matrices sent with each sequence header, only the first will be logged (because it changes the default matrices).

Also note that the frame numbering given in the dump file refers to the encoded frame numbers and not the display frame numbers. These two are different when pulldown is present. In that case, you can use a DGIndex project with "Ignore Pulldown Flags" to serve the video such that the display frame numbers correspond to the encoded frame numbering.

Note that this setting is not stored in the DGIndex INI file.

### Log Timestamps

This option allows you to dump a trace of the audio and video PTS/DTS timestamps encountered when a Save Project operation is performed. For transport streams, the PCRs encountered on the configured PCR PID are also logged, and for program streams the SCRs are logged. The dump also includes traces for each decoded picture and GOP starts in order to expose the alignment of the timestamps with the pictures.

To dump the timestamps, enable this option and then perform a Save Project operation. A text file will be created that contains the timestamp trace. For example, if your d2v file is named "movie.d2v", the dump file will be named "movie.timestamps.txt".

The timestamps are presented in native syntax units, and in brackets divided by 90 (so the time unit is milliseconds). E.g.:

    PCR 237636675 [2640407ms]

Note that this option setting is not stored in the DGIndex INI file.

### Force Fusion-Style Audio

Program streams that include audio on private stream 1 usually follow the DVD convention of including an audio substream number and associated header information. This is only a convention, however, and is not required by MPEG syntax. Some streams do not follow this convention and, instead, they do not provide an audio substream number and associated header information. The most common case is seen in program streams (MPG files) created by the Dvico Fusion capture cards. Therefore, we can say that these streams carry "Fusion-style" audio. Such streams need to be parsed differently, and this is controlled by this option. Note that Fusion transport streams (TP files) are handled correctly and this option setting is ignored for such streams.

Ideally, Fusion-style audio should be detected automatically and this option should not be required. However, some technical problems stand in the way and so the initial support for these streams requires setting this option.

Enable this option if you want to force audio parsing in Fusion style. Disable it for conventional DVD-like parsing.

This option setting is stored in the INI file and so is retained across DGIndex invocations.

### Enable Info Log

When the **Enable Info Log** option is checked and the Information Panel is closed, either manually or automatically by DGIndex, a log file is created that contains the information displayed in the Information Panel at the time of closure. The file name used is that of the first input file with the extension changed to "log". The log file is created in the same directory as the first input file. If this directory is not writable, then the log file is not created. This feature may be used by the command line interface (CLI) to obtain information about the input file. Refer to Appendix B or C for details.

This option setting is stored in the INI file and so is retained across DGIndex invocations.

[back to top][top]

## Tools Menu

### Analyze Sync

This tool allows the user to create a log of the audio delay values that would be generated if the project were to be started at all the I frames of the stream. This is useful for diagnosing problems with the audio delay reported by DGIndex. For example, a VOB is processed and the delay is reported as 66792ms. That is obviously spurious, but why? If we execute this tool, we see the following in the generated log:

    -----
    Delay Analysis Output (Audio ID 80)

    Decode picture: temporal reference 2[I]
    delay = 66792
    Decode picture: temporal reference 2[I]
    delay = -231
    Decode picture: temporal reference 2[I]
    delay = -219
    [etc.]
    -----

Clearly, there is something fishy right at the beginning of the stream (in this case it was a jump in the SCR). To solve this, we can start our project one GOP in from the beginning of the stream, i.e., hit the > button once and then the [ button before executing Save Project.

When you select the Analyze Sync tool, a popup will appear allowing you to select the audio ID to analyze, and then a file open dialog will appear. You should specify a timestamps dump file that you have previously created using Options/Log Timestamps.

Note that for transport and PVA streams, only one audio stream can be selected at a time, so audio id = 0 is always used. Don't confuse it with the audio PID.

Note that for a displayed delay value to be usable and correct, you must start your project at the corresponding I frame.

### Fix D2V

Sometimes streams are encountered that contain mixtures of TFF and BFF material. In MPEG2, these field order transitions are fixed at display time via the TFF/RFF flags. But we usually want to output a stream with one consistent field order, and some tools, such as Telecide and (Leak)KernelDeint, require a fixed field order. Indeed, AVI does not carry a field order property!

This tool adjusts the TFF/RFF flags so that a stream with a single field order is output. This affects only streams containing field order transitions, of course. When field order correction is applied using this tool, the uncorrected D2V file is renamed so that it ends with the extension ".bad". The corrected version is output as the usual D2V file.

This tool should be used only when you have reason to believe that field order transitions are present in the stream. For some pathological streams, however, you may find that the field order correction causes problems that you don't encounter when using the ".bad" D2V file. Therefore, it is always advisable to treat the correction as an advisory and to test both the good and bad D2V files.

Note that field order correction should not be applied to streams containing frame repeat RFFs.

### Parse D2V

This tool is intended for power users. When selected, you are prompted to open a d2v file. After the d2v file is selected, a text file is created that contains an analysis of the contents of the d2v file, and the system's editor is opened on that text file. If your d2v file is named "movie.d2v", the text file will be named "movie.parse.txt". The contents of the text file will be readily understood by power users.

[back to top][top]

## Help Menu

### SIMD, VFAPI Plug-In

These are read-only indicators displaying information about the status of DGIndex.

- **Detected SIMD** - Indicates which SIMD optimizations were detected as available for this CPU.
- **VFAPI Plug-In** - Indicates whether the VFAPI Plug-In (DGVfapi.vfp) has been detected and is usable for VFAPI frameserving, i.e., it is present in the DGIndex program directory.

The **VFAPI Plug-In** indication must be present; otherwise, VFAPI frame serving will not be operative.

### Quick Start Guide, DGIndex User Manual, and DGDecode User Manual

These options open the respective documents in your browser. To work, the files "QuickStart.html", "DGIndexManual.html", "DGDecodeManual.html" that are included in the DGMPGDec package must be in the same directory as the "DGIndex.exe" file. Do not rename these files.

## Display Window

### Navigation

Preview the video file(s) and select the start/end points for the project/demuxing.

Use the **navigation slider**, and these two buttons for navigation:

    < - Seek to previous I frame.
    > - Seek to next I frame.

Currently, the display window displays only I frames in navigation mode (because they are possible selection points for cutting the video), which usually appear about every 12 to 15 frames. This makes the video appear to jump when stepping, instead of moving smoothly. Of course, during play, preview, or frameserving, all frames are shown.

The video may in some cases appear stretched/squashed in the display window because DGIndex displays the video as is it stored in the MPEG stream, which may not be the same as the final display size. For an explanation, read Doom9's article on [Aspect Ratios](http://www.doom9.org/aspectratios.htm).

When a video is displayed that has a frame size that will not fit on a 1024x768 desktop, the video is reduced in size by a factor of two. Thus, high-definition (HD) videos can be previewed without requiring scrolling.

### Selection

Starting and ending points for the project may also be optionally specified.

    [ button or HOME key - Mark selection start point.
    ] button or END key - Mark selection end point.

When doing Save Project, Save Project and Demux Video, or Preview, the operation will be applied only to the portion of the timeline between the selected start and end points.

By default, the entire timeline is automatically selected. Selection start/end point can be changed, but only one set of start/end point may be specified for a project (i.e.: multiple start/end points are not yet supported).

### Splash Screen

When no file is loaded DGIndex can display a splash screen. To enable this feature, place an appropriately sized windows BMP file called "dgindex.bmp" in the same directory as DGIndex.exe.

[back to top][top]

## Information Panel

### Introduction

The Information Panel displays important statistics about the loaded video and audio stream(s). Information displayed in this panel is often essential for correctly processing DGIndex projects, especially when frameserving into third-party applications. The Information Panel is displayed when a project is saved or when using the Play/Preview commands. Note that when doing a Play/Preview operation, the information will apply only to the section actually played or previewed.

When the **Enable Info Log** option is checked and the Information Panel is closed, either manually or automatically by DGIndex, a log file is created that contains the information displayed in the Information Panel at the time of closure. The file name used is that of the first input file with the extension changed to "log". The log file is created in the same directory as the first input file. If this directory is not writable, then the log file is not created. This feature may be used by the command line interface (CLI) to obtain information about the input file. Refer to Appendix B or C for details.

### Video Section

**Stream Type** - Displays the stream type: Transport \[188/204\], MPEG1/MPEG2 Program, PVA, or Elementary.

**Profile** - For MPEG2, this field displays the MPEG profile used to encode the currently loaded video. For MPEG1, it displays "\[MPEG1\]".

**Frame Size** - Displays the size of the frame in the currently displayed video. Note that this reports the values of the horizontal_size and vertical_size fields in the MPEG syntax.

**Display Size** - Displays the display size of the frame in the currently displayed video. Note that this reports the values of the display_horizontal_size and display_vertical_size fields of the MPEG syntax. If the sequence_display_extension that carries these values is not present, then this field is blank, and the display size is the same as the frame size.

**Aspect Ratio** - Displays the aspect ratio specified in the MPEG stream. In brackets, the raw value of the aspect_ratio_information field is given. By appropriately interpreting the Frame Size, Display Size, and Aspect Ratio according to the MPEG2 video specification, the sample aspect ratio (SAR) and display aspect ratio (DAR) can be inferred.

**Frame Rate** - Displays the playback frame rate of the currently loaded video.

**Video Type** - If the stream does not contain pulldown flags, this field will display "PAL" if the frame rate is 25.0 fps; otherwise, it will display "NTSC". If pulldown flags are present, this field will show a percentage "Film" or a percentage "Video" value. These two always add up to 100%, so only the greater of the two is shown.

**Sequence** - Displays **_Frame only_** if the _progressive_sequence_ flag is set. Displays **_Field/Frame_** if the _progressive_sequence_ flag is not set.

**Frame Struct** - Displays **_Frame_** or **_Field_** depending on whether the frame is encoded as a frame picture or field pictures.

**Frame Type** - Displays **_Interlaced_** or **_Progressive_** depending on the value of the MPEG2 _progressive_frame_ flag. Please be aware that this merely describes how the frame was encoded; it says nothing about whether the content of the frame is progressive or interlaced. It is common for progressive video to be encoded as interlaced, and vice versa.

**Coding Type** - Displays the coding type of the decoded picture (I, P, or B). Note that if a frame is displayed with pulldown, the frame may have been assembled from two pictures with different coding types. In this case, the coding type of the latest decoded picture that makes up the frame is displayed. To view the coding types without this complication, set **Ignore Pulldown Flags** in the Video menu.

**Colorimetry** - Displays the colorimetry scheme used by the stream. Note that if the stream does not declare the colorimetry, then ITU-R BT.709* is reported for HD video, and ITU-R BT.470-2* is reported for SD video. The \* character indicates that the stream did not declare the colorimetry.

**Field Order** - Displays **_Top_** or **_Bottom_** depending on the field order of the currently loaded video.

**Coded #** - Displays the total number of MPEG pictures decoded during the Save Project, Play, or Preview operation.

**Playback #** - Displays the total number of MPEG pictures displayed during the Save Project, Play, or Preview operation. This number can be greater than the Coded # if pulldown is present and the **Honor Pulldown Flags** option is selected.

**Frame Rpts** - Displays the total number of extra frames displayed due to RFF frame repeats encountered during the Save Project, Play, or Preview operation. If the **Ignore Pulldown Flags** option is selected, this field is not incremented.

**Field Rpts** - Displays the total number of extra fields displayed due to RFF field repeats encountered during the Save Project, Play, or Preview operation. If the **Ignore Pulldown Flags** option is selected, this field is not incremented.

**Vob/Cell ID** - Displays the Vob and Cell ID for the current position in the video stream. It is valid only for vob files.

**Bitrate** - Displays a windowed video bitrate averaged over the last 30 displayed frames.

**Bitrate (Avg)** - Displays the video bitrate averaged over all the displayed frames.

**Bitrate (Max)** - Displays the maximum video bitrate encountered over all the displayed frames.

### Audio Section

**Tracks list box** - Displays the detected audio id's and corresponding track information.

**Timestamp** - Displays the timestamp for the current position in the audio stream.

### Status Section

**Elapsed** - Displays the time elapsed since starting the current Save Project, Play, or Preview operation.

**Remain** - Displays the time remaining for completion of the current Save Project, Play, or Preview operation.

**FPS** - Displays the frame rate achieved by DGIndex's play/preview process. Note that this may not match the video frame rate of the stream, because the user can configure the playback speed.

**Info** - Displays video and audio error information.

[back to top][top]

## APPENDIX A: D2V File Structure

### Introduction

This Appendix documents the syntax and semantics of the D2V file format. Normal users need not concern themselves with this information, as it is intended for developers and power users.

There are three sections to all D2V files: 1) the header section, 2) the settings section, and 3) the data section. These sections are described below.

### Header

Following is a typical header section:

    DGIndexProjectFile16
    3
    G:\RETURN_OF_THE_KING\VIDEO_TS\VTS_01_1.VOB
    G:\RETURN_OF_THE_KING\VIDEO_TS\VTS_01_2.VOB
    G:\RETURN_OF_THE_KING\VIDEO_TS\VTS_01_3.VOB

The first line always begins with the string `DGIndexProjectFile` immediately followed by a two-digit D2V version number. This number is checked by DGDecode and must match the version expected. This mechanism thus enforces the requirement that DGIndex and DGDecode must be used in matched pairs. If DGDecode rejects your D2V file as "obsolete", be sure that you are using the correct version of DGDecode. Sometimes one forgets that one has an older version in the Avisynth plugins folder, or one forgets to upgrade DGDecode when upgrading DGIndex.

The second line gives the number of video files included in the D2V project file. Each video file must be referenced on the subsequent line(s). The header section must then terminate with a blank line.

Each source file reference consists of either a filename, if the **Use Full Paths** option is disabled; or the full absolute path of the referenced file, if the **Use Full Paths** option is enabled. If the **Use Full Paths** option is enabled and the referenced files are moved after creation of the D2V file, you must either edit the paths to be correct, or you must regenerate your D2V file.

In the remainder of the D2V file, the files are referenced with a number corresponding to their place in this list. The list is zero-based, that is, the first file is numbered **0**.

### Settings

Following is a typical settings section:

    Stream_Type=1
    MPEG_Type=2
    iDCT_Algorithm=5
    YUVRGB_Scale=1
    Luminance_Filter=0,0
    Clipping=0,0,0,0
    Aspect_Ratio=4:3
    Picture_Size=720x480
    Field_Operation=0
    Frame_Rate=29970 (30000/1001)
    Location=0,0,6,17CF3

Note that the Clipping setting is called "Cropping" in the DGIndex menus. It was left as "Clipping" in the D2V file to avoid having to increment the D2V file format version number and cause incompatibilities with existing third-party applications.

The second section of a D2V file defines values that are determined by, or within, DGIndex. Some are determined by DGIndex as it analyzes the MPEG stream, others reflect user-specified choices. All of these fields must be explicitly defined, even if the associated value is zero. The settings section also must terminate with a blank line.

If the stream type is set to 2 (transport stream) then two additional lines are appended after the Stream_Type line:

    MPEG2_Transport_PID=880,881,880
    Transport_Packet_Size=188

Settings determined automatically by DGIndex are: Stream_Type, Transport_Packet_Size, MPEG_Type, Aspect_Ratio, Picture_Size, and Frame_Rate (although the user-defined Field_Operation does have an impact on the Frame_Rate value).

The remaining settings are user-defined within DGIndex: MPEG2_Transport_PID,iDCT_Algorithm, YUVRGB_Scale, Luminance_Filter, Clipping, Field_Operation, and Location.

The following table describes each of these parameters and the range of values possible for each.

<table cellspacing="2px" cellpadding="10%" border="1px">
    <tbody>
        <tr>
            <th align="center">D2V Field
            </th>
            <th align="center">Possible Values
            </th>
            <th align="center">Definition/Comments
            </th>
        </tr>
        <tr>
            <td>Stream_Type
            </td>
            <td>(<b>0</b>) Elementary Stream<br> (
                <b>1</b>) Program Stream<br> (
                <b>2</b>) Transport Stream<br> (
                <b>3</b>) PVA Stream
            </td>
            <td>Defines the type of MPEG stream.<br> Determined by DGIndex. All possible values listed.
            </td>
        </tr>
        <tr>
            <td>MPEG2_Transport_PID
            </td>
            <td>(<b>Video PID, Audio PID, PCR PID</b>)
            </td>
            <td>Selects the video/audio PIDs to be decoded.<br> User-specified. Appears only for Stream_Type 2.
            </td>
        </tr>
        <tr>
            <td>Transport_Packet_Size
            </td>
            <td>(<b>188, 204</b>)
            </td>
            <td>Specifies the size in bytes of the transport packets.<br> Determined by DGIndex. Appears only for Stream_Type 2.
            </td>
        </tr>
        <tr>
            <td>MPEG_Type
            </td>
            <td>(<b>1</b>) MPEG-1<br> (
                <b>2</b>) MPEG-2
            </td>
            <td>Defines the type of MPEG stream.<br> Determined by DGIndex. All possible values listed.
            </td>
        </tr>
        <tr>
            <td>iDCT_Algorithm
            </td>
            <td> (<b>1</b>) 32-bit MMX<br> (
                <b>2</b>) 32-bit SSEMMX<br> (
                <b>3</b>) 32-bit SSE2MMX<br> (
                <b>4</b>) 64-bit Floating Point<br> (
                <b>5</b>) 64-bit IEEE-1180 Reference<br> (
                <b>6</b>) 32-bit SSEMMX (Skal)<br> (
                <b>7</b>) 32-bit Simple MMX (XviD)
            </td>
            <td>Defines the iDCT DGDecode will use to decode this video.<br> User-specified. All possible values listed.
            </td>
        </tr>
        <tr>
            <td>YUVRGB_Scale
            </td>
            <td>(<b>0</b>) TV Scale<br> (
                <b>1</b>) PC Scale
            </td>
            <td>Defines the range DGDecode will use if RGB conversion is requested.<br> User-specified. All possible values listed.
            </td>
        </tr>
        <tr>
            <td>Luminance_Filter
            </td>
            <td>(<b>Gamma, Offset</b>)
            </td>
            <td>Defines values for DGIndex's Luminance_Filter.<br> User-specified.
                <br> Both Gamma and Offset have a range of: +/- 256.<br> This parameter is unrelated to DGDecode's <i>LumaFilter</i>.
            </td>
        </tr>
        <tr>
            <td>Clipping
            </td>
            <td>(<b>ClipLeft, ClipRight, ClipTop, ClipBottom</b>)
            </td>
            <td>Defines values for Clipping (aka: Cropping) lines of video.<br> User-specified.
                <br> Arbitrary values possible within the resolution of the video.
            </td>
        </tr>
        <tr>
            <td>Aspect_Ratio
            </td>
            <td>MPEG2:<br> (
                <b>1:1</b>)<br> (
                <b>4:3</b>)<br> (
                <b>16:9</b>)<br> (
                <b>2.21:1</b>)<br> MPEG1:
                <br> (
                <b>1:1</b>)<br> (
                <b>0.6735</b>)<br> (
                <b>16:9,625</b>)<br> (
                <b>0.7615</b>)<br> (
                <b>0.8055</b>)<br> (
                <b>16:9,525</b>)<br> (
                <b>0.8935</b>)<br> (
                <b>4:3,625</b>)<br> (
                <b>0.9815</b>)<br> (
                <b>1.0255</b>)<br> (
                <b>1.0695</b>)<br> (
                <b>4:3,525</b>)<br> (
                <b>1.575</b>)<br> (
                <b>1.2015</b>)
            </td>
            <td>Defines the Aspect Ratio of the video specified in the MPEG stream.<br> Determined by DGIndex. All possible values listed.
            </td>
        </tr>
        <tr>
            <td>Picture_Size
            </td>
            <td>(<b>widthxheight</b>)
            </td>
            <td>Defines the size of the video <u>after</u> clipping has been applied.<br> Determined by DGIndex.<br> Arbitrary values possible within the size of the loaded video.
            </td>
        </tr>
        <tr>
            <td>Field_Operation
            </td>
            <td>(<b>0</b>) Honor Pulldown Flags<br> (
                <b>1</b>) Force Film<br> (
                <b>2</b>) Ignore Pulldown Flags
            </td>
            <td>Defines values for Field Operation.<br> User-specified. All possible values listed.
            </td>
        </tr>
        <tr>
            <td>Frame_Rate
            </td>
            <td>(<b>rate</b>) (<b>num/den</b>)
            </td>
            <td>'rate' defines output framerate * 1000.<br> 'num/den' defines output framerate as numerator/denominator.
            </td>
        </tr>
        <tr>
            <td>Location
            </td>
            <td>(<b>StartFile,StartOffset,EndFile,EndOffset</b>)
            </td>
            <td>Defines start and end points for the video selection range.<br> User-specified.
                <br>
            </td>
        </tr>
    </tbody>
</table>

### Data

Following is a typical data section (real ones will typically be much longer):

    d00 6 0 0 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 384035 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 768034 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 1152609 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 1537064 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 1922483 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 2304616 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 2689746 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 3075088 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 3459597 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 3844802 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 4227419 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 4613561 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0
    d00 6 0 4996887 0 0 0 b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0 ff


    FINISHED 0.00% FILM

The data section of a D2V file contains the indexing of the MPEG video stream. There is one line for each I picture in the MPEG stream. Each line describes that I picture as well as all the following pictures up to, but not including, the next I picture.

Each I picture line consists of eight fields that must be explicitly defined, even if the associated value is zero. The layout for each line is as follows:

    info matrix file position skip vob cell flags ...

Therefore, the second line in the typical data section given above would break down as follows:

<table cellspacing="2px" cellpadding="10%" border="1px">
    <tbody>
        <tr>
            <th align="center">info</th>
            <th align="center">matrix</th>
            <th align="center">file</th>
            <th align="center">position</th>
            <th align="center">skip</th>
            <th align="center">vob</th>
            <th align="center">cell</th>
            <th align="center">flags</th>
        </tr>
        <tr>
            <td><code>d00</code></td>
            <td><code>6</code></td>
            <td><code>0</code></td>
            <td><code>384035</code></td>
            <td><code>0</code></td>
            <td><code>0</code></td>
            <td><code>0</code></td>
            <td><code>b0 b0 90 b0 b0 a0 b0 b0 a0 b0 b0 a0 b0 b0 a0</code></td>
        </tr>
    </tbody>
</table>

The following section describes each field of an I picture line and its range of possible values.

The info field is a 12-bit hex number bit-mapped as follows:

<table cellspacing="2px" cellpadding="10%" border="1px">
    <tbody>
        <tr>
            <th align="center">Bit Position</th>
            <th align="center">Definition/Comments</th>
        </tr>
        <tr>
            <td>bit 11</td>
            <td>1 (Always 1 to signal start of data line)</td>
        </tr>
        <tr>
            <td>bit 10</td>
            <td>1 (This line's pictures are part of a closed GOP)<br> 0 (Open GOP)</td>
        </tr>
        <tr>
            <td>bit 9</td>
            <td>1 (This line's pictures are part of a progressive sequence)<br> 0 (Otherwise)</td>
        </tr>
        <tr>
            <td>bit 8</td>
            <td>1 (This I picture starts a new GOP)<br> 0 (Otherwise)</td>
        </tr>
        <tr>
            <td>bits 7-0</td>
            <td>0 (Reserved)</td>
        </tr>
    </tbody>
</table>

The matrix field displays the currently applicable matrix_coefficients value (colorimetry).

The file field references the file (as defined in the beginning of the D2V file) containing this line's pictures.

The position field is a decimal number that points to the byte position in the source file at which decoding of the indexed I picture should begin. For non-packetized streams, if a sequence header is present it is indexed, otherwise the picture header is indexed. Packetized streams are treated the same way except that the packet header start code preceding the sequence header or picture header is indexed. We prefer to index sequence headers because they may carry new quant matrices, but they are not mandatory at each GOP, so we fall back to the picture header.

The skip field defines how many I frames to skip to reach the one actually needed. This is required because an indexed unit (especially packs) can contain multiple I frames. If nothing were done, we would have multiple D2V file index lines with the same position and DGDecode would not have enough information to implement random access properly. So DGIndex inserts this field telling how many I frames to skip starting from the index position to reach the required I frame. Usually this field is 0, but if you have a stream with large packs, you may see non-zero values in this field.

The vob and cell fields display the current position within the DVD navigation stream (private stream 2). The VOB and CELL numbers are taken from there. Not applicable for non-DVD sources.

The sequence of Flags fields contains per-picture data corresponding to the pictures in **_display_** order, that is, the Flags fields are re-ordered into display order. End Of Stream is signaled with a Flags field value of `ff`. Each per-picture Flags field is an 8-bit hex number bit-mapped as follows:

<table cellspacing="2px" cellpadding="10%" border="1px">
    <tbody>
        <tr>
            <th align="center">Bit Position</th>
            <th align="center">Definition/Comments</th>
        </tr>
        <tr>
            <td>bit 7</td>
            <td>
                <b>1</b> (Picture is decodable without the previous GOP)<br>
                <b>0</b> (Otherwise)
            </td>
        </tr>
        <tr>
            <td>bit 6</td>
            <td>
                <i>Progressive_Frame</i> Flag (See notes below)<br>
                <b>0</b> (Interlaced)<br>
                <b>1</b> (Progressive)
            </td>
        </tr>
        <tr>
            <td>bits 5-4</td>
            <td>
                <i>Picture_Coding_Type</i> Flag<br>
                <b>00</b> (Reserved)<br>
                <b>01</b> (I-Frame)<br>
                <b>10</b> (P-Frame)<br>
                <b>11</b> (B-Frame)
            </td>
        </tr>
        <tr>
            <td>bits 3-2</td>
            <td>
                <b>00</b> (Reserved)<br>
                <b>01</b> (Reserved)<br>
                <b>10</b> (Reserved)<br>
                <b>11</b> (Reserved)
            </td>
        </tr>
        <tr>
            <td>bit 1</td>
            <td><i>TFF</i> Flag</td>
        </tr>
        <tr>
            <td>bit 0</td>
            <td><i>RFF</i> Flag</td>
        </tr>
    </tbody>
</table>

**Notes on _Progressive_Frame_ Flag**: This specifies that the picture is part of a frame (either a frame picture making up the entire frame, or a field picture making up one of the fields of the frame) whose fields are sampled at one moment. A RFF flag may be present however, such that the **_displayed_** frame after pulldown is interlaced.

[back to top][top]

## APPENDIX B: Legacy Command-Line Interface

### Parameters

If the command line contains an = sign then legacy parsing is used. If it does not, then Unix-style parsing is used.

DGIndex can also be executed via a command-line interface. This feature is intended for use in automated scripts and batch files, and for use by third-party applications. Following is the syntax and semantics for the legacy command line interface. Note that options not included are controlled by DGIndex defaults and the DGIndex INI file.

Note that if you have a transport stream and do not specify any PIDs, then DGIndex will attempt to determine automatically as in GUI (no-CLI) operation.

<table cellspacing="2px" cellpadding="10%" border="1px">
    <tbody>
        <tr>
            <th align="center">Parameter</th>
            <th align="center">Command-Line Flag</th>
            <th align="center">Definition/Comments</th>
        </tr>
        <tr>
            <td>Set Delimiter</td>
            <td id="tt">-SD=delimiter character</td>
            <td>Set the delimiter character.<br> For some applications, the [ and ] characters are used in filenames. In such cases, you can use the -SD option to set a different delimiter character. The -SD option must precede any other options that use a delimiter.
            </td>
        </tr>
        <tr>
            <td>Input Files</td>
            <td id="tt">-IF=[file1,file2,file3,...]</td>
            <td>Load specific files.<br> If a specified file cannot be found, it is ignored.<br> Brackets Required!
            </td>
        </tr>
        <tr>
            <td>Auto-Input Files
            </td>
            <td id="tt">-AIF=[file1]
            </td>
            <td>Autoload files incrementally.<br> If a specified file cannot be found, it is ignored.<br> Brackets Required!
            </td>
        </tr>
        <tr>
            <td>Batch Files
            </td>
            <td id="tt">-BF=[filelist]
            </td>
            <td>
                Load a batch of files.<br>
                The filelist should be a plain text file listing one file per line with no extra white space.<br>
                If a specified file cannot be found, it is ignored.<br>
                Brackets Required!<br>
                <hr>
                HINT: The following DOS command automatically generates a filelist:<br>
                <code>DIR *.VOB /B &gt; FILELIST</code><br>
            </td>
        </tr>
        <tr>
            <td>Project Range
            </td>
            <td id="tt">-RG=f1/off1/f2/off2
            </td>
            <td>Project range, specified as in the D2V file syntax<br> file numbers are zero-based, offsets are in hex (one LBA=2048 bytes)
            </td>
        </tr>
        <tr>
            <td>Audio PID
            </td>
            <td id="tt">-AP=audioPID
            </td>
            <td>Transport stream audio PID, hex notation. (e.g., -AP=14)<br> See note in text above.
            </td>
        </tr>
        <tr>
            <td>Video PID
            </td>
            <td id="tt">-VP=videoPID
            </td>
            <td>Transport stream video PID, hex notation. (e.g., -VP=11)<br> See note in text above.
            </td>
        </tr>
        <tr>
            <td>PCR PID
            </td>
            <td id="tt">-PP=pcrPID
            </td>
            <td>Transport stream PCR PID, hex notation. (e.g., -PP=11)<br> See note in text above.
            </td>
        </tr>
        <tr>
            <td>iDCT Algorithm
            </td>
            <td id="tt">-IA=algorithm
            </td>
            <td>1=MMX, 2=SSEMMX, 3=SSE2MMX, 4=64-bit Floating Point, 5=IEEE-1180 Reference, 6=Skal MMX, 7=Simple MMX
            </td>
        </tr>
        <tr>
            <td>Field Operation
            </td>
            <td id="tt">-FO=field operation
            </td>
            <td>0=Honor Pulldown Flags, 1=Force Film, 2=Ignore Pulldown Flags
            </td>
        </tr>
        <tr>
            <td>YUV-&gt;RGB
            </td>
            <td id="tt">-YR=method
            </td>
            <td>1=PC Scale, 2=TV Scale
            </td>
        </tr>
        <tr>
            <td>Track Number
            </td>
            <td id="tt">-TN=track(s)
            </td>
            <td>track#,track#,track#,...<br> Use a comma-separated list of audio_ids. If OM=3 (Decode), only one track must be listed.
            </td>
        </tr>
        <tr>
            <td>Output Method
            </td>
            <td id="tt">-OM=method
            </td>
            <td>0=None, 1=Demux Tracks, 2=Demux All Tracks, 3=Decode AC3 to WAV
            </td>
        </tr>
        <tr>
            <td>Dynamic Range Control
            </td>
            <td id="tt">-DRC=amount
            </td>
            <td>0=Off, 1=Light, 2=Normal, 3=Heavy
            </td>
        </tr>
        <tr>
            <td>Dolby Surround Downmix
            </td>
            <td id="tt">-DSD=on/off
            </td>
            <td>0=Off, 1=On
            </td>
        </tr>
        <tr>
            <td>Downsample Audio (48-&gt;44.1 kHz downsample)
            </td>
            <td id="tt">-DSA=method
            </td>
            <td>0=None, 1=Low, 2=Mid, 3=High, 4=Ultrahigh
            </td>
        </tr>
        <tr>
            <td>Output File
            </td>
            <td id="tt">-OF=[project file]
            </td>
            <td>Do Not Include Extension!<br> Brackets Required!
            </td>
        </tr>
        <tr>
            <td>Output File and Demux
            </td>
            <td id="tt">-OFD=[project file]
            </td>
            <td>Same as -OF but also demux the video.<br> Brackets Required!
            </td>
        </tr>
        <tr>
            <td>AVS Script Generation
            </td>
            <td id="tt">-AT=[AVS template file]
            </td>
            <td>Specifies a template file for AVS script generation. Use -AT=[] to disable AVS script generation.
            </td>
        </tr>
        <tr>
            <td>Exit
            </td>
            <td id="tt">-EXIT
            </td>
            <td>Exits DGIndex when project has been saved.
            </td>
        </tr>
        <tr>
            <td>Minimize
            </td>
            <td id="tt">-MINIMIZE
            </td>
            <td>Starts and runs DGIndex in the minimized state.
            </td>
        </tr>
        <tr>
            <td>Hide
            </td>
            <td id="tt">-HIDE
            </td>
            <td>Starts and runs DGIndex in the hidden state.
            </td>
        </tr>
        <tr>
            <td>Preview
            </td>
            <td id="tt">-PREVIEW
            </td>
            <td>Previews the first 100 frames (used for log generation).
            </td>
        </tr>
    </tbody>
</table>

### Example

This example...

    DGIndex -IA=2 -FO=2 -TN=80,c0 -OM=1 -AIF=[d:\files\vts_02_1.vob] -OF=[d:\MyProject] -exit

will save a project file to "d:\MyProject.d2v", specifying:

    Auto-Input Files = "[d:\files\vts_02_1.vob]"
    Output File = "[d:\MyProject]"
    iDCT = "SSEMMX"
    Field Operation = "Ignore Pulldown Flags"
    Output Method = "Demux"
    Track Numbers (audio id's) = "80 and c0"
    EXIT

Here is an example that uses the ! character:

    DGIndex -FO=2 -TN=80 -OM=1 -SD=! -IF=!D:\vobfiles\vts_02_1.vob! -OF=!d:\MyProject! -exit

Note that if you process the CLI line with a DOS shell (e.g., in a batch file), then the specified delimiter character must not be a DOS special character. If you invoke DGIndex using CLI programmatically, then DOS special characters can be used, because the DOS shell does not get involved.

The -preview option can be used to silently generate a log file that describes the characteristics of the input file:

    DGIndex -AIF=[zdf.vob] -OF=[zdf] -preview -minimize -exit

The log file is generated in the same directory as the input file (must be writable) and will have the same name as the input file but with the extension replaced by "log". The log file generation option must be enabled in the INI file.

[back to top][top]

## APPENDIX C: Unix-Style Command-Line Interface

### Parameters

If the command line contains an = sign then legacy parsing is used. If it does not, then Unix-style parsing is used.

DGIndex can also be executed via a command-line interface. This feature is intended for use in automated scripts and batch files, and for use by third-party applications. Following is the syntax and semantics for the Unix-style command line interface. Note that options not included are controlled by DGIndex defaults and the DGIndex INI file.

Note that if you have a transport stream and do not specify any PIDs, then DGIndex will attempt to determine automatically as in GUI (no CLI) operation.

<table cellspacing="2px" cellpadding="5%" border="1px">
    <tbody>
        <tr>
            <th align="center">Parameter
            </th>
            <th width="250" align="center">Command-Line Flag
            </th>
            <th align="center">Definition/Comments
            </th>
        </tr>
        <tr>
            <td>Input Files
            </td>
            <td id="tt">-i input_file<br> -i "input_file"<br> -i file1 file2 file3<br> -i my,file.vob
            </td>
            <td>Load specified files.<br> The -i option must be the first option!<br> If a specified file cannot be found, it is ignored.<br> Commas in filenames are supported.
            </td>
        </tr>
        <tr>
            <td>Auto-Input Files
            </td>
            <td id="tt">-ai file_1.vob
            </td>
            <td>Autoload files incrementally.<br> The -i option must be the first option!<br> If a specified file cannot be found, it is ignored.<br> Commas in filenames are supported.
            </td>
        </tr>
        <tr>
            <td>Output File
            </td>
            <td id="tt">-o project<br> -o "c:\My Projects\project"
            </td>
            <td>Specify the DGA index file to be written.<br> Do Not Include Extension!
            </td>
        </tr>
        <tr>
            <td>Output File and Demux Video
            </td>
            <td id="tt">-od project<br> -od "c:\My Projects\project"
            </td>
            <td>Specify the D2V index file to be written and enable demuxing of the video.<br> Do Not Include Extension!
            </td>
        </tr>
        <tr>
            <td>Project Range
            </td>
            <td id="tt">-rg 0 0 1 1ff
            </td>
            <td>Project range, specified as in the D2V file syntax<br> file numbers are zero-based, offsets are in hex (one LBA=2048 bytes)
            </td>
        </tr>
        <tr>
            <td>Audio PID
            </td>
            <td id="tt">-ap audioPID
            </td>
            <td>Transport stream audio PID, hex notation (e.g., -ap 14).
            </td>
        </tr>
        <tr>
            <td>Video PID
            </td>
            <td id="tt">-vp videoPID
            </td>
            <td>Transport stream video PID, hex notation (e.g., -vp 11).
            </td>
        </tr>
        <tr>
            <td>PCR PID
            </td>
            <td id="tt">-pp pcrPID
            </td>
            <td>Transport stream PCR PID, hex notation (e.g., -pp 11).
            </td>
        </tr>
        <tr>
            <td>iDCT Algorithm
            </td>
            <td id="tt">-ia algorithm
            </td>
            <td>1=MMX, 2=SSEMMX, 3=SSE2MMX, 4=64-bit Floating Point, 5=IEEE-1180 Reference, 6=Skal MMX, 7=Simple MMX
            </td>
        </tr>
        <tr>
            <td>Field Operation
            </td>
            <td id="tt">-fo field operation
            </td>
            <td>0=Honor Pulldown Flags, 1=Force Film, 2=Ignore Pulldown Flags
            </td>
        </tr>
        <tr>
            <td>YUV-&gt;RGB
            </td>
            <td id="tt">-yr method
            </td>
            <td>1=PC Scale, 2=TV Scale
            </td>
        </tr>
        <tr>
            <td>Audio Tracks
            </td>
            <td id="tt">-tn track(s)
            </td>
            <td>track#,track#,track#,...<br> Use a comma-separated list of audio_ids. If OM=3 (Decode), only one track must be listed.
            </td>
        </tr>
        <tr>
            <td>Output Method
            </td>
            <td id="tt">-om method
            </td>
            <td>0=None, 1=Demux Tracks, 2=Demux All Tracks, 3=Decode AC3 to WAV
            </td>
        </tr>
        <tr>
            <td>Dynamic Range Control
            </td>
            <td id="tt">-drc amount
            </td>
            <td>0=Off, 1=Light, 2=Normal, 3=Heavy
            </td>
        </tr>
        <tr>
            <td>Dolby Surround Downmix
            </td>
            <td id="tt">-dsd on/off
            </td>
            <td>0=Off, 1=On
            </td>
        </tr>
        <tr>
            <td>Downsample Audio (48-&gt;44.1 kHz downsample)
            </td>
            <td id="tt">-dsa method
            </td>
            <td>0=None, 1=Low, 2=Mid, 3=High, 4=Ultrahigh
            </td>
        </tr>
        <tr>
            <td>AVS Script Generation
            </td>
            <td id="tt">-at template_file
            </td>
            <td>Specifies a template file for AVS script generation.<br> Use -at "" to disable AVS script generation.
            </td>
        </tr>
        <tr>
            <td>Exit
            </td>
            <td id="tt">-exit
            </td>
            <td>Exits DGIndex when project has been saved.
            </td>
        </tr>
        <tr>
            <td>Minimize
            </td>
            <td id="tt">-minimize
            </td>
            <td>Starts and runs DGIndex in the minimized state.
            </td>
        </tr>
        <tr>
            <td>Hide
            </td>
            <td id="tt">-hide
            </td>
            <td>Starts and runs DGIndex in the hidden state.
            </td>
        </tr>
        <tr>
            <td>Preview
            </td>
            <td id="tt">-preview
            </td>
            <td>Previews the first 100 frames (used for log generation).
            </td>
        </tr>
    </tbody>
</table>

### Example

    DGIndex dgindex -i "c:\Movies\Transformers.vob" -o "c:\Movies\Transformers" -fo 0 -om 2

The -preview option can be used to silently generate a log file that describes the characteristics of the input file:

    DGIndex -i zdf.vob -o zdf -preview -minimize -exit

The log file is generated in the same directory as the input file (must be writable) and will have the same name as the input file but with the extension replaced by "log". The log file generation option must be enabled in the INI file.

[back to top][top]

## APPENDIX D: "AC3 5.1" versus "AC3 3/2"

DGIndex reports AC3 5.1 audio as "AC3 3/2" and users sometimes wonder why. The 1 in "5.1" refers to the Low Frequency Effects (LFE) channel. When 5.1 audio must be played on one or two speakers, the source channels must be "downmixed".

The AC3 specification says that downmixing of the LFE channel is optional. In practice, it is dangerous, because loudspeakers can be overdriven by the full scale low frequency content of the LFE channel (which is intended for a special speaker called a subwoofer). That's why downmixing of the LFE is almost never implemented. Encoding 5.1 audio such that it can be played properly on one or two speakers is an art that is seriously complicated by downmixing the LFE channel.

So "3/2" means 5.1 without the LFE (maybe 5.0 would be better terminology, but 3/2 is in common use). Since almost no application downmixes the LFE (including DGIndex), DGIndex labels the source as "3/2". The LFE is not downmixed because a) it is complex to implement (needs rolloff), b) it is dangerous to downmix it, and c) the LFE is often avoided anyway by content providers. There are more details in [this article](http://www.ambisonic.net/dvda.html#bass).

[back to top][top]

## APPENDIX E: Auxiliary INI File Options

Some aspects of DGIndex's behavior can be modified via the INI file but not through the GUI, as described below:

- Use_MPA_Extensions=0: The extension used on demultiplexed MPEG audio files will be "mp1", "mp2", or "mp3", depending on the audio layer.
- Use_MPA_Extensions=1: The extension used on demultiplexed MPEG audio files will be "mpa".

- Notify_When_Done=0: No notification is done when a Save Project operation is finished.
- Notify_When_Done=1: The end of a Save Project operation is signaled by bringing the DGIndex window to the foreground.
- Notify_When_Done=2: The end of a Save Project operation is signaled by a beep.
- Notify_When_Done=3: The end of a Save Project operation is signaled by bringing the DGIndex window to the foreground and a beep.

[back to top][top]

## Credits

All documentation in this README.md was originally from https://rationalqm.us/dgmpgdec/DGIndexManual.html  
I have only re-written it in markdown format and fixed a few mistakes and updated a few links.

Alphabetical Order:

- "Cyberia", users manual contents and modernization
- "dvd2svcd", command line interface
- "fccHandler", MPEG decoding fixes/improvements, good advice
- Donald Graft ("neuron2"), frame loss fix, accurate indexing, PVA support, MPEG1 support, and more
- Peter Gubanov, author of the MMX/SSEMMX iDCT
- Chia-chen Kuo ("jackei"), author of DVD2AVI
- Miha Peternel, author of the Floating Point and Reference iDCT
- Dmitry Rozhdestvensky, author of the SSE2 iDCT
- "sh0dan", code optimizations
- "Skal", for his SSEMMX iDCT
- "trbarry", transport parsing, and code optimizations
- "tritical", upsampling, and more

[back to top][top]

## License

This program is distributed under the terms of the GNU Public License. For details please refer to the file COPYING.TXT included in the distribution package

This README.md contents are Copyright (C) 2004-2007 Donald A. Graft, All Rights Reserved.

[back to top][top]

[top]: #what-is-dgindex
