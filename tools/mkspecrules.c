#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spec.h"
#include "util.h"

struct Segment* g_segments;
int g_segmentsCount;

// fish-oot: what the two files below ask of the link, so that nothing outside the audio driver moves in the CPU's
// caches when the driver or the scheduler changes (fishmod/docs/audio-sweep.md: the cost of everything else is
// compared with the build before the audio work, and code or data that has only moved can cost more or less by
// where it now collides).
//
// fishmod/data/audio_pin.txt, lines "SECTION MODULUS PHASE": after the driver's last object
// (src/audio/fishmod_audio_pin.o) the section goes on at the next address that is PHASE past a multiple of MODULUS
// (the instruction cache's 16 KB for code, the data cache's 8 KB for data): where it went on before the audio work.
//
// fishmod/data/sched_text_slots.txt: the object named on its first line (the scheduler) is linked function by
// function in the order listed (it is compiled with -ffunction-sections); "NAME SIZE" is a function that has grown
// since: SIZE bytes are left where it was and the function itself is linked with the audio driver's
// (audio_text_order.txt lists it; any function the file doesn't list goes there too). Built without function
// sections (IDO, the reference build), none of the names match and the object is linked whole, as it always was.
static void fishmod_pin_phase(FILE *fout, const char *fpath, const char *section_name)
{
    FILE *f;
    char line[256];

    if (strstr(fpath, "/src/audio/fishmod_audio_pin.o") == NULL || (f = fopen("fishmod/data/audio_pin.txt", "r")) == NULL)
        return;
    while (fgets(line, sizeof(line), f) != NULL) {
        char sec[64];
        unsigned int mod, phase;

        if (line[0] != '#' && sscanf(line, " %63s %x %x", sec, &mod, &phase) == 3 && strcmp(sec, section_name) == 0) {
            // (only where the pin object has something in the section: it is empty when the driver isn't GCC's)
            fprintf(fout, "        .LfishmodPin_%s = .;\n        %s(.%s*)\n", section_name, fpath, section_name);
            fprintf(fout, "        . = (. > .LfishmodPin_%s) ? ALIGN(. - 0x%X, 0x%X) + 0x%X : .;\n", section_name, phase, mod, phase);
        }
    }
    fclose(f);
}

static char fishmod_slots_path[512];

static int fishmod_slots(FILE *fout, const char *fpath)
{
    FILE *f = fopen("fishmod/data/sched_text_slots.txt", "r");
    char line[256];
    char object[256] = "";
    int first = 1;

    if (f == NULL)
        return 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        char name[256];
        unsigned int size;
        int fields;

        if (line[0] == '#' || (fields = sscanf(line, " %255[A-Za-z0-9_./] %u", name, &size)) < 1)
            continue;
        if (object[0] == '\0') {
            strcpy(object, name);
            if (strstr(fpath, object) == NULL)
                break;
            continue;
        }
        if (fields == 2) {
            fprintf(fout, "        . = (.LfishmodSlotsOn > .LfishmodSlots) ? . + %u : .;\n", size);
        } else {
            if (first)
                fprintf(fout, "        .LfishmodSlots = .;\n");
            fprintf(fout, "        %s(.text.%s)\n", fpath, name);
            if (first)
                fprintf(fout, "        .LfishmodSlotsOn = .;\n");
            first = 0;
        }
    }
    fclose(f);
    if (object[0] == '\0' || strstr(fpath, object) == NULL)
        return 0;
    fprintf(fout, "        %s(.text)\n", fpath);
    strcpy(fishmod_slots_path, fpath);
    return 1;
}

static void write_progbits_section(FILE *fout, struct Segment *seg, const char *section_name, bool with_padding)
{
    fprintf(fout, "    .%s :\n    {\n", section_name);

    // fish-oot: the audio driver's functions in the order fishmod/data/audio_text_order.txt gives (one a line; '#'
    // comments), ahead of the rest of the driver's code. The driver is built with each function in its own section
    // (.text.NAME: GCC's -ffunction-sections, the Makefile's GCC_AUDIO), and the linker takes the listed ones first,
    // where the driver's code starts. The order is worked out from records of what the audio thread executes
    // (fishmod/tools/layout): the instruction cache is 16 KB, direct-mapped, and functions that run in turn must
    // not share its lines. Built otherwise (IDO), the lines match nothing.
    FILE *order = NULL;
    int ordered = 0;
    int text = strcmp(section_name, "text") == 0;

    if (text) {
        order = fopen("fishmod/data/audio_text_order.txt", "r");
        fishmod_slots_path[0] = '\0';
    }
    for (int i = 0; i < seg->includesCount; i++) {
        if (order != NULL && !ordered && strstr(seg->includes[i].fpath, "/src/audio/internal/") != NULL) {
            char line[256];

            while (fgets(line, sizeof(line), order) != NULL) {
                char name[256];
                unsigned int size;
                int fields = sscanf(line, " %255[A-Za-z0-9_.*] %x", name, &size);

                // "NAME 0xSIZE": the function is padded to SIZE, the largest it is in any build of the mod, so
                // that what follows it sits at the same place in every build (fishmod/tools/layout). A build
                // where it has grown past that stops here (the location counter can't go back): the order is
                // then made again. Where the function isn't in the build at all, nothing is kept for it.
                if (fields == 2) {
                    static int pads = 0;

                    fprintf(fout, "        .LfishmodAudioPad%d = .;\n        *(.text.%s)\n", pads, name);
                    fprintf(fout, "        . = (. > .LfishmodAudioPad%d) ? .LfishmodAudioPad%d + 0x%X : .;\n", pads, pads, size);
                    pads++;
                } else if (fields == 1) {
                    fprintf(fout, "        *(.text.%s)\n", name);
                }
            }
            if (fishmod_slots_path[0] != '\0')
                fprintf(fout, "        %s(.text*)\n", fishmod_slots_path);
            ordered = 1;
        }
        if (text && fishmod_slots(fout, seg->includes[i].fpath))
            continue;
        fishmod_pin_phase(fout, seg->includes[i].fpath, section_name);
        fprintf(fout, "        %s(.%s*)\n", seg->includes[i].fpath, section_name);
        if (with_padding && seg->includes[i].linkerPadding != 0)
            fprintf(fout, "        . += 0x%X;\n", seg->includes[i].linkerPadding);
    }

    if (order != NULL) {
        fclose(order);
    }
    fprintf(fout, "    }\n");
}

static void write_noload_section(FILE *fout, struct Segment *seg, const char *section_name)
{
    fprintf(fout, "    .%s (NOLOAD) :\n    {\n", section_name);

    for (int i = 0; i < seg->includesCount; i++) {
        fishmod_pin_phase(fout, seg->includes[i].fpath, section_name);
        fprintf(fout, "        %s(.%s*)\n", seg->includes[i].fpath, section_name);
    }

    fprintf(fout, "    }\n");
}

/**
 * Write a linker script for partial linking of a single segment.
 *
 * The original plan was to use a single linker script and pass the files over the command line to ld,
 * however in the iQue version we have bss files that are created partway through linking that need
 * special handling in the linker script.
 */
static void write_linker_script(FILE *fout, struct Segment *seg)
{
    fprintf(fout,
       "OUTPUT_ARCH (mips)\n"
       "SECTIONS {\n"
    );

    write_progbits_section(fout, seg, "text", true);
    write_progbits_section(fout, seg, "data", false);
    write_progbits_section(fout, seg, "rodata", false);
    write_noload_section(fout, seg, "bss");

    /* GNU ld assumes that the linker script always combines .gptab.data and
     * .gptab.sdata into .gptab.sdata, and likewise for .gptab.bss and .gptab.sbss.
     * To avoid dealing with this, we just discard all .gptab sections.
     */
    fprintf(fout,
       "    /DISCARD/ :\n"
       "    {\n"
       "        *(.gptab.*)\n"
       "    }\n"
       "}\n"
    );
}

static void write_overlay_rules(FILE *fout, const char *ovls_dir)
{
    for (int i = 0; i < g_segmentsCount; i++) {
        Segment *seg = &g_segments[i];

        /* Write rule for partial linkage of this segment */
        fprintf(fout, "%s/%s.plf:", ovls_dir, seg->name);
        for (int j = 0; j < seg->includesCount; j++) {
            if (seg->includes[j].fpath[0] != '*') /* Skip iQue bss files */
                fprintf(fout, " \\\n\t\t%s", seg->includes[j].fpath);
        }
        fprintf(fout, " \\\n\t\t$(SEGMENTS_DIR)/Makefile");
        fprintf(fout, "\n"
                      "\t@echo Linking \"%s\"\n"
                      "\t$(SEG_VERBOSE)$(LD) $(SEG_LDFLAGS) -o $@\n"
                      "\n", seg->name);
    }

    /* List every expected plf in a variable */
    fprintf(fout, "SEGMENT_FILES :=");
    for (int i = 0; i < g_segmentsCount; i++)
        fprintf(fout, " \\\n\t\t%s/%s.plf", ovls_dir, g_segments[i].name);

    /* List overlay plfs in a variable */
    fprintf(fout, "\n\nOVL_SEGMENT_FILES :=");
    for (int i = 0; i < g_segmentsCount; i++) {
        if (!(g_segments[i].flags & FLAG_OVL))
            continue;
        fprintf(fout, " \\\n\t\t%s/%s.plf", ovls_dir, g_segments[i].name);
    }
    fprintf(fout, "\n");
}

static void usage(const char *execname)
{
    fprintf(stderr, "spec segment makefile rules generator v0.01\n"
                    "usage: %s SPEC_FILE OBJ_DIRECTORY MAKEFILE_OUT\n"
                    "SPEC_FILE      file describing the organization of object files into segments\n"
                    "OBJ_DIRECTORY  directory where object files will be stored\n"
                    "MAKEFILE_OUT   filename of output makefile to write linking rules\n",
                    execname);
}

int main(int argc, char **argv)
{
    FILE *makefile;
    void *spec;
    size_t size;

    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }

    const char *spec_path = argv[1];
    const char *seg_dir = argv[2];
    const char *makefile_path = argv[3];

    if (seg_dir[0] == '\0') {
        usage(argv[0]);
        return 2;
    }

    spec = util_read_whole_file(spec_path, &size);
    parse_rom_spec(spec, &g_segments, &g_segmentsCount);

    makefile = fopen(makefile_path, "w");
    if (makefile == NULL)
        util_fatal_error("failed to open file '%s' for writing", makefile_path);
    write_overlay_rules(makefile, seg_dir);
    fclose(makefile);

    // Find the longest segment name
    size_t max_name = 1;
    for (int i = 0; i < g_segmentsCount; i++) {
        size_t namelen = strlen(g_segments[i].name);
        if (namelen > max_name)
            max_name = namelen;
    }

    size_t seg_dir_len = strlen(seg_dir);

    // Check if segments dir path needs a path separator appended
    bool needs_sep = seg_dir[seg_dir_len - 1] != '/';
    // Allocate a buffer large enough for the longest path
    char *ld_outpath = malloc(seg_dir_len + needs_sep + max_name + strlen(".ld") + 1);

    // Write segments dir path now since it's always the same
    strcpy(ld_outpath, seg_dir);
    if (needs_sep) {
        ld_outpath[seg_dir_len] = '/';
        seg_dir_len++;
    }

    for (int i = 0; i < g_segmentsCount; i++) {
        Segment *seg = &g_segments[i];

        // Form the full path for this segment's linker script
        strcpy(ld_outpath + seg_dir_len, seg->name);
        ld_outpath[seg_dir_len + strlen(seg->name) + 0] = '.';
        ld_outpath[seg_dir_len + strlen(seg->name) + 1] = 'l';
        ld_outpath[seg_dir_len + strlen(seg->name) + 2] = 'd';
        ld_outpath[seg_dir_len + strlen(seg->name) + 3] = '\0';

        FILE *ldf = fopen(ld_outpath, "w");
        write_linker_script(ldf, seg);
        fclose(ldf);
    }

    free(ld_outpath);


    free_rom_spec(g_segments, g_segmentsCount);
    free(spec);

    return 0;
}
