#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>

/* IDs for our application objects */
#define APPNAME "NetShellGUI"

/* Menu item commands */
#define MENU_ABOUT 1
#define MENU_QUIT  2

int main(int argc, char *argv[])
{
    /* Application and window objects */
    APTR app = NULL;
    APTR mainwindow = NULL;
    APTR startbutton = NULL;
    APTR stopbutton = NULL;
    APTR portstring = NULL;
    APTR logtext = NULL;
    
    /* Create application */
    app = MUI_NewObject(MUIC_Application,
        MUIA_Application_Title, (ULONG)APPNAME,
        MUIA_Application_Version, (ULONG)"1.0",
        MUIA_Application_Copyright, (ULONG)"©2025 sblo",
        MUIA_Application_Author, (ULONG)"sblo",
        MUIA_Application_Description, (ULONG)"NetShell GUI Application",
        TAG_END);
    
    if (!app) {
        return 10;
    }
    
    /* Create window */
    mainwindow = MUI_NewObject(MUIC_Window,
        MUIA_Window_Title, (ULONG)"NetShell Control",
        MUIA_Window_Width, MUIV_Window_Width_Screen(60),
        MUIA_Window_Height, MUIV_Window_Height_Screen(40),
        MUIA_Window_SizeGadget, TRUE,
        MUIA_Window_CloseGadget, TRUE,
        MUIA_Window_Activate, TRUE,
        MUIA_Window_RootObject, MUI_NewObject(MUIC_VGroup,
            MUIA_Group_Spacing, 5,
            MUIA_Group_Child, MUI_NewObject(MUIC_HGroup,
                MUIA_Group_Child, MUI_NewObject(MUIC_Label, MUIA_Text_Contents, (ULONG)"Port:", TAG_END),
                MUIA_Group_Child, (portstring = MUI_NewObject(MUIC_String, 
                    MUIA_String_MaxChars, 5, 
                    MUIA_String_Contents, (ULONG)"2323",
                    TAG_END)),
                TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_HGroup,
                MUIA_Group_Child, (startbutton = MUI_NewObject(MUIC_Button,
                    MUIA_Button_Contents, (ULONG)"Start",
                    TAG_END)),
                MUIA_Group_Child, (stopbutton = MUI_NewObject(MUIC_Button,
                    MUIA_Button_Contents, (ULONG)"Stop",
                    TAG_END)),
                TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_Label,
                MUIA_Text_Contents, (ULONG)"Log Output:",
                TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
                MUIA_Scrollgroup_Child, (logtext = MUI_NewObject(MUIC_Text,
                    MUIA_Text_Contents, (ULONG)"NetShell GUI initialized...\\nReady to start server.\\n",
                    MUIA_Frame, MUIV_Frame_String,
                    MUIA_Background, MUII_TextBack,
                    TAG_END)),
                TAG_END),
            TAG_END),
        TAG_END),
        TAG_END);
    
    if (!mainwindow) {
        MUI_DisposeObject(app);
        return 20;
    }
    
    /* Open the window */
    set(mainwindow, MUIA_Window_Open, TRUE);
    
    /* Wait for window to be closed */
    {
        LONG quit = FALSE;
        ULONG sigmask;
        ULONG signals;
        
        sigmask = 1L << (APTR)app;
        
        while (!quit) {
            signals = Wait(sigmask | SIGBREAKF_CTRL_C);
            
            if (signals & SIGBREAKF_CTRL_C) {
                quit = TRUE;
            }
            
            /* Process application events */
            {
                ULONG objclass;
                ULONG objcode;
                
                while (MUI_ApplicationInput(app, &objclass, &objcode)) {
                    switch (objclass) {
                        case MUIM_Application_ReturnID:
                            if (objcode == MUIV_Application_ReturnID_Quit) {
                                quit = TRUE;
                            }
                            break;
                            
                        default:
                            MUI_DispatchEvent(app);
                            break;
                    }
                }
            }
        }
    }
    
    /* Clean up */
    set(mainwindow, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(mainwindow);
    MUI_DisposeObject(app);
    
    return 0;
}