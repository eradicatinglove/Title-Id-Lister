#include <switch.h>
#include <switch/nacp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TITLES 100
#define WINDOW_SIZE 38  // number of titles shown at once on screen

int main(int argc, char** argv) {
    consoleInit(NULL);
    nsInitialize();
    fsInitialize();

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    NsApplicationRecord records[MAX_TITLES];
    s32 recordCount = 0;
    Result rc = nsListApplicationRecord(records, MAX_TITLES, 0, &recordCount);
    if (R_FAILED(rc)) {
        printf("Error: Failed to list applications: 0x%x\n", rc);
        consoleUpdate(NULL);
        goto cleanup;
    }

    // Preload names for all titles
    char names[MAX_TITLES][0x201] = {{0}};
    for (s32 i = 0; i < recordCount; i++) {
        NsApplicationControlData controlData;
        size_t controlSize;
        rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, records[i].application_id, &controlData, sizeof(controlData), &controlSize);
        if (R_SUCCEEDED(rc)) {
            NacpLanguageEntry* langEntry = NULL;
            if (R_SUCCEEDED(nacpGetLanguageEntry(&controlData.nacp, &langEntry)) && langEntry != NULL) {
                strncpy(names[i], langEntry->name, sizeof(names[i]) - 1);
            } else {
                strncpy(names[i], controlData.nacp.lang[0].name, sizeof(names[i]) - 1);
            }
            names[i][sizeof(names[i]) - 1] = '\0';
        } else {
            snprintf(names[i], sizeof(names[i]), "Unknown Title");
        }
    }

    int scroll = 0;          // Index of the first title shown in the window
    int selected = 0;        // Currently selected index

    consoleClear();

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        // Scroll and selection controls

        // Normal single-step scroll down
        if (kDown & HidNpadButton_Down) {
            if (selected < recordCount - 1) {
                selected++;
                if (selected >= scroll + WINDOW_SIZE) {
                    scroll++;
                    if (scroll > recordCount - WINDOW_SIZE)
                        scroll = recordCount - WINDOW_SIZE;
                    if (scroll < 0)
                        scroll = 0;
                }
            }
        }
        // Normal single-step scroll up
        if (kDown & HidNpadButton_Up) {
            if (selected > 0) {
                selected--;
                if (selected < scroll) {
                    scroll--;
                    if (scroll < 0)
                        scroll = 0;
                }
            }
        }

        // Fast scroll down by 10 titles
        if (kDown & HidNpadButton_Right) {
            if (selected < recordCount - 1) {
                selected += 10;
                if (selected >= recordCount)
                    selected = recordCount - 1;

                // Adjust scroll window to keep selected visible
                if (selected >= scroll + WINDOW_SIZE) {
                    scroll = selected - WINDOW_SIZE + 1;
                    if (scroll > recordCount - WINDOW_SIZE)
                        scroll = recordCount - WINDOW_SIZE;
                    if (scroll < 0)
                        scroll = 0;
                }
            }
        }
        // Fast scroll up by 10 titles
        if (kDown & HidNpadButton_Left) {
            if (selected > 0) {
                selected -= 10;
                if (selected < 0)
                    selected = 0;

                // Adjust scroll window to keep selected visible
                if (selected < scroll) {
                    scroll = selected;
                    if (scroll < 0)
                        scroll = 0;
                }
            }
        }

        if (kDown & HidNpadButton_L) break;

        // Clamp scroll in case recordCount < WINDOW_SIZE
        if (scroll < 0) scroll = 0;
        if (scroll > recordCount - WINDOW_SIZE) scroll = recordCount - WINDOW_SIZE;
        if (scroll < 0) scroll = 0;

        consoleClear();

        printf("Nintendo Switch Installed Titles (Total: %d)\n\n", recordCount);

        int end = scroll + WINDOW_SIZE;
        if (end > recordCount) end = recordCount;

        for (int i = scroll; i < end; i++) {
            if (i == selected) {
                // Highlight selected title ID with brackets and > marker
                printf("> %2d: [%016lX] - %s\n", i, records[i].application_id, names[i]);
            } else {
                printf("  %2d:  %016lX  - %s\n", i, records[i].application_id, names[i]);
            }
        }

        printf("\nUse D-Pad Up/Down to scroll by 1, Left/Right to scroll by 10. Press L to exit.\n");

        consoleUpdate(NULL);
    }

cleanup:
    fsExit();
    nsExit();
    consoleExit(NULL);
    return 0;
}

