/*
 * NetShell MUI GUI Application (Proper MorphOS Server GUI version)
 * Provides a server management interface for the extended NetShell system
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
    APTR start_button, stop_button, port_string, status_text, log_text;

    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        return 10;
    }

    /* Menu structure for detached menu bar */
    struct NewMenu newmenu[] = {
        { NM_TITLE, (STRPTR)"Project", 0, 0, 0, NULL },
            { NM_ITEM, (STRPTR)"About...", (STRPTR)"?", 0, 0, (APTR)1 },
            { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
            { NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0, (APTR)2 },
        { NM_END, NULL, 0, 0, 0, NULL }
    };

    /* Create the application with menu */
    app = ApplicationObject,
        MUIA_Application_Title, (ULONG)"NetShell Server Manager",
        MUIA_Application_Version, (ULONG)"1.0",
        MUIA_Application_Copyright, (ULONG)"©2025 sblo",
        MUIA_Application_Author, (ULONG)"sblo",
        MUIA_Application_Description, (ULONG)"NetShell Server Manager for MorphOS",
        MUIA_Application_Menustrip, MUI_MakeObject(MUIO_Menustrip, newmenu, 0),
        
        SubWindow, window = WindowObject,
            MUIA_Window_Title, (ULONG)"NetShell Server",
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            MUIA_Window_Activate, TRUE,
            
            WindowContents, VGroup,
                Child, HGroup,
                    GroupFrameT((ULONG)"Server Control"),
                    Child, start_button = SimpleButton((ULONG)"_Start Server"),
                    Child, stop_button = SimpleButton((ULONG)"_Stop Server"),
                    Child, Label((ULONG)"Port:"),
                    Child, port_string = StringObject,
                        StringFrame,
                        MUIA_String_Contents, (ULONG)"2324", 
                        MUIA_String_MaxLen, 5,
                    End,
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Server Status"),
                    Child, status_text = TextObject,
                        MUIA_Text_Contents, (ULONG)"Status: Stopped",
                        MUIA_Background, MUII_TextBack,
                    End,
                End,
                
                Child, HGroup,
                    GroupFrameT((ULONG)"Server Log"),
                    Child, log_text = TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, (ULONG)"NetShell Server Manager started...\nReady to start server.\n",
                    End,
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
        ULONG class;
        ULONG code;
        BOOL done = FALSE;
        
        while (!done) {
            /* Get input from the application */
            while ((code = DoMethod(app, MUIM_Application_Input, 0)) != 0) {
                switch (code) {
                    case MUIV_Application_ReturnID_Quit:
                        done = TRUE;
                        break;
                    default:
                        if (code == (ULONG)start_button) {
                            set(status_text, MUIA_Text_Contents, (ULONG)"Status: Running");
                            DoMethod(log_text, MUIM_Text_Set, (ULONG)"Server started.\n");
                        } else if (code == (ULONG)stop_button) {
                            set(status_text, MUIA_Text_Contents, (ULONG)"Status: Stopped");
                            DoMethod(log_text, MUIM_Text_Set, (ULONG)"Server stopped.\n");
                        }
                        break;
                }
            }
            
            if (!done) {
                Wait(1L << ((struct Library *)app)->lib_Summary | SIGBREAKF_CTRL_C);
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