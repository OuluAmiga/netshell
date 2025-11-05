#ifndef GUI_APP_H
#define GUI_APP_H

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

/* Function declarations */
int main(int argc, char *argv[]);

#endif /* GUI_APP_H */