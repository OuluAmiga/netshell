#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#if defined(__MORPHOS__)
#include <proto/icon.h>
#endif

#include <libraries/mui.h>

#include <clib/alib_protos.h>

/* IDs for our application objects */
#define APPNAME "NetShellGUI"

/* Object IDs */
enum {
    OID_MAINWINDOW,
    OID_MENUSTRUCT,
    OID_STARTBUTTON,
    OID_STOPBUTTON,
    OID_PORTLABEL,
    OID_PORTINTEGER,
    OID_LOGTEXTEDITOR,
    OID_APP
};

/* Menu IDs */
enum {
    MID_QUIT,
    MID_ABOUT
};

int main(int argc, char *argv[])
{
    /* Application and window objects */
    Object *app = NULL;
    Object *mainwindow = NULL;
    
    /* Menu objects */
    struct NewMenu newmenu[] = {
        { NM_TITLE, "Project", 0, 0, 0, 0 },
            { NM_ITEM, "About...", "?", 0, 0, (APTR)MID_ABOUT },
            { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
            { NM_ITEM, "Quit", "Q", 0, 0, (APTR)MID_QUIT },
        { NM_END, 0, 0, 0, 0, 0 }
    };
    
    /* Initialize application */
    app = ApplicationObject,
        MUIA_Application_Author, "sblo",
        MUIA_Application_Base, "NETSHELLGUI",
        MUIA_Application_Title, APPNAME,
        MUIA_Application_Version, "$VER: NetShellGUI 1.0",
        MUIA_Application_Copyright, "©2025 sblo",
        MUIA_Application_Description, "NetShell GUI",
        MUIA_Application_Menustrip, MUI_MakeObject(MUIO_Menustrip, newmenu, 0),
        
        SubWindow, mainwindow = WindowObject,
            MUIA_Window_Title, "NetShell",
            MUIA_Window_ID, MAKE_ID('N','S','G','I'),
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            
            WindowContents, VGroup,
                Child, HGroup,
                    Child, Label("Port:"),
                    Child, (Object *)MUI_MakeObject(MUIO_LabelFrame, "Port Settings"),
                    Child, (Object *)MUI_MakeObject(MUIO_BarFrame),
                End,
                Child, HGroup,
                    Child, Label("Port:"),
                    Child, (Object *)MUI_MakeObject(MUIO_BarFrame),
                    /* Integer widget for port input */
                    Child, (Object *)IntegerObject,
                        MUIA_Integer_Minimum, 1,
                        MUIA_Integer_Maximum, 65535,
                        MUIA_Integer_Number, 2323,
                    End,
                End,
                Child, HGroup,
                    Child, (Object *)MUI_MakeObject(MUIO_BarFrame),
                    /* Start button */
                    Child, (Object *)ButtongObject,
                        MUIA_Frame, MUIV_Frame_Button,
                        MUIA_Button_Contents, Label("Start"),
                    End,
                    Child, (Object *)MUI_MakeObject(MUIO_BarFrame),
                    /* Stop button */
                    Child, (Object *)ButtongObject,
                        MUIA_Frame, MUIV_Frame_Button,
                        MUIA_Button_Contents, Label("Stop"),
                    End,
                    Child, (Object *)MUI_MakeObject(MUIO_BarFrame),
                End,
                Child, (Object *)MUI_MakeObject(MUIO_LabelFrame, "Log"),
                /* Text area for logging */
                Child, ScrollgroupObject,
                    MUIA_Scrollgroup_HorizBar, MUIV_Scrollgroup_HorizBar_None,
                    MUIA_Scrollgroup_VertBar, MUIV_Scrollgroup_VertBar_Auto,
                    MUIA_Scrollgroup_Contents, (Object *)TextEditorObject,
                        MUIA_Frame, MUIV_Frame_String,
                        MUIA_TextEditor_ReadOnly, TRUE,
                        MUIA_TextEditor_EditHook, NULL,
                        MUIA_TextEditor_Quiet, TRUE,
                        MUIA_Background, MUII_TextBack,
                    End,
                End,
            End,
        End,
    End;
    
    if (app != NULL) {
        ULONG sigs = 0;
        int done = FALSE;
        
        SetAttrs(mainwindow, MUIA_Window_Open, TRUE, TAG_DONE);
        
        while (!done) {
            ULONG sig = Wait(sigs = (1L << MUI_EventHandlerBase(app)) | SIGBREAKF_CTRL_C);
            
            if (sig & (1L << MUI_EventHandlerBase(app))) {
                ULONG id;
                
                while ((id = DoMethod(app, MUIM_HandleEvent, mainwindow, MUIF_HandleEvent_Application)) != 0) {
                    switch (id) {
                        case MID_QUIT:
                            done = TRUE;
                            break;
                        case MID_ABOUT:
                            MUI_Request(app, mainwindow, 0, "About", "OK", "NetShell GUI Application\nVersion 1.0");
                            break;
                    }
                }
            }
            
            if (sig & SIGBREAKF_CTRL_C) {
                done = TRUE;
            }
        }
        
        SetAttrs(mainwindow, MUIA_Window_Open, FALSE, TAG_DONE);
        MUI_DisposeObject(app);
    }
    
    return 0;
}