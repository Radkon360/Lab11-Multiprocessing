/*
 * Name:        Konrad Swierczek
 * Assignment:  CPE-2600 Lab 11 - Multiprocessing (mandelmovie)
 * Section:     111
 * File:        mandelmovie.c
 *
 * Description:
 *   Generates a sequence of Mandelbrot images by running the provided
 *   "mandel" program once per frame. Each frame is rendered by its own
 *   child process (fork + execv). At most N children run at the same
 *   time, where N is given on the command line (-p). The scale shrinks
 *   exponentially from frame to frame so the finished movie appears to
 *   "fly" into the Mandelbrot set.
 *
 *   The parent waits for every child to finish before exiting.
 *
 *   Frames are written as mandel0.jpg ... mandel49.jpg (for 50 frames),
 *   which the following command stitches into a movie:
 *       ffmpeg -i mandel%d.jpg mandel.mpg
 *
 * Usage:
 *   ./mandelmovie -p <processes> [options]   (see -h)
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Default configuration (a deep zoom near a spiral region of the set)
#define DEFAULT_PROCS   1
#define DEFAULT_FRAMES  50
#define DEFAULT_XCENTER 0.286932
#define DEFAULT_YCENTER 0.014287
#define DEFAULT_START_SCALE 4.0
#define DEFAULT_END_SCALE   0.0005
#define DEFAULT_WIDTH   1000
#define DEFAULT_HEIGHT  1000
#define DEFAULT_MAXITER 1000

#define MANDEL_PATH "./mandel"
#define BUF_LEN 64

static void show_help(void);
static void launch_frame(int frame, double xcenter, double ycenter,
                         double scale, int width, int height, int max_iter);
static int reap_one_child(void);

int main(int argc, char *argv[])
{
    int procs = DEFAULT_PROCS;
    int frames = DEFAULT_FRAMES;
    double xcenter = DEFAULT_XCENTER;
    double ycenter = DEFAULT_YCENTER;
    double start_scale = DEFAULT_START_SCALE;
    double end_scale = DEFAULT_END_SCALE;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    int max_iter = DEFAULT_MAXITER;
    int opt;

    // Interpret the command line with getopt
    while ((opt = getopt(argc, argv, "p:n:x:y:a:b:W:H:m:h")) != -1) {
        switch (opt) {
        case 'p': procs = atoi(optarg); break;
        case 'n': frames = atoi(optarg); break;
        case 'x': xcenter = atof(optarg); break;
        case 'y': ycenter = atof(optarg); break;
        case 'a': start_scale = atof(optarg); break;
        case 'b': end_scale = atof(optarg); break;
        case 'W': width = atoi(optarg); break;
        case 'H': height = atoi(optarg); break;
        case 'm': max_iter = atoi(optarg); break;
        case 'h':
            show_help();
            return 0;
        default:
            show_help();
            return 1;
        }
    }

    // Validate input
    if (procs < 1 || frames < 1 || width < 1 || height < 1 ||
        max_iter < 1 || start_scale <= 0 || end_scale <= 0) {
        fprintf(stderr, "mandelmovie: invalid argument value\n");
        show_help();
        return 1;
    }

    int running = 0;   // children currently alive
    int failures = 0;  // children that failed

    for (int frame = 0; frame < frames; frame++) {
        // Never allow more than 'procs' children at once:
        // wait for one to finish before starting the next.
        if (running >= procs) {
            failures += reap_one_child();
            running--;
        }

        // Exponential interpolation from start_scale down to end_scale
        double t = (frames > 1) ? (double)frame / (frames - 1) : 0.0;
        double scale = start_scale * pow(end_scale / start_scale, t);

        launch_frame(frame, xcenter, ycenter, scale, width, height, max_iter);
        running++;
    }

    // Parent must wait for all remaining children before exiting
    while (running > 0) {
        failures += reap_one_child();
        running--;
    }

    if (failures > 0) {
        fprintf(stderr, "mandelmovie: %d frame(s) failed\n", failures);
        return 1;
    }

    printf("mandelmovie: finished %d frames using up to %d process(es)\n",
           frames, procs);
    return 0;
}

/*
 * Fork a child that execs mandel to render a single frame.
 * Arguments are built as strings before the fork.
 */
static void launch_frame(int frame, double xcenter, double ycenter,
                         double scale, int width, int height, int max_iter)
{
    char outfile[BUF_LEN], xs[BUF_LEN], ys[BUF_LEN], ss[BUF_LEN];
    char ws[BUF_LEN], hs[BUF_LEN], ms[BUF_LEN];

    snprintf(outfile, BUF_LEN, "mandel%d.jpg", frame);
    snprintf(xs, BUF_LEN, "%.10f", xcenter);
    snprintf(ys, BUF_LEN, "%.10f", ycenter);
    snprintf(ss, BUF_LEN, "%.10f", scale);
    snprintf(ws, BUF_LEN, "%d", width);
    snprintf(hs, BUF_LEN, "%d", height);
    snprintf(ms, BUF_LEN, "%d", max_iter);

    char *args[] = { "mandel", "-x", xs, "-y", ys, "-s", ss,
                     "-W", ws, "-H", hs, "-m", ms, "-o", outfile, NULL };

    fflush(stdout); // avoid duplicated buffered output in the child

    pid_t pid = fork();
    if (pid < 0) {
        perror("mandelmovie: fork");
        exit(1);
    }
    if (pid == 0) {
        // Child: replace this process with mandel
        execv(MANDEL_PATH, args);
        perror("mandelmovie: execv");
        _exit(127);
    }
    // Parent returns immediately and keeps going
}

/*
 * Wait for any one child to finish.
 * Returns 0 on clean exit, 1 if the child failed.
 */
static int reap_one_child(void)
{
    int status;
    pid_t pid;

    do {
        pid = wait(&status);
    } while (pid < 0 && errno == EINTR);

    if (pid < 0) {
        perror("mandelmovie: wait");
        return 1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }
    fprintf(stderr, "mandelmovie: child %d did not exit cleanly\n", (int)pid);
    return 1;
}

static void show_help(void)
{
    printf("Use: mandelmovie -p <processes> [options]\n");
    printf("Where options are:\n");
    printf("-p <num>    Max number of child processes at once. (default=%d)\n",
           DEFAULT_PROCS);
    printf("-n <num>    Number of frames to generate. (default=%d)\n",
           DEFAULT_FRAMES);
    printf("-x <coord>  X coordinate of zoom center. (default=%g)\n",
           DEFAULT_XCENTER);
    printf("-y <coord>  Y coordinate of zoom center. (default=%g)\n",
           DEFAULT_YCENTER);
    printf("-a <scale>  Scale of the first frame. (default=%g)\n",
           DEFAULT_START_SCALE);
    printf("-b <scale>  Scale of the last frame. (default=%g)\n",
           DEFAULT_END_SCALE);
    printf("-W <pixels> Image width. (default=%d)\n", DEFAULT_WIDTH);
    printf("-H <pixels> Image height. (default=%d)\n", DEFAULT_HEIGHT);
    printf("-m <max>    Max iterations per point. (default=%d)\n",
           DEFAULT_MAXITER);
    printf("-h          Show this help text.\n");
    printf("\nExample:\n");
    printf("mandelmovie -p 5\n");
}
