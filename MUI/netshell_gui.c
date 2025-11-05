/*
 * NetShell MUI GUI Application
 * Provides a graphical interface for the extended NetShell system
 */

#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
#include <mui/BattMCC_mcc.h>
#include <mui/Text_mcc.h>
#include <mui/Numeric_mcc.h>

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Application ID */
#define APP_NAME "NetShellGUI"
#define DEFAULT_PORT 2324

/* Object IDs for our application */
enum {
    OID_APP,
    OID_WINDOW,
    OID_HOSTNAME_STR,
    OID_PORT_NUM,
    OID_CONNECT_BUTTON,
    OID_DISCONNECT_BUTTON,
    OID_SEND_FILE_BUTTON,
    OID_GET_FILE_BUTTON,
    OID_COMMAND_STR,
    OID_EXECUTE_BUTTON,
    OID_OUTPUT_AREA,
    OID_STATUS_TXT
};

/* Menu IDs */
enum {
    MID_QUIT,
    MID_CONNECT,
    MID_DISCONNECT,
    MID_ABOUT
};

/* Application structure */
struct AppData {
    Object *App;
    Object *Window;
    Object *HostnameStr;
    Object *PortNum;
    Object *ConnectButton;
    Object *DisconnectButton;
    Object *SendFileButton;
    Object *GetFileButton;
    Object *CommandStr;
    Object *ExecuteButton;
    Object *OutputArea;
    Object *StatusTxt;
};

/* Function declarations */
struct AppData *CreateApp(void);
void DisposeApp(struct AppData *app);
void UpdateStatus(struct AppData *app, const char *status);
void AppendOutput(struct AppData *app, const char *text);

struct AppData *CreateApp(void)
{
    struct AppData *app = NULL;
    struct NewMenu newmenu[] = {
        { NM_TITLE, "Project", 0, 0, 0, 0 },
            { NM_ITEM, "Connect", "C", 0, 0, (APTR)MID_CONNECT },
            { NM_ITEM, "Disconnect", "D", 0, 0, (APTR)MID_DISCONNECT },
            { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
            { NM_ITEM, "About...", "?", 0, 0, (APTR)MID_ABOUT },
            { NM_ITEM, NM_BARLABEL, 0, 0, 0, 0 },
            { NM_ITEM, "Quit", "Q", 0, 0, (APTR)MID_QUIT },
        { NM_END, NULL, 0, 0, 0, 0 }
    };
    
    if (!(app = (struct AppData *)AllocVec(sizeof(struct AppData), MEMF_CLEAR))) {
        return NULL;
    }
    
    app->App = ApplicationObject,
        MUIA_Application_Author, "sblo",
        MUIA_Application_Base, "NETSHELLGUI",
        MUIA_Application_Title, APP_NAME,
        MUIA_Application_Version, "$VER: NetShellGUI 1.0",
        MUIA_Application_Copyright, "©2025 sblo",
        MUIA_Application_Description, "NetShell GUI for MorphOS",
        MUIA_Application_Menustrip, MUI_MakeObject(MUIO_Menustrip, newmenu, 0),
        
        SubWindow, app->Window = WindowObject,
            MUIA_Window_Title, "NetShell Manager",
            MUIA_Window_ID, MAKE_ID('N','S','G','I'),
            MUIA_Window_SizeGadget, TRUE,
            MUIA_Window_CloseGadget, TRUE,
            MUIA_Window_Activate, TRUE,
            
            WindowContents, VGroup,
                Child, HGroup,
                    GroupFrameT("Connection Settings"),
                    Child, Label("Hostname:"),
                    Child, app->HostnameStr = StringObject,
                        StringFrame,
                        MUIA_String_Contents, "192.168.1.136",
                        MUIA_String_MaxLen, 64,
                    End,
                    Child, Label("Port:"),
                    Child, app->PortNum = StringObject,
                        StringFrame,
                        MUIA_String_Contents, "2324",
                        MUIA_String_MaxLen, 5,
                    End,
                    Child, app->ConnectButton = SimpleButton("_Connect"),
                    Child, app->DisconnectButton = SimpleButton("_Disconnect"),
                End,
                
                Child, HGroup,
                    GroupFrameT("File Transfer"),
                    Child, app->SendFileButton = SimpleButton("_Send File"),
                    Child, app->GetFileButton = SimpleButton("_Get File"),
                End,
                
                Child, HGroup,
                    GroupFrameT("Command"),
                    Child, app->CommandStr = StringObject,
                        StringFrame,
                        MUIA_String_Contents, "",
                        MUIA_String_MaxLen, 256,
                    End,
                    Child, app->ExecuteButton = SimpleButton("_Execute"),
                End,
                
                Child, HGroup,
                    GroupFrameT("Output"),
                    Child, ScrollgroupObject,
                        MUIA_Scrollgroup_Contents, app->OutputArea = TextObject,
                            TextFrame,
                            MUIA_Background, MUII_TextBack,
                            MUIA_Text_Contents, "NetShell GUI started...\n",
                        End,
                    MUIA_Scrollgroup_HorizBar, MUIV_Scrollgroup_HorizBar_Auto,
                    MUIA_Scrollgroup_VertBar, MUIV_Scrollgroup_VertBar_Auto,
                    End,
                End,
                
                Child, HGroup,
                    Child, app->StatusTxt = TextObject,
                        TextFrame,
                        MUIA_Background, MUII_TextBack,
                        MUIA_Text_Contents, "Ready",
                    End,
                End,
            End,
        End,
    End;
    
    if (!app->App) {
        DisposeApp(app);
        return NULL;
    }
    
    return app;
}

void DisposeApp(struct AppData *app)
{
    if (app) {
        if (app->App) {
            MUI_DisposeObject(app->App);
        }
        FreeVec(app);
    }
}

void UpdateStatus(struct AppData *app, const char *status)
{
    if (app && app->StatusTxt && status) {
        set(app->StatusTxt, MUIA_Text_Contents, status);
    }
}

void AppendOutput(struct AppData *app, const char *text)
{
    if (app && app->OutputArea && text) {
        char *current, *newtext;
        char *separator = "\n";
        
        get(app->OutputArea, MUIA_Text_Contents, &current);
        
        newtext = (char *)AllocVec(strlen(current) + strlen(text) + strlen(separator) + 1, MEMF_ANY);
        if (newtext) {
            strcpy(newtext, current);
            strcat(newtext, text);
            strcat(newtext, separator);
            
            set(app->OutputArea, MUIA_Text_Contents, newtext);
            DoMethod(app->OutputArea, MUIM_Text_Set, newtext);
            
            FreeVec(newtext);
        }
    }
}

int main(int argc, char *argv[])
{
    struct AppData *app = NULL;
    int result = RETURN_OK;
    
    // Initialize application
    if (!(app = CreateApp())) {
        printf("Failed to create application\n");
        return RETURN_FAIL;
    }
    
    // Open the main window
    set(app->Window, MUIA_Window_Open, TRUE);
    
    // Check if window was opened successfully
    if (!MUIWINDOW(app->Window)) {
        printf("Failed to open window\n");
        result = RETURN_FAIL;
        goto cleanup;
    }
    
    // Add initial output
    AppendOutput(app, "NetShell GUI ready. Connect to a server to begin.");
    UpdateStatus(app, "Ready - Not Connected");
    
    // Main event loop
    {
        ULONG sigmask = 0;
        ULONG signals = 0;
        ULONG objclass = 0;
        ULONG objdata = 0;
        BOOL done = FALSE;
        
        // Get the signal mask for our application
        sigmask = 1L << MUI_EventHandlerBase(app->App);
        
        while (!done) {
            // Wait for events
            signals = Wait(sigmask | SIGBREAKF_CTRL_C);
            
            if (signals & SIGBREAKF_CTRL_C) {
                break;
            }
            
            // Process application events
            while (MUI_ApplicationInput(app->App, &objclass, &objdata)) {
                switch (objclass) {
                    case MUIM_Application_ReturnID:
                        switch (objdata) {
                            case MUIV_Application_ReturnID_Quit:
                                done = TRUE;
                                break;
                        }
                        break;
                        
                    case MUIM_Notify:
                        // Handle notifications from gadgets
                        if (objdata == app->ConnectButton) {
                            char *hostname;
                            char port_str[10];
                            int port;
                            
                            get(app->HostnameStr, MUIA_String_Contents, &hostname);
                            get(app->PortNum, MUIA_String_Contents, &port_str);
                            port = atoi(port_str);
                            
                            if (port <= 0 || port > 65535) {
                                port = DEFAULT_PORT;
                            }
                            
                            char conn_msg[256];
                            snprintf(conn_msg, sizeof(conn_msg), "Connecting to %s:%d...", hostname, port);
                            UpdateStatus(app, conn_msg);
                            AppendOutput(app, conn_msg);
                        }
                        else if (objdata == app->DisconnectButton) {
                            UpdateStatus(app, "Disconnected");
                            AppendOutput(app, "Disconnected from server.");
                        }
                        else if (objdata == app->ExecuteButton) {
                            char *command;
                            get(app->CommandStr, MUIA_String_Contents, &command);
                            
                            if (command && strlen(command) > 0) {
                                char exec_msg[512];
                                snprintf(exec_msg, sizeof(exec_msg), "Executing: %s", command);
                                AppendOutput(app, exec_msg);
                                UpdateStatus(app, "Command sent");
                            }
                        }
                        else if (objdata == app->SendFileButton) {
                            AppendOutput(app, "Send File button clicked - would open file requester");
                            UpdateStatus(app, "Send File mode");
                        }
                        else if (objdata == app->GetFileButton) {
                            AppendOutput(app, "Get File button clicked - would open file requester");
                            UpdateStatus(app, "Get File mode");
                        }
                        break;
                        
                    case MUIM_Menuitem:
                        switch ((int)objdata) {
                            case MID_QUIT:
                                done = TRUE;
                                break;
                            case MID_CONNECT:
                                // Connect action
                                UpdateStatus(app, "Connecting...");
                                AppendOutput(app, "Connect menu selected");
                                break;
                            case MID_DISCONNECT:
                                // Disconnect action
                                UpdateStatus(app, "Disconnected");
                                AppendOutput(app, "Disconnect menu selected");
                                break;
                            case MID_ABOUT:
                                MUI_Request(app->App, app->Window, 0, "About", "OK", 
                                          "NetShell GUI\nVersion 1.0\n©2025 sblo");
                                break;
                        }
                        break;
                }
            }
        }
    }
    
cleanup:
    // Close window if it's open
    if (MUIWINDOW(app->Window)) {
        set(app->Window, MUIA_Window_Open, FALSE);
    }
    
    // Dispose of application
    DisposeApp(app);
    
    return result;
}