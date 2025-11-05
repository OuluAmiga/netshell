#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

int main(int argc, char *argv[])
{
    struct Library *MUIMasterBase = NULL;
    Object *app, *window, *portstring, *startbutton, *stopbutton, *logtext;
    
    /* Open MUIMaster library */
    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) {
        return 1;
    }
    
    app = ApplicationObject,
        MUIA_Application_Title, "NetShellGUI",
        MUIA_Application_Version, "1.0",
        MUIA_Application_Copyright, "©2025 sblo",
        MUIA_Application_Author, "sblo",
        MUIA_Application_Description, "NetShell GUI Application",
        SubWindow, window = WindowObject,
            MUIA_Window_Title, "NetShell Control",
            MUIA_Window_ID, MAKE_ID('N','S','G','I'),
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            WindowContents, VGroup,
                Child, HGroup,
                    Child, Label("Port: "),
                    Child, portstring = StringObject,
                        StringFrame,
                        MUIA_String_MaxLen, 5,
                        MUIA_String_Contents, "2323",
                    End,
                End,
                Child, HGroup,
                    Child, startbutton = SimpleButton("Start"),
                    Child, stopbutton = SimpleButton("Stop"),
                End,
                Child, Label("Log Output:"),
                Child, ScrollgroupObject,
                    MUIA_Scrollgroup_Contents, logtext = TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, "NetShell GUI initialized...\nReady to start server.\n",
                        MUIA_Text_HiSpeed, TRUE,
                    End,
                End,
            End,
        End,
    End;
    
    if (app) {
        set(window, MUIA_Window_Open, TRUE);
        
        {   /* Event loop */
            ULONG signal;
            ULONG signals;
            BOOL done = FALSE;
            
            while (!done) {
                Wait(1L << ((struct Task *)app)->tc_SigBit | SIGBREAKF_CTRL_C);
                
                while ((signal = DoMethod(app, MUIM_Application_Input, &done)) != 0) {
                    if (signal == MUIV_Application_ReturnID_Quit) {
                        done = TRUE;
                    }
                }
            }
        }
        
        set(window, MUIA_Window_Open, FALSE);
        MUI_DisposeObject(app);
    }
    
    CloseLibrary(MUIMasterBase);
    return 0;
}