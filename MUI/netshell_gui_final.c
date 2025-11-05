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
#include <clib/alib_protos.h>  // For DoMethod

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
            MUIA_Window_ID, MAKE_ID('N','S','G','I'),
            
            WindowContents, VGroup,
                Child, GroupObject,
                    MUIA_Group_Frame, MUIV_Frame_Group,
                    MUIA_Group_FrameTitle, "Connection Settings",
                    MUIA_Group_Child, HGroup,
                        Child, Label("Hostname:"),
                        Child, hostname_str = StringObject,
                            StringFrame,
                            MUIA_String_Contents, "192.168.1.136",
                            MUIA_String_MaxLen, 64,
                        End,
                        Child, Label("Port:"),
                        Child, port_str = StringObject,
                            StringFrame,
                            MUIA_String_Contents, "2324", 
                            MUIA_String_MaxLen, 5,
                        End,
                        Child, connect_btn = SimpleButton("_Connect"),
                        Child, disconnect_btn = SimpleButton("_Disconnect"),
                    End,
                End,
                
                Child, GroupObject,
                    MUIA_Group_Frame, MUIV_Frame_Group,
                    MUIA_Group_FrameTitle, "File Transfer",
                    MUIA_Group_Child, HGroup,
                        Child, send_file_btn = SimpleButton("_Send File"),
                        Child, get_file_btn = SimpleButton("_Get File"),
                    End,
                End,
                
                Child, GroupObject,
                    MUIA_Group_Frame, MUIV_Frame_Group,
                    MUIA_Group_FrameTitle, "Command",
                    MUIA_Group_Child, HGroup,
                        Child, command_str = StringObject,
                            StringFrame,
                            MUIA_String_Contents, "",
                            MUIA_String_MaxLen, 256,
                        End,
                        Child, exec_btn = SimpleButton("_Execute"),
                    End,
                End,
                
                Child, GroupObject,
                    MUIA_Group_Frame, MUIV_Frame_Group,
                    MUIA_Group_FrameTitle, "Output",
                    MUIA_Group_Child, TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, "NetShell GUI started...\nReady to connect.\n",
                        MUIA_Text_HiChar, 0,
                    End,
                End,
                
                Child, HGroup,
                    Child, status_txt = TextObject,
                        MUIA_Text_Contents, "Ready - Not Connected",
                        MUIA_Background, MUII_TextBack,
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

    printf("NetShell GUI running...\n");

    /* Simple event loop using the proper MorphOS approach */
    {
        ULONG sigs = 0;
        
        // Wait for application to finish - this will keep the window open
        // The window closing will cause the application to exit naturally
        while (TRUE) {
            // Wait for either MUI events or Ctrl+C break
            sigs = Wait(1L << ((struct Library *)MUIMasterBase)->lib_Summary | SIGBREAKF_CTRL_C);
            
            // Check for Ctrl+C
            if (sigs & SIGBREAKF_CTRL_C) {
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