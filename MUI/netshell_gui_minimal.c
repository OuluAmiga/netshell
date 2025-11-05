/*
 * NetShell MUI GUI Application (Minimal working version for MorphOS)
 * Provides a graphical interface for the extended NetShell system
 */

#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct Library *MUIMasterBase = NULL;
    APTR app = NULL, window = NULL;

    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        printf("Failed to open muimaster.library\n");
        return 10;
    }

    /* Create the application using MUI macros */
    app = ApplicationObject,
        MUIA_Application_Title, "NetShellGUI",
        MUIA_Application_Version, "1.0",
        MUIA_Application_Copyright, "©2025 sblo",
        MUIA_Application_Author, "sblo",
        MUIA_Application_Description, "NetShell GUI for MorphOS",
        
        SubWindow, window = WindowObject,
            MUIA_Window_Title, "NetShell Manager",
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            MUIA_Window_Activate, TRUE,
            
            WindowContents, VGroup,
                Child, HGroup,
                    GroupFrameT("Connection Settings"),
                    Child, Label("Hostname:"),
                    Child, StringObject,
                        StringFrame,
                        MUIA_String_Contents, "192.168.1.136",
                        MUIA_String_MaxLen, 64,
                    End,
                    Child, Label("Port:"),
                    Child, StringObject,
                        StringFrame,
                        MUIA_String_Contents, "2324", 
                        MUIA_String_MaxLen, 5,
                    End,
                    Child, SimpleButton("_Connect"),
                    Child, SimpleButton("_Disconnect"),
                End,
                
                Child, HGroup,
                    GroupFrameT("File Transfer"),
                    Child, SimpleButton("_Send File"),
                    Child, SimpleButton("_Get File"),
                End,
                
                Child, HGroup,
                    GroupFrameT("Command"),
                    Child, StringObject,
                        StringFrame,
                        MUIA_String_Contents, "",
                        MUIA_String_MaxLen, 256,
                    End,
                    Child, SimpleButton("_Execute"),
                End,
                
                Child, HGroup,
                    GroupFrameT("Output"),
                    Child, TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, "NetShell GUI started...\nReady to connect.\n",
                    End,
                End,
                
                Child, TextObject,
                    MUIA_Text_Contents, "Ready - Not Connected",
                    MUIA_Background, MUII_TextBack,
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

    printf("NetShell GUI running...\n");

    /* Event loop - wait for window events */
    {
        ULONG signal = 0;
        LONG result = 0;
        
        // Wait for events using MUI's built-in event handling
        while ((result = DoMethod(app, MUIM_Application_Run, &signal)) != MUIV_Application_ReturnID_Quit) {
            // If the result indicates we should quit
            if (result == MUIV_Application_ReturnID_Quit) {
                break;
            }
            
            // Otherwise, continue running
            Wait(signal | SIGBREAKF_CTRL_C);
            
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