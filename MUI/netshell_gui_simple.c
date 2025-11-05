/*
 * NetShell MUI GUI Application (Simplified for MorphOS)
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

/* Application ID */
#define APP_NAME "NetShellGUI"
#define DEFAULT_PORT 2324

int main(int argc, char *argv[])
{
    struct Library *MUIMasterBase = NULL;
    Object *app = NULL, *window = NULL;
    Object *hostname_str, *port_str, *connect_btn, *disconnect_btn;
    Object *send_file_btn, *get_file_btn, *command_str, *exec_btn;
    Object *output_area, *status_txt;

    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        printf("Failed to open muimaster.library\n");
        return 10;
    }

    /* Create the application */
    app = ApplicationObject,
        MUIA_Application_Title, (ULONG)APP_NAME,
        MUIA_Application_Version, (ULONG)"1.0",
        MUIA_Application_Copyright, (ULONG)"©2025 sblo",
        MUIA_Application_Author, (ULONG)"sblo",
        MUIA_Application_Description, (ULONG)"NetShell GUI for MorphOS",
        
        SubWindow, window = WindowObject,
            MUIA_Window_Title, (ULONG)"NetShell Manager",
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            MUIA_Window_Activate, TRUE,
            
            WindowContents, VGroup,
                Child, HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    GroupFrameT((STRPTR)"Connection Settings"),
                    Child, Label((STRPTR)"Hostname:"),
                    Child, hostname_str = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"192.168.1.136",
                        MUIA_String_MaxLen, 64,
                    End,
                    Child, Label((STRPTR)"Port:"),
                    Child, port_str = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"2324",
                        MUIA_String_MaxLen, 5,
                    End,
                    Child, connect_btn = SimpleButton((STRPTR)"_Connect"),
                    Child, disconnect_btn = SimpleButton((STRPTR)"_Disconnect"),
                End,
                
                Child, HGroup,
                    GroupFrameT((STRPTR)"File Transfer"),
                    Child, send_file_btn = SimpleButton((STRPTR)"_Send File"),
                    Child, get_file_btn = SimpleButton((STRPTR)"_Get File"),
                End,
                
                Child, HGroup,
                    GroupFrameT((STRPTR)"Command"),
                    Child, command_str = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"",
                        MUIA_String_MaxLen, 256,
                    End,
                    Child, exec_btn = SimpleButton((STRPTR)"_Execute"),
                End,
                
                Child, HGroup,
                    GroupFrameT((STRPTR)"Output"),
                    Child, output_area = TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, (ULONG)"NetShell GUI started...\nReady to connect.\n",
                    End,
                End,
                
                Child, HGroup,
                    Child, status_txt = TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, (ULONG)"Ready - Not Connected",
                    End,
                End,
            End,
        End,
    End;

    if (!app) {
        printf("Failed to create application\n");
        CloseLibrary(MUIMasterBase);
        return 20;
    }

    /* Open the window */
    set(window, MUIA_Window_Open, TRUE);

    /* Check if window was opened */
    if (!MUIWINDOW(window)) {
        printf("Failed to open window\n");
        MUI_DisposeObject(app);
        CloseLibrary(MUIMasterBase);
        return 30;
    }

    printf("NetShell GUI running...\n");

    /* Simple event loop */
    {
        ULONG sigs = 0;
        ULONG class = 0;
        ULONG code = 0;
        BOOL done = FALSE;
        
        while (!done) {
            /* Wait for application messages */
            sigs = Wait(1L << ((struct Library *)MUIMasterBase)->lib_SigBit | SIGBREAKF_CTRL_C);
            
            /* Check for Ctrl+C break */
            if (sigs & SIGBREAKF_CTRL_C) {
                break;
            }
            
            /* Process application input */
            while (DoMethod(app, MUIM_Application_Input, 0) != 0) {
                /* Process all messages */
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