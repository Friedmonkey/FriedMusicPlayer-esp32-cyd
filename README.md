usage:


include the library and if you need set the sample size
the default sample size is 44100
```
//#define FRIED_SAMPLE_RATE 16000
#include <FriedMusicPlayer.h>
```

then in your `setup()`
call `setup_audio` with the path to the raw file (export from audacity)

```
void setup() {
  Serial.begin(115200);
  setup_audio("/rickroll.raw");
  set_audio_volume(0.1f);
}

```
you can also change the volume with set_audio_volume

and thats it you dont have to do anything in the loop since setup_audio spawns a new task on the only other availibe core, core 0

to get a raw file install audacity

drag your music in there

right click the track and select slit stereo to mono and delete either of the 2 tracks so you're left with 1 track

make sure the sample size is 44100 or lower

to change the sample do ctrl + a and go to tracks -> resample and select 44100 or something lower

then press file export audio and chose `other uncompressed files` once again make sure to select the right sample size that matches what the other is

for header choose `RAW (header-less)`
and for encoding select Unsigned 8bit PCM

export and put it on the sd card and thats it

