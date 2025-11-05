/*
 * NetShell MUI GUI Application (Proper MorphOS GUI version)
 * Provides a graphical interface for the extended NetShell system
 * Designed as a pure GUI application without console window
 */

#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>

#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct Library *MUIMasterBase = NULL;
    APTR app = NULL, window = NULL;

    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        return 10;
    }

    /* Create the application using MUI macros */
    app = ApplicationObject,
        MUIA_Application_Title, (ULONG)"NetShell Manager",
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
                    GroupFrameT((ULONG)"Connection Settings"),
                    Child, Label((ULONG)"Hostname:"),
                    Child, StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"192.168.1.136",
                        MUIA_String_MaxLen, 64,
                    End,
                    Child, Label((ULONG)"Port:"),
                    Child, StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"2324", 
                        MUIA_String_MaxLen, 5,
                    End,
                    Child, SimpleButton((ULONG)"_Connect"),
                    Child, SimpleButton((ULONG)"_Disconnect"),
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"File Transfer"),
                    Child, SimpleButton((ULONG)"_Send File"),
                    Child, SimpleButton((ULONG)"_Get File"),
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Command"),
                    Child, StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"",
                        MUIA_String_MaxLen, 256,
                    End,
                    Child, SimpleButton((ULONG)"_Execute"),
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Output"),
                    Child, TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, (ULONG)"NetShell GUI started...\nReady to connect.\n",
                    End,
                End,
                
                Child, TextObject,
                    MUIA_Text_Contents, (ULONG)"Ready - Not Connected",
                    MUIA_Background, MUII_TextBack,
                End,
            End,
        End,
    End;

    if (!app) {
        CloseLibrary(MUIMasterBase);
        return 20;
    }

    /* Open the window */
    set(window, MUIA_Window_Open, TRUE);

    /* Event loop for the application */
    {
        ULONG signal = 0;
        LONG result = 0;
        
        // Run the application until quit
        while ((result = DoMethod(app, MUIM_Application_Run, &signal)) != MUIV_Application_ReturnID_Quit) {
            if (result == MUIV_Application_ReturnID_Quit) {
                break;
            }
            
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

    return 0;
}