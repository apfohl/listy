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

char *read_file(const char *path)
{
    char *result = NULL;
    size_t result_size = 0;

    FILE *file = fopen(path, "r");

    char buffer[CHUNK];
    size_t nread = 0;

    while ((nread = fread(buffer, sizeof(char), CHUNK, file)) > 0)
    {
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
    FCGX_FPrintF(out, "<p>%s:</p>\n<pre>\n", label);
    for (; *envp != NULL; envp++)
    {
        FCGX_FPrintF(out, "%s\n", *envp);
    }
    FCGX_FPrintF(out, "</pre>\n");
}

static char *render_template(const char *path, TMPL_varlist *varlist)
{
    char *template = NULL;
    size_t template_size = 0;

    FILE *template_stream = open_memstream(&template, &template_size);

    TMPL_write(path, NULL, NULL, varlist, template_stream, NULL);

    fflush(template_stream);
    fclose(template_stream);

    return template;
}

static char *render_layout(const char *path,
                           const char *title,
                           const char *content)
{

    TMPL_varlist *varlist = TMPL_add_var(NULL, "title", title, "content",
                                         content, 0);
    char *template = render_template(path, varlist);
    TMPL_free_varlist(varlist);

    return template;
}

static char *render_postqueue(struct jzon *jzon)
{
    TMPL_varlist *varlist = NULL;
    TMPL_varlist *mainlist = NULL;
    TMPL_loop *queue_entries = NULL;
    TMPL_loop *recipient_entries = NULL;

    const struct jzon_array *recipients =
        jzon_object_get(jzon, "recipients", NULL)->array;

    char size[16];
    snprintf(size, 16, "%d", (int)jzon_object_get(jzon, "message_size", NULL)->number);

    char arrival_time[16];
    snprintf(arrival_time, 16, "%d", (int)jzon_object_get(jzon, "arrival_time", NULL)->number);

    // add information
    varlist = TMPL_add_var(
        varlist,
        "queue_id", jzon_object_get(jzon, "queue_id", NULL)->string,
        "status", jzon_object_get(jzon, "queue_name", NULL)->string,
        "size", size,
        "arrival_time", arrival_time,
        "sender", jzon_object_get(jzon, "sender", NULL)->string,
        0);

    // add recipients
    for (int i = 0; i < recipients->capacity; i++)
    {
        struct jzon *recipient = recipients->elements[i];

        TMPL_varlist *vl_recipient = TMPL_add_var(
            NULL,
            "address", jzon_object_get(recipient, "address", NULL)->string,
            "delay_reason", jzon_object_get(recipient, "delay_reason", NULL)->string,
            0);

        recipient_entries = TMPL_add_varlist(recipient_entries, vl_recipient);
    }

    // Add recipients
    varlist = TMPL_add_loop(varlist, "recipient_entries", recipient_entries);

    // add entry to queue
    queue_entries = TMPL_add_varlist(queue_entries, varlist);

    // add queue to main list
    mainlist = TMPL_add_loop(mainlist, "queue_entries", queue_entries);

    // render template
    char *postqueue = render_template("templates/postqueue.tmpl", mainlist);

    // free varlist
    TMPL_free_varlist(mainlist);

    return postqueue;
}

int main(int argc, char **argv)
{
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;

    const char *title = "Postfix Mail Queue";

    while (FCGX_Accept(&in, &out, &err, &envp) >= 0)
    {
        char *json = read_file("queue.json");
        struct jzon *jzon = jzon_parse(json, NULL);
        free(json);

        FCGX_FPrintF(out, "Content-type: text/html\nStatus: 200\r\n\r\n");

        char *postqueue = render_postqueue(jzon);

        char *layout =
            render_layout("templates/layout.tmpl", title, postqueue);
        FCGX_FPrintF(out, layout);

        free(layout);
        free(postqueue);

        PrintEnv(out, "Request environment", envp);
        PrintEnv(out, "Initial environment", environ);

        jzon_free(jzon);
    }

    return EXIT_SUCCESS;
}
