#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <jzon.h>
#include <ctemplate.h>

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

/*static void PrintEnv(FCGX_Stream *out, char *label, char **envp)
{
    FCGX_FPrintF(out, "<p>%s:</p>\n<pre>\n", label);
    for(; *envp != NULL; envp++) {
        FCGX_FPrintF(out, "%s\n", *envp);
    }
    FCGX_FPrintF(out, "</pre>\n");
}*/

static void print_row(FCGX_Stream *out, struct jzon *jzon) {
    const char *queue_id = jzon_object_get(jzon, "queue_id", NULL)->string;
    const char *queue_name = jzon_object_get(jzon, "queue_name", NULL)->string;
    const int message_size =
        (int) jzon_object_get(jzon, "message_size", NULL)->number;
    const int arrival_time =
        (int) jzon_object_get(jzon, "arrival_time", NULL)->number;
    const char *sender = jzon_object_get(jzon, "sender", NULL)->string;
    const struct jzon_array *recipients =
        jzon_object_get(jzon, "recipients", NULL)->array;

    FCGX_FPrintF(out,
        "<tr><td>%s</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td></tr>\n",
        queue_id, queue_name, message_size, arrival_time, sender);

    FCGX_FPrintF(out, "<tr><td colspan=\"5\"><ul>");

    for (int i = 0; i < recipients->capacity; i++) {
        struct jzon *recipient = recipients->elements[i];

        FCGX_FPrintF(out, "<li><strong>%s</strong> (%s)</li>",
            jzon_object_get(recipient, "address", NULL)->string,
            jzon_object_get(recipient, "delay_reason", NULL)->string);
    }

    FCGX_FPrintF(out, "</ul></td></tr>\n");
}

static size_t render_template(FCGX_Stream *out, const char *path,
    TMPL_varlist *varlist) {

    char *template = NULL;
    size_t template_size = 0;

    FILE *template_stream = open_memstream(&template, &template_size);

    TMPL_write(path, NULL, NULL, varlist, template_stream, NULL);

    fflush(template_stream);
    fclose(template_stream);

    FCGX_FPrintF(out, template);

    free(template);

    return template_size;
}

int main(int argc, char **argv)
{
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    int count = 0;

    const char *title = "Postfix Mail Queue";

    while(FCGX_Accept(&in, &out, &err, &envp) >= 0) {
        char *json = read_file("queue.json");
        struct jzon *jzon = jzon_parse(json, NULL);
        free(json);

        // char *content_length = FCGX_GetParam("CONTENT_LENGTH", envp);
        // int len = 0;

        FCGX_FPrintF(out, "Content-type: text/html\nStatus: 200\r\n\r\n");

        TMPL_varlist *varlist = TMPL_add_var(0, "title", title, 0);
        (void) render_template(out, "layout.tmpl", varlist);
        TMPL_free_varlist(varlist);

        FCGX_FPrintF(out, "<p>Request number %d,  Process ID: %d</p>\n",
           ++count, getpid());

        FCGX_FPrintF(out,
            "<p>\n"
            "<table class=\"table table-sm\">\n"
            "<thead><tr><th scope=\"col\">Queue ID</th><th scope=\"col\">Status</th><th scope=\"col\">Size</th><th scope=\"col\">Arrival Time</th><th scope=\"col\">Sender</th></tr></thead>\n"
            "<tbody>\n");

        print_row(out, jzon);

        FCGX_FPrintF(out,
            "</tbody>\n"
            "</table>\n"
            "</p>\n");

        /*if (content_length != NULL)
            len = strtol(content_length, NULL, 10);

        if (len <= 0) {
            FCGX_FPrintF(out, "<p>No data from standard input.</p>\n");
        } else {
            int i, ch;

            FCGX_FPrintF(out, "<p>Standard input:</p>\n<pre>\n");
            for (i = 0; i < len; i++) {
                if ((ch = FCGX_GetChar(in)) < 0) {
                    FCGX_FPrintF(out,
                        "Error: Not enough bytes received on standard input.\n");
                    break;
                }
                FCGX_PutChar(ch, out);
            }
            FCGX_FPrintF(out, "\n</pre>\n");
        }

        PrintEnv(out, "Request environment", envp);
        PrintEnv(out, "Initial environment", environ);*/

        FCGX_FPrintF(out,
            "</div>\n"
            "</body>\n"
            "</html>\n");

        jzon_free(jzon);
    }

    return EXIT_SUCCESS;
}
