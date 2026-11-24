# CPE-2600 Lab 11 - Multiprocessing (mandelmovie)

Name: Konrad Swierczek
Section: 111

## Implementation overview

`mandelmovie` is a new program that leaves the provided `mandel` program unchanged. It takes the number of child processes on the command line (`-p`), interpreted with `getopt`. Optional flags (`-n`, `-x`, `-y`, `-a`, `-b`, `-W`, `-H`, `-m`) change the frame count, zoom center, start and end scale, image size and max iterations. Run `./mandelmovie -h` for the full list.

For each of the 50 frames, the parent computes a scale that shrinks exponentially from 4 down to 0.0005, which gives a smooth zoom into the point x=0.286932, y=0.014287. It then calls `fork()`, and the child replaces itself with `mandel` using `execv()` to write `mandel<N>.jpg`. The parent never lets more than `-p` children run at once: when the limit is reached it calls `wait()` for one child to finish before starting the next frame. After the last frame is launched, the parent waits for all remaining children before exiting. Each child works independently, so no pipes are needed.

The frames are stitched into a movie with:

    ffmpeg -i mandel%d.jpg mandel.mpg

## Runtime results

![Runtime vs number of processes](runtime.png)

| Processes | Time (s) |
|-----------|----------|
| 1  |  |
| 2  |  |
| 5  |  |
| 10 |  |
| 20 |  |

## Discussion

<!-- fill in after testing: does more processes always mean a speedup? -->

## Build and run

    make
    ./mandelmovie -p 5
    ./timing.sh
    make movie
