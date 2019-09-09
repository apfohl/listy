#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <jzon.h>

const int CHUNK = 1024;

extern char **environ;

char *read_file(const char *path) {
    char *result = NULL;
    size_t result_size = 0;

    FILE *file = fopen(path, "r");

    char buffer[CHUNK];
    size_t nread = 0;

    while ((nread = fread(buffer, sizeof(char), CHUNK, file)) > 0) {
        result = realloc(result, (result_size + nread) * sizeof(char));
        memcpy(result + result_size, buffer, nread);
        result_size += nread;
    }

    fclose(file);

    result = realloc(result, (result_size + 1) * sizeof(char));
    result_size++;
    result[result_size] = '\0';

    return result;
}

static void PrintEnv(FCGX_Stream *out, char *label, char **envp)
{
    FCGX_FPrintF(out, "%s:<br>\n<pre>\n", label);
    for( ; *envp != NULL; envp++) {
        FCGX_FPrintF(out, "%s\n", *envp);
    }
    FCGX_FPrintF(out, "</pre><p>\n");
}

int main(int argc, char **argv)
{
    char *json = read_file("queue.json");
    struct jzon *jzon = jzon_parse(json, NULL);
    free(json);

    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    int count = 0;

    while(FCGX_Accept(&in, &out, &err, &envp) >= 0) {
        char *content_length = FCGX_GetParam("CONTENT_LENGTH", envp);
        int len = 0;

        FCGX_FPrintF(out,
           "Content-type: text/html\r\n"
           "\r\n"
           "<title>FastCGI echo (fcgiapp version)</title>"
           "<h1>FastCGI echo (fcgiapp version)</h1>\n"
           "Request number %d,  Process ID: %d<p>\n", ++count, getpid());

        FCGX_FPrintF(out,
        "<table><tr>"
        "<td>%s</td><td>%s</td><td>%d</td><td>%d</td>"
        "</tr></table>\n",
        jzon_object_get(jzon, "queue_name", NULL)->string,
        jzon_object_get(jzon, "queue_id", NULL)->string,
        (int) jzon_object_get(jzon, "arrival_time", NULL)->number,
        (int) jzon_object_get(jzon, "message_size", NULL)->number);

        if (content_length != NULL)
            len = strtol(content_length, NULL, 10);

        if (len <= 0) {
            FCGX_FPrintF(out, "No data from standard input.<p>\n");
        } else {
            int i, ch;

            FCGX_FPrintF(out, "Standard input:<br>\n<pre>\n");
            for (i = 0; i < len; i++) {
                if ((ch = FCGX_GetChar(in)) < 0) {
                    FCGX_FPrintF(out,
                        "Error: Not enough bytes received on standard input<p>\n");
                    break;
                }
                FCGX_PutChar(ch, out);
            }
            FCGX_FPrintF(out, "\n</pre><p>\n");
        }

        PrintEnv(out, "Request environment", envp);
        PrintEnv(out, "Initial environment", environ);
    }

    return EXIT_SUCCESS;
}
