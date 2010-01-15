/******************************************************************************
 *                                                                            *
 * Copyright (C) 2006-2009 by Tor Andersson.                                  *
 * Copyright (C) 2008-2009 by Ben Cressey.                                    *
 * Copyright (C) 2009 by Baltasar Garc�a Perez-Schofield.                     *
 *                                                                            *
 * This file is part of Gargoyle.                                             *
 *                                                                            *
 * Gargoyle is free software; you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Gargoyle is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Gargoyle; if not, write to the Free Software                    *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA *
 *                                                                            *
 *****************************************************************************/

#include "launcher.h"

#define T_ADRIFT    "scare"
#define T_ADVSYS    "advsys"
#define T_AGT       "agility"
#define T_ALAN2     "alan2"
#define T_ALAN3     "alan3"
#define T_GLULX     "git"
#define T_HUGO      "hugo"
#define T_JACL      "jacl"
#define T_LEV9      "level9"
#define T_MGSR      "magnetic"
#define T_QUEST     "geas"
#define T_TADS2     "tadsr"
#define T_TADS3     "tadsr"
#define T_ZCODE     "frotz"
#define T_ZSIX      "nitfol"

#define ID_ZCOD (giblorb_make_id('Z','C','O','D'))
#define ID_GLUL (giblorb_make_id('G','L','U','L'))

const char * AppName = "Gargoyle " VERSION;

#define MaxBuffer 1024
char dir[MaxBuffer];
char buf[MaxBuffer];
char tmp[MaxBuffer];

static char *terp = NULL;

#if defined (OS_WINDOWS)
    static const char * LaunchingTemplate = "\"%s\\%s.exe\" %s \"%s\"";
    static const char * DirSeparator = "\\";
#else
#if defined (OS_UNIX) || defined (OS_MAC)
    static const char * LaunchingTemplate = "%s/%s";
    static const char * DirSeparator = "/";
#endif
#endif

char filterlist[] =
"All Games\0*.taf;*.agx;*.d$$;*.acd;*.a3c;*.asl;*.cas;*.ulx;*.hex;*.jacl;*.j2;*.gam;*.t3;*.z?;*.l9;*.sna;*.mag;*.dat;*.blb;*.glb;*.zlb;*.blorb;*.gblorb;*.zblorb\0"
"Adrift Games (*.taf)\0*.taf\0"
"AdvSys Games (*.dat)\0*.dat\0"
"AGT Games (*.agx)\0*.agx;*.d$$\0"
"Alan Games (*.acd,*.a3c)\0*.acd;*.a3c\0"
"Glulxe Games (*.ulx)\0*.ulx;*.blb;*.blorb;*.glb;*.gblorb\0"
"Hugo Games (*.hex)\0*.hex\0"
"JACL Games (*.jacl,*.j2)\0*.jacl;*.j2\0"
"Level 9 (*.sna)\0*.sna\0"
"Magnetic Scrolls (*.mag)\0*.mag\0"
"Quest Games (*.asl,*.cas)\0*.asl;*.cas\0"
"TADS 2 Games (*.gam)\0*.gam;*.t3\0"
"TADS 3 Games (*.t3)\0*.gam;*.t3\0"
"Z-code Games (*.z?)\0*.z?;*.zlb;*.zblorb\0"
"All Files\0*\0"
"\0\0";

/* OS-dependent functions -------------------------------------------------- */
void os_init(void)
{
#if defined (OS_UNIX)
    gtk_init(NULL, NULL);
#else
#if defined (OS_MAC)
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
    [NSApp setDelegate:[[nsDelegate alloc] init]];
    [NSApp finishLaunching];
    [NSApp nextEventMatchingMask: NSAnyEventMask 
                       untilDate: [NSDate distantPast] 
                          inMode: NSDefaultRunLoopMode 
                         dequeue: YES];	
    [pool drain];
#endif
#endif
}

int os_args(int argc, char **argv)
{
    #if defined (OS_WINDOWS) || defined (OS_UNIX)
        if (argc == 2)
        {
            strcpy(buf, argv[1]);
            return TRUE;
        }
    #else
    #if defined (OS_MAC)
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        NSString * nsArgv = [[NSApp delegate] argv];
    
        int size = [nsArgv length];
        
        if (size)
        {
            CFStringGetBytes((CFStringRef) nsArgv, CFRangeMake(0, size),
                             kCFStringEncodingASCII, 0, FALSE,
                             buf, MaxBuffer, NULL);
            int bounds = size < MaxBuffer ? size : MaxBuffer;
            buf[bounds] = '\0';
        }
        
        [pool drain];	
        return size; 
    #endif
    #endif
}

void showMessageBoxError(const char * msg)
{
    #if defined (OS_WINDOWS)
        MessageBox(NULL, msg, AppName, MB_ICONERROR);
    #else
    #if defined (OS_UNIX)
        GtkWidget * msgDlg = gtk_message_dialog_new(NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_CLOSE,
                                             "%s", msg
        );

        gtk_window_set_title(GTK_WINDOW(msgDlg), AppName);
        gtk_dialog_run(GTK_DIALOG(msgDlg ));
        gtk_widget_destroy(msgDlg);
    #else
    #if defined (OS_MAC)
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        NSString * nsMsg = [[NSString alloc] initWithCString: msg encoding: NSASCIIStringEncoding];
        NSRunAlertPanel(@"Fatal error", nsMsg, NULL, NULL, NULL);
        [nsMsg release];
        [pool drain];
    #endif
    #endif
    #endif
}

bool askFileName(char * buffer, int maxBuffer)
{
    char *title = "Gargoyle " VERSION;
    *buffer = 0;

    #if defined (OS_WINDOWS)
        OPENFILENAME ofn;

        memset(&ofn, 0, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = maxBuffer;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = title;
        ofn.lpstrFilter = filterlist;
        ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;

        return GetOpenFileName(&ofn);
    #else
    #if defined (OS_UNIX)
        GtkWidget * openDlg = gtk_file_chooser_dialog_new(AppName, NULL,
                                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                          NULL);

        if (getenv("HOME"))
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(openDlg), getenv("HOME"));

        gint result = gtk_dialog_run(GTK_DIALOG(openDlg));

        if (result == GTK_RESPONSE_ACCEPT)
            strcpy(buffer, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(openDlg)));

        gtk_widget_destroy(openDlg);
        gdk_flush();

        return (*buffer != 0);
    #else
    #if defined (OS_MAC)
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

        NSOpenPanel * openDlg = [NSOpenPanel openPanel];
        [openDlg setCanChooseFiles: YES];
        [openDlg setCanChooseDirectories: NO];
        [openDlg setAllowsMultipleSelection: NO];
    
        NSString * nsTitle = [[NSString alloc] initWithCString: title encoding: NSASCIIStringEncoding];
        [openDlg setTitle: nsTitle];
        [nsTitle release];

        if ([openDlg runModal] == NSFileHandlingPanelOKButton)
        {
            NSString * fileref = [openDlg filename];
            int size = [fileref length];

            CFStringGetBytes((CFStringRef) fileref, CFRangeMake(0, size),
                             kCFStringEncodingASCII, 0, FALSE,
                             buffer, maxBuffer, NULL);
        
            int bounds = size < maxBuffer ? size : maxBuffer;
            buffer[bounds] = '\0';
        }

        [pool drain];
        return (*buffer != 0);
    #endif
    #endif
    #endif
}

char * getCurrentWorkingDirectory(char *buffer, int maxBuffer)
{
    #if defined (OS_WINDOWS)
        if( !GetCurrentDirectory( maxBuffer, buffer ) ) {
            *buffer = 0;
        }
    #else
    #if defined (OS_UNIX) || defined (OS_MAC)
        if ( getcwd( buffer, maxBuffer ) == NULL ) {
            *buffer = 0;
        }
    #endif
    #endif

    if ( *buffer == 0 ) {
        showMessageBoxError( "FATAL: Unable to retrieve current directory" );
    }

    return buffer;
}

void getExeFullPath(char *path)
{
    char exepath[MaxBuffer] = {0};
    unsigned int exelen;

    #if defined (OS_WINDOWS)
        exelen = GetModuleFileName(NULL, exepath, sizeof(exepath));
    #else
    #if defined (OS_UNIX)
        exelen = readlink("/proc/self/exe", exepath, sizeof(exepath));
    #else
    #if defined (OS_MAC)
        char tmppath[MaxBuffer] = {0};
        exelen = sizeof(tmppath);
        _NSGetExecutablePath(tmppath, &exelen);
        exelen = (realpath(tmppath, exepath) != NULL);
    #endif
    #endif
    #endif

   if (exelen <= 0 || exelen >= MaxBuffer)
       showMessageBoxError( "FATAL: Unable to locate executable path" );

   strcpy(path, exepath);
   return;
}

bool exec(const char *cmd, char **args)
{
    #if defined (OS_WINDOWS)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        int res;

        memset(&si, 0, sizeof si);
        memset(&pi, 0, sizeof pi);

        si.cb = sizeof si;

        sprintf(tmp,"%s",cmd);

        res = CreateProcess(
            NULL,	/* no module name (use cmd line)    */
            tmp,	/* command line                     */
            NULL,	/* security attrs,                  */
            NULL,	/* thread attrs,                    */
            FALSE,	/* inherithandles                   */
            0,		/* creation flags                   */
            NULL,	/* environ                          */
            NULL,	/* cwd                              */
            &si,	/* startupinfo                      */
            &pi		/* procinfo                         */
        );

        return (res != 0);
    #else
    #if defined (OS_UNIX)	
        return (execv(cmd, args) == 0);
    #else
    #if defined (OS_MAC)
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];	
        NSTask * proc = [[NSTask alloc] init];
    
        NSString * nsCmd = [[NSString alloc] initWithCString: cmd encoding: NSASCIIStringEncoding];
        NSMutableArray * nsArgs = [NSMutableArray arrayWithCapacity:2];

        if (args[1])
            [nsArgs addObject: [[NSString alloc] initWithCString: args[1] encoding: NSASCIIStringEncoding]];

        if (args[2])
            [nsArgs addObject: [[NSString alloc] initWithCString: args[2] encoding: NSASCIIStringEncoding]];
    
        if ([nsCmd length] && [nsArgs count])
        {
            [proc setLaunchPath: nsCmd];
            [proc setArguments: nsArgs];
            [proc launch];
        }
        
        [pool drain];
        return [proc isRunning];
    #endif
    #endif
    #endif
}

/* Other functions --------------------------------------------------------- */
void cleanUp()
/* Proceed to clean all acquired resources */
{
    if ( terp != NULL ) {
        free( terp );
        terp = NULL;
    }
}

void cleanAndExit(int exitCode)
{
    cleanUp();
    exit(exitCode);
}

void runterp(char *exe, char *flags)
{
    char *args[] = {NULL, NULL, NULL};
    
    #if defined (OS_WINDOWS)
        sprintf(tmp, LaunchingTemplate, dir, exe, flags, buf);
    #else
    #if defined (OS_UNIX) || defined (OS_MAC)
        sprintf(tmp, LaunchingTemplate, dir, exe);
        if (strstr(flags, "-"))
        {
            args[0] = exe;
            args[1] = flags;
            args[2] = buf;
        }
        else
        {
            args[0] = exe;
            args[1] = buf;
        }
    #endif
    #endif

    if (!exec(tmp, args)) {
        showMessageBoxError("Could not start 'terp.\nSorry.");
        cleanAndExit(EXIT_FAILURE);
    }
    cleanAndExit(EXIT_SUCCESS);
}

void runblorb(void)
{
    char magic[4];
    strid_t file;
    giblorb_result_t res;
    giblorb_err_t err;
    giblorb_map_t *map;

    sprintf(tmp, "Could not load Blorb file:\n%s\n", buf);

    file = glkunix_stream_open_pathname(buf, 0, 0);
    if (!file) {
        showMessageBoxError(tmp);
        cleanAndExit(EXIT_FAILURE);
    }

    err = giblorb_create_map(file, &map);
    if (err) {
        showMessageBoxError(tmp);
        cleanAndExit(EXIT_FAILURE);
    }

    err = giblorb_load_resource(map, giblorb_method_FilePos,
            &res, giblorb_ID_Exec, 0);
    if (err) {
        showMessageBoxError(tmp);
        cleanAndExit(EXIT_FAILURE);
    }

    glk_stream_set_position(file, res.data.startpos, 0);
    glk_get_buffer_stream(file, magic, 4);

    switch (res.chunktype)
    {
    case ID_ZCOD:
        if (terp)
            runterp(terp, "");
        else if (magic[0] == 6)
            runterp(T_ZSIX, "");
        else
            runterp(T_ZCODE, "");
        break;

    case ID_GLUL:
        if (terp)
            runterp(terp, "");
        else
            runterp(T_GLULX, "");
        break;

    default:
        sprintf(tmp, "Unknown game type in Blorb file:\n%s\n", buf);
        showMessageBoxError(tmp);
        cleanAndExit(EXIT_FAILURE);
    }
}

char *readconfig(char *fname, char *gamefile)
{
    FILE *f;
    char buf[MaxBuffer];
    char *s;
    char *cmd, *arg;
    int accept = 0;
    int i;

    f = fopen(fname, "r");
    if (!f)
        return NULL;

    while (1)
    {
        s = fgets(buf, sizeof buf, f);
        if (!s)
            break;

        /* kill newline */
        if (strlen(buf) && buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = 0;

        if (!strlen(buf) || buf[0] == '#')
            continue;

        if (strchr(buf,'['))
        {
            for (i = 0; i < strlen(buf); i++)
                buf[i] = tolower(buf[i]);

            if (strstr(buf, gamefile))
                accept = 1;
            else
                accept = 0;
        }

        if (!accept)
            continue;

        cmd = strtok(buf, "\r\n\t ");
        if (!cmd)
            continue;

        arg = strtok(NULL, "\r\n\t #");
        if (!arg)
            continue;

        if (!strcmp(cmd, "terp"))
            terp = strdup(arg);
    }

    fclose(f);
    return terp;
}

void configterp(char *game, int ext)
{
    char *ini = "garglk.ini";
    char path[MaxBuffer];
    char gameref[MaxBuffer];
    char gameid[MaxBuffer];
    int i;

    if (!ext) {
        /* ini search based on full game name */
        if (strrchr(game, *DirSeparator))
            strcpy(gameref, strrchr(game, *DirSeparator)+1);
        else
            return;
    } else {
        /* ini search based on extension */
        if (strrchr(game, '.')) {
            gameref[0] = ' ';
            gameref[1] = '\0';
            strcat(gameref,strrchr(game, '.'));
        }
        else
            return;
    }

    /* use lowercase game name to compare with ini value */
    gameid[strlen(gameref)] = '\0';
    for (i = 0; i < strlen(gameref); i++)
        gameid[i] = tolower(gameref[i]);

    /* ini stored in game directory */
    strcpy(path, game);
    if (strrchr(path, '.'))
        strcpy(strrchr(path, '.'), ".ini");
    else
        strcat(path, ".ini");

    readconfig(path, gameid);
    if (terp)
        return;

    /* ini stored in working directory */
    getCurrentWorkingDirectory(path, sizeof path);
    strcat(path, DirSeparator);
    strcat(path, ini);
    readconfig(path, gameid);
    if (terp)
        return;

    /* home directory */
    if (getenv("HOME")) {
        strcpy(path, getenv("HOME"));
        strcat(path, DirSeparator);
        strcat(path, ini);
        readconfig(path, gameid);
        if (terp)
            return;
    }

    /* freedesktop.org config directory */
    if (getenv("XDG_CONFIG_HOME"))
    {
        strcpy(path, getenv("XDG_CONFIG_HOME"));
        strcat(path, DirSeparator);
        strcat(path, ini);
        readconfig(path, gameid);
        if (terp)
            return;
    }

    /* gargoyle directory */
    strcpy(path, dir);
    strcat(path, DirSeparator);
    strcat(path, ini);
    readconfig(path, gameid);
    if (terp)
        return;

    /* no terp found */
    return;
}

int main(int argc, char **argv)
{
    char *ext;
    char *gamepath;
    char *dirpos;

	os_init();

    /* get dir of executable */
    getExeFullPath(dir);
    dirpos = strrchr(dir, *DirSeparator);
    if ( dirpos != NULL ) {
        *dirpos = '\0';
    }

    if (!os_args(argc, argv))
    {
        if (!askFileName(buf,sizeof(buf))) {
            strcpy(buf, "");
        }
    }

    if (strlen(buf) == 0) {
        cleanAndExit(EXIT_SUCCESS);
    }

    gamepath = buf;

    configterp(gamepath, FALSE);
    if (!terp)
        configterp(gamepath, TRUE);

    ext = strrchr(gamepath, '.');
    if (ext)
        ext++;
    else
        ext = "";

    if (!strcasecmp(ext, "blb"))
        runblorb();
    if (!strcasecmp(ext, "blorb"))
        runblorb();
    if (!strcasecmp(ext, "glb"))
        runblorb();
    if (!strcasecmp(ext, "gbl"))
        runblorb();
    if (!strcasecmp(ext, "gblorb"))
        runblorb();
    if (!strcasecmp(ext, "zlb"))
        runblorb();
    if (!strcasecmp(ext, "zbl"))
        runblorb();
    if (!strcasecmp(ext, "zblorb"))
        runblorb();

    if (terp)
        runterp(terp, "");

    if (!strcasecmp(ext, "dat"))
        runterp(T_ADVSYS, "");

    if (!strcasecmp(ext, "d$$"))
        runterp(T_AGT, "-gl");
    if (!strcasecmp(ext, "agx"))
        runterp(T_AGT, "-gl");

    if (!strcasecmp(ext, "acd"))
        runterp(T_ALAN2, "");

    if (!strcasecmp(ext, "a3c"))
        runterp(T_ALAN3, "");

    if (!strcasecmp(ext, "taf"))
        runterp(T_ADRIFT, "");

    if (!strcasecmp(ext, "ulx"))
        runterp(T_GLULX, "");

    if (!strcasecmp(ext, "hex"))
        runterp(T_HUGO, "");

    if (!strcasecmp(ext, "jacl"))
        runterp(T_JACL, "");
    if (!strcasecmp(ext, "j2"))
        runterp(T_JACL, "");

    if (!strcasecmp(ext, "gam"))
        runterp(T_TADS2, "");

    if (!strcasecmp(ext, "t3"))
        runterp(T_TADS3, "");

    if (!strcasecmp(ext, "z1"))
        runterp(T_ZCODE, "");
    if (!strcasecmp(ext, "z2"))
        runterp(T_ZCODE, "");
    if (!strcasecmp(ext, "z3"))
        runterp(T_ZCODE, "");
    if (!strcasecmp(ext, "z4"))
        runterp(T_ZCODE, "");
    if (!strcasecmp(ext, "z5"))
        runterp(T_ZCODE, "");
    if (!strcasecmp(ext, "z7"))
        runterp(T_ZCODE, "");
    if (!strcasecmp(ext, "z8"))
        runterp(T_ZCODE, "");

    if (!strcasecmp(ext, "z6"))
        runterp(T_ZSIX, "");

    if (!strcasecmp(ext, "l9"))
        runterp(T_LEV9, "");
    if (!strcasecmp(ext, "sna"))
        runterp(T_LEV9, "");
    if (!strcasecmp(ext, "mag"))
        runterp(T_MGSR, "");

    if (!strcasecmp(ext, "asl"))
        runterp(T_QUEST, "");
    if (!strcasecmp(ext, "cas"))
        runterp(T_QUEST, "");

    sprintf(tmp, "Unknown file type: \"%s\"\nSorry.", ext);
    showMessageBoxError(tmp);

    cleanUp();
    return EXIT_FAILURE;
}
