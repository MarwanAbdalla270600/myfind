#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <fnmatch.h>
#include <signal.h>
#include <limits.h>

#define MAX_PATH 1000
#define MAX_EXPRESSIONS 100

struct expression
{
    char *type; 
    char *param;    
};

struct expression *createExpression(char *type, char *param)
{
    struct expression *newExpression = calloc(1, sizeof(struct expression *));
    newExpression->type = type;
    newExpression->param = param;
    return newExpression;
}

void freeExpressions(struct expression **expressions, int expressionSize)
{
    for (int i = 0; i < expressionSize; i++)
    {
        free(expressions[i]);
    }
}

bool equals(char *a, char *b) {
    return strcmp(a, b) == 0;
}

int isActionOrTest(char *argument)
{
    if (equals(argument, "-print") || equals(argument, "-ls") || equals(argument, "--print") || equals(argument, "--ls"))
    {
        return 1;
    }
    if (equals(argument, "-name")|| equals(argument, "-type") || equals(argument, "-user") || equals(argument, "--name")|| equals(argument, "--type") || equals(argument, "--user"))
    {
        return 0;
    }
    return -1;
}

bool nameCheck(char *filename, char *pattern)
{
    int tmp = fnmatch(pattern, filename, FNM_NOESCAPE); 
    return tmp == 0 ? true : false;
}

bool typeCheck(struct stat fileStat, char filetype)
{
    switch (filetype)
    {
    case 'f':
        return S_ISREG(fileStat.st_mode);
        break;
    case 'd':
        return S_ISDIR(fileStat.st_mode);
        break;
    case 'l':
        return S_ISLNK(fileStat.st_mode);
        break;
    case 'c':
        return S_ISCHR(fileStat.st_mode);
        break;
    case 'b':
        return S_ISBLK(fileStat.st_mode);
        break;
    case 'p':
        return S_ISFIFO(fileStat.st_mode);
        break;
    case 's':
        //return S_ISSOCK(fileStat.st_mode);
        break;
    default:
        break;
    }
    return false;
}

bool userCheck(struct stat fileStat, char *user)
{
    struct passwd *pw = getpwuid(fileStat.st_uid);
    if ((unsigned int)atoi(user) == pw->pw_uid || equals(user, pw->pw_name))
    {
        return true;
    }
    return false;
}

bool checkIfDirectory(char *path) {
    struct stat fileStat;
    stat(path, &fileStat);
    return S_ISDIR(fileStat.st_mode);
}

void printPermissions(struct stat *fileStat) {
    printf((S_ISDIR(fileStat->st_mode)) ? "d" : "-");
    printf((fileStat->st_mode & S_IRUSR) ? "r" : "-");
    printf((fileStat->st_mode & S_IWUSR) ? "w" : "-");
    printf((fileStat->st_mode & S_IXUSR) ? "x" : "-");
    printf((fileStat->st_mode & S_IRGRP) ? "r" : "-");
    printf((fileStat->st_mode & S_IWGRP) ? "w" : "-");
    printf((fileStat->st_mode & S_IXGRP) ? "x" : "-");
    printf((fileStat->st_mode & S_IROTH) ? "r" : "-");
    printf((fileStat->st_mode & S_IWOTH) ? "w" : "-");
    printf((fileStat->st_mode & S_IXOTH) ? "x" : "-");
}

void printDate(struct stat *fileStat) {
    struct tm *timeinfo;
    char buffer[80];
    timeinfo = localtime(&fileStat->st_mtime);
    strftime(buffer, 80, "%b %d %H:%M", timeinfo);
    printf(" %s", buffer);
}

void printfileDetailed(char *path, char *file)
{
    struct stat fileStat;

    char fullpath[MAX_PATH] = {0};
    strcat(fullpath, path);
    if (!equals(file, ""))
    {
        strcat(fullpath, "/");
        strcat(fullpath, file);
    }

    stat(fullpath, &fileStat);
    printf("  %ld", fileStat.st_ino);
    printf("%7ld ", fileStat.st_blocks / 2);
    printPermissions(&fileStat);
    printf(" %3ld\t", fileStat.st_nlink);
    struct group *grp = getgrgid(fileStat.st_gid);
    struct passwd *pwd = getpwuid(fileStat.st_uid);
    printf("%s\t", pwd->pw_name);
    printf(" %s\t", grp->gr_name);
    printf("  %8ld", fileStat.st_size);
    printDate(&fileStat);
    printf(" %s\n", fullpath);
}

void printfile(char *path, char *file, int mode)
{
    if (mode == 0)
    {
        if (equals(file, ""))
        {
            printf("%s\n", path);
        }
        else
        {
            printf("%s/%s\n", path, file);
        }
    }
    else if (mode == 1)
    {
        printfileDetailed(path, file);
    }
}

bool applyChainExpression(struct expression **expressions, int expressionSize, char *path, char *filename)
{
    if (expressionSize == 0)
    {
        printfile(path, filename, 0);
        return true;
    }

    char fullpath[MAX_PATH] = {0};
    strcat(fullpath, path);
    strcat(fullpath, "/");
    strcat(fullpath, filename);
    struct stat fileStat;
    stat(fullpath, &fileStat);

    for (int i = 0; i < expressionSize; i++)
    {
        if (equals(expressions[i]->type, "print"))
        {
            printfile(path, filename, 0);
        }
        else if (equals(expressions[i]->type, "ls"))
        {
            printfile(path, filename, 1);
        }
        else if (equals(expressions[i]->type, "name"))
        {
            if (!nameCheck(filename, expressions[i]->param))
            {
                return false;
            }
        }
        else if (equals(expressions[i]->type, "type"))
        {
            if (!typeCheck(fileStat, expressions[i]->param[0]))
            {
                return false;
            }
        }
        else if (equals(expressions[i]->type, "user"))
        {
            if (!userCheck(fileStat, expressions[i]->param))
            {
                return false;
            }
        }
    }
    if (!equals(expressions[expressionSize - 1]->type, "ls") && !equals(expressions[expressionSize - 1]->type, "print"))
    {
        printfile(path, filename, 0);
    }
    return true;
}

void findAllFiles(char *directory, struct expression **expressions, int expressionSize)
{
    DIR *dir = opendir(directory);
    if (dir == NULL)
    {
        printf("myfind: ‘%s’: Keine Berechtigung\n", directory);
        return;
    }
    struct dirent *entity;
    entity = readdir(dir);
    while (entity != NULL)
    {
        char path[MAX_PATH] = {0};
        strcat(path, directory);
        strcat(path, "/");
        strcat(path, entity->d_name);
        if (!checkIfDirectory(path))
        {
            applyChainExpression(expressions, expressionSize, directory, entity->d_name);
        }
       
        if (checkIfDirectory(path) && !equals(entity->d_name, ".") && !equals(entity->d_name, ".."))
        {
            applyChainExpression(expressions, expressionSize, directory, entity->d_name);
            findAllFiles(path, expressions, expressionSize);
        }
        entity = readdir(dir);
    }
    closedir(dir);
}

int main(int argc, char *argv[])
{
    int c;
    struct expression *expressions[MAX_EXPRESSIONS] = {NULL};
    int expressioncounter = 0;
    char *startingPoint = argv[1];
    if (argc > 1)
    {
        startingPoint = isActionOrTest(argv[1]) == -1 ? argv[1] : ".";
        int length = strlen(startingPoint);
        if (startingPoint[length - 1] == '/' && strlen(startingPoint) != 1)
        {
            startingPoint[length - 1] = '\0';
        }
    }
    else
    {
        startingPoint = ".";
    }

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"print", no_argument, NULL, 'p'},
            {"ls", no_argument, NULL, 'l'},
            {"name", required_argument, NULL, 'n'},
            {"type", required_argument, NULL, 't'},
            {"user", required_argument, NULL, 'u'},
        };
        
        opterr = 0; //Set to -1 to activate standart error message
        c = getopt_long_only(argc, argv, "",
                             long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {

        case 'p':
            expressions[expressioncounter] = createExpression("print", NULL);
            expressioncounter++;
            break;
        case 'l':
            expressions[expressioncounter] = createExpression("ls", NULL);
            expressioncounter++;
            break;
        case 'n':
            expressions[expressioncounter] = createExpression("name", optarg);
            expressioncounter++;
            break;
        case 't':
            if (strlen(optarg) != 1 || strchr("fdlcbps", optarg[0]) == NULL)
            {
                printf("myfind: Ungültiger typ\n");
                exit(0);
            }
            expressions[expressioncounter] = createExpression("type", optarg);
            expressioncounter++;
            break;
        case 'u':
            if (getpwnam(optarg) == NULL)
            {
                fprintf(stderr, "myfind: ‘%s’ ist ein unbekannter Benutzername.\n", optarg);
                exit(0);
            }
            expressions[expressioncounter] = createExpression("user", optarg);
            expressioncounter++;
            break;
        default:
            fprintf(stderr, "myfind: unbekannte Option\n");
            return 0;
            break;
        }
    }

    if (opendir(startingPoint) == NULL)
    {
        fprintf(stderr, "myfind: ‘%s’: Datei oder Verzeichnis nicht gefunden\n", startingPoint);
        exit(0);
    }

    applyChainExpression(expressions, expressioncounter, startingPoint, "");
    findAllFiles(startingPoint, expressions, expressioncounter);
    freeExpressions(expressions, expressioncounter);
    return 0;
}