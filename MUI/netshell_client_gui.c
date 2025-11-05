/*
 * NetShell Client MUI GUI Application
 * Provides a client interface for the extended NetShell system
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
    APTR connect_button, disconnect_button, hostname_string, port_string;
    APTR send_file_button, get_file_button, command_string, execute_button;
    APTR output_text, status_text;
    APTR about_button, quit_button;

    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        return 10;
    }

    /* Create the application with menu */
    
    app = ApplicationObject,
        MUIA_Application_Title, (ULONG)"NetShell Client Manager",
        MUIA_Application_Version, (ULONG)"1.0",
        MUIA_Application_Copyright, (ULONG)"©2025 sblo",
        MUIA_Application_Author, (ULONG)"sblo",
        MUIA_Application_Description, (ULONG)"NetShell Client Manager for MorphOS",
        MUIA_Application_Window, window = WindowObject,
            MUIA_Window_Title, (ULONG)"NetShell Client",
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            MUIA_Window_Activate, TRUE,
            
            WindowContents, VGroup,
                Child, HGroup,
                    GroupFrameT((ULONG)"Connection Settings"),
                    Child, Label((ULONG)"Hostname:"),
                    Child, hostname_string = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"192.168.1.136", 
                        MUIA_String_MaxLen, 64,
                    End,
                    Child, Label((ULONG)"Port:"),
                    Child, port_string = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"2324", 
                        MUIA_String_MaxLen, 5,
                    End,
                    Child, connect_button = SimpleButton((ULONG)"_Connect"),
                    Child, disconnect_button = SimpleButton((ULONG)"_Disconnect"),
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"File Transfer"),
                    Child, send_file_button = SimpleButton((ULONG)"_Send File"),
                    Child, get_file_button = SimpleButton((ULONG)"_Get File"),
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Command Execution"),
                    Child, command_string = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"", 
                        MUIA_String_MaxLen, 256,
                    End,
                    Child, execute_button = SimpleButton((ULONG)"_Execute"),
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Output"),
                    Child, output_text = TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, (ULONG)"NetShell Client ready...\nConnect to begin.\n",
                    End,
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Status"),
                    Child, status_text = TextObject,
                        MUIA_Text_Contents, (ULONG)"Ready - Not Connected",
                        MUIA_Background, MUII_TextBack,
                    End,
                End,
                
                Child, HGroup,
                    Child, about_button = SimpleButton((ULONG)"_About"),
                    Child, quit_button = SimpleButton((ULONG)"_Quit"),
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

    /* Event loop for the application with proper event handling */
    {
        ULONG result;
        BOOL done = FALSE;
        
        while (!done) {
            /* Get input from the application */
            while ((result = DoMethod(app, MUIM_Application_Input, &done)) != 0) {
                if (result == MUIV_Application_ReturnID_Quit) {
                    done = TRUE;
                    break;
                } else if (result == (ULONG)quit_button) {
                    done = TRUE;
                } else if (result == (ULONG)about_button) {
                    MUI_Request(app, window, 0, (STRPTR)"About", (STRPTR)"OK", (STRPTR)"NetShell Client Manager\nVersion 1.0\n©2025 sblo");
                } else if (result == (ULONG)connect_button) {
                    set(status_text, MUIA_Text_Contents, (ULONG)"Status: Connected");
                } else if (result == (ULONG)disconnect_button) {
                    set(status_text, MUIA_Text_Contents, (ULONG)"Status: Disconnected");
                }
            }
            
            if (!done) {
                Wait(SIGBREAKF_CTRL_C);
            }
            
            // Check for Ctrl+C
            if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
                done = TRUE;
            }
        }
    }

    /* Close window and cleanup */
    set(window, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
    CloseLibrary(MUIMasterBase);

    return 0;
}