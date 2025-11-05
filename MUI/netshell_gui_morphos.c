/*
 * NetShell MUI GUI Application (Compatible for MorphOS)
 * Provides a graphical interface for the extended NetShell system
 */

#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct Library *MUIMasterBase = NULL;
    APTR app = NULL, window = NULL;
    APTR hostname_str, port_str, connect_btn, disconnect_btn;
    APTR send_file_btn, get_file_btn, command_str, exec_btn;
    APTR output_area, status_txt;

    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        printf("Failed to open muimaster.library\n");
        return 10;
    }

    /* Create the application */
    app = MUI_NewObject(MUIC_Application,
        MUIA_Application_Title, (ULONG)"NetShellGUI",
        MUIA_Application_Version, (ULONG)"1.0",
        MUIA_Application_Copyright, (ULONG)"©2025 sblo",
        MUIA_Application_Author, (ULONG)"sblo",
        MUIA_Application_Description, (ULONG)"NetShell GUI for MorphOS",
        MUIA_Application_Window, window = MUI_NewObject(MUIC_Window,
            MUIA_Window_Title, (ULONG)"NetShell Manager",
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            MUIA_Window_Activate, TRUE,
            MUIA_Window_RootObject, MUI_NewObject(MUIC_VGroup,
                MUIA_Group_Child, MUI_NewObject(MUIC_HGroup,
                    MUIA_Group_Child, MUI_NewObject(MUIC_Label, MUIA_Text_Contents, (ULONG)"Hostname:", TAG_DONE),
                    MUIA_Group_Child, hostname_str = MUI_NewObject(MUIC_String,
                        MUIA_String_Contents, (ULONG)"192.168.1.136",
                        MUIA_String_MaxLen, 64,
                        TAG_DONE),
                    MUIA_Group_Child, MUI_NewObject(MUIC_Label, MUIA_Text_Contents, (ULONG)"Port:", TAG_DONE),
                    MUIA_Group_Child, port_str = MUI_NewObject(MUIC_String,
                        MUIA_String_Contents, (ULONG)"2324",
                        MUIA_String_MaxLen, 5,
                        TAG_DONE),
                    MUIA_Group_Child, connect_btn = MUI_MakeObject(MUIO_Button, (ULONG)"_Connect", TAG_DONE),
                    MUIA_Group_Child, disconnect_btn = MUI_MakeObject(MUIO_Button, (ULONG)"_Disconnect", TAG_DONE),
                    TAG_DONE),
                MUIA_Group_Child, MUI_NewObject(MUIC_HGroup,
                    MUIA_Group_Child, send_file_btn = MUI_MakeObject(MUIO_Button, (ULONG)"_Send File", TAG_DONE),
                    MUIA_Group_Child, get_file_btn = MUI_MakeObject(MUIO_Button, (ULONG)"_Get File", TAG_DONE),
                    TAG_DONE),
                MUIA_Group_Child, MUI_NewObject(MUIC_HGroup,
                    MUIA_Group_Child, command_str = MUI_NewObject(MUIC_String,
                        MUIA_String_Contents, (ULONG)"",
                        MUIA_String_MaxLen, 256,
                        TAG_DONE),
                    MUIA_Group_Child, exec_btn = MUI_MakeObject(MUIO_Button, (ULONG)"_Execute", TAG_DONE),
                    TAG_DONE),
                MUIA_Group_Child, MUI_NewObject(MUIC_Group,
                    MUIA_Group_Child, output_area = MUI_NewObject(MUIC_Text,
                        MUIA_Text_Contents, (ULONG)"NetShell GUI started...\nReady to connect.\n",
                        MUIA_Background, MUII_TextBack,
                        TAG_DONE),
                    TAG_DONE),
                MUIA_Group_Child, MUI_NewObject(MUIC_Group,
                    MUIA_Group_Child, status_txt = MUI_NewObject(MUIC_Text,
                        MUIA_Text_Contents, (ULONG)"Ready - Not Connected",
                        MUIA_Background, MUII_TextBack,
                        TAG_DONE),
                    TAG_DONE),
                TAG_DONE),
            TAG_DONE),
        TAG_DONE),
    TAG_DONE);

    if (!app) {
        printf("Failed to create application\n");
        CloseLibrary(MUIMasterBase);
        return 20;
    }

    /* Open the window */
    set(window, MUIA_Window_Open, TRUE);

    printf("NetShell GUI running...\n");

    /* Simple event loop using the proper MorphOS approach */
    {
        ULONG class;
        ULONG code;
        BOOL done = FALSE;
        
        // Wait for application to finish
        while (!done) {
            // Process application input using MUI's built-in event handling
            if (MUI_ApplicationInput(app, &class, &code)) {
                if (class == MUIM_Application_ReturnID && code == MUIV_Application_ReturnID_Quit) {
                    done = TRUE;
                }
            } else {
                // No more messages, do other work or wait
                Wait(1L << MUI_EventHandlerBase(app) | SIGBREAKF_CTRL_C);
            }
            
            // Check for Ctrl+C
            if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
                break;
            }
        }
    }

    /* Close window and cleanup */
    set(window, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
    CloseLibrary(MUIMasterBase);

    printf("NetShell GUI closed.\n");
    return 0;
}