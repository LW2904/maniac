_This branch represents an effort to port maniac to the Linux OS family and is currently not in an usable state._

<h2 align="center">maniac</h2>

<p align="center">
  Simple external cheat for <a href="https://osu.ppy.sh/help/wiki/Game_Modes/osu!mania">osu!mania</a>.<br>
</p>

Please note that maniac is __currently being rewritten__ as it transitions into the 1.0.0 release, this branch reflects the state of the rewrite. As it is now in a usable state, release candidates (`v1.0.0-rcx`) will be provided -- I am grateful for bug reports and feedback.

## Usage

1. Download the latest build from the [releases tab](https://github.com/LW2904/maniac/releases).
2. Start osu, start playing any beatmap for a couple of seconds and then either exit or pause. (Doesn't matter which map or mode, you only have to do this once for every time you start osu.)
3. Open a terminal and run maniac (e.g. by dragging-and-dropping the executable onto `cmd.exe`).

Maniac will now automatically play any beatmap you open in osu.

### Options

Maniac accepts options which have to be passed through the command line when running the executable.

```
$ ./maniac.exe -h

Usage: maniac [options]

Options:
    -h / --help                Show this message and exit.
    -r / --randomization [a,b] Add milliseconds in the range [a,b] to all k
                               ey presses. If only `a` is provided, `b` imp
                               licitly equals `-a`. (default: 0,0, implicit
                               : -5,5)
    -u / --humanization [a]    For every key press, an offset calculated th
                               rough (density at that point * (a / 100)) is
                                added to the time. (default: 0, implicit: 1
                               00)
    -c / --compensation [a]    Add static offset in milliseconds to every a
                               ction to compensate for the time it takes ma
                               niac to send a keypress. (default: -20)
    -m / --mirror-mod          Mirror the keys pressed (i.e.: mirror mod su
                               pport). (default: false, implicit: true)

    Note that all options have both a default and an implicit value. The di
    fference is best illustrated through an example:

    command                       humanization
    $ ./maniac                    0
    $ ./maniac --humanization     100
    $ ./maniac --humanization 50  50
```

## Thanks
 
- n0b453c0d3r on UC and [mrflashstudio](https://github.com/mrflashstudio) for
 providing up to date signatures and offsets
- [Asi Shavit](https://github.com/adishavit) for the [Argh](https://github.com/adishavit/argh) library
- to everyone who reported bugs and provided feedback
