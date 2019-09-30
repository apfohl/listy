#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <jzon.h>
#include <ctemplate.h>
#include <libconfig.h>

void map_to_json_array(const char *line, size_t line_length, void **output)
{
    char **result = (char **)output;

    if (*result == NULL)
    {
        *result = malloc(3 * sizeof(char));
        strcpy(*result, "[]");
    }

    ssize_t output_length = strlen(*result);

    size_t new_size =
        1 +                           // [
        (output_length - 2) +         // existing data without brackets
        (output_length > 2 ? 1 : 0) + // comma, if array is not empty
        line_length +                 // new data
        1 +                           // ]
        1;                            // terminating null byte

    *result = realloc(*result, new_size * sizeof(char));

    sprintf(*result + 1 + output_length - 2, "%s%s]", output_length > 2 ? "," : "", line);
}

void read_lines(FILE *stream, void (*map)(const char *, size_t, void **), void **output)
{
    char *line = NULL;
    size_t line_length = 0;

    for (ssize_t nread = 0; nread != -1; nread = getline(&line, &line_length, stream))
    {
        if (nread == 0)
        {
            continue;
        }

        map(line, line_length, output);
    }

    free(line);
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

static char *render_layout(const char *path, const char *title, const char *content)
{

    TMPL_varlist *varlist = TMPL_add_var(NULL, "title", title, "content", content, 0);
    char *template = render_template(path, varlist);
    TMPL_free_varlist(varlist);

    return template;
}

static char *render_postqueue(struct jzon *jzon)
{

    TMPL_varlist *mainlist = NULL;
    TMPL_loop *queue_entries = NULL;

    const struct jzon_array *entries = jzon->array;

    for (int i = 0; i < entries->capacity; i++)
    {
        struct jzon *entry = entries->elements[i];

        char size[16];
        snprintf(size, 16, "%d", (int)jzon_object_get(entry, "message_size", NULL)->number);

        char arrival_time[16];
        snprintf(arrival_time, 16, "%d", (int)jzon_object_get(entry, "arrival_time", NULL)->number);

        // add information
        TMPL_varlist *varlist = NULL;
        varlist = TMPL_add_var(
            varlist,
            "queue_id", jzon_object_get(entry, "queue_id", NULL)->string,
            "status", jzon_object_get(entry, "queue_name", NULL)->string,
            "size", size,
            "arrival_time", arrival_time,
            "sender", jzon_object_get(entry, "sender", NULL)->string,
            0);

        // add recipients
        const struct jzon_array *recipients =
            jzon_object_get(entry, "recipients", NULL)->array;

        TMPL_loop *recipient_entries = NULL;

        for (int j = 0; j < recipients->capacity; j++)
        {
            struct jzon *recipient = recipients->elements[j];

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
    }

    // add queue to main list
    mainlist = TMPL_add_loop(mainlist, "queue_entries", queue_entries);

    // render template
    char *postqueue = render_template("templates/postqueue.tmpl", mainlist);

    // free varlist
    TMPL_free_varlist(mainlist);

    return postqueue;
}

char *get_path_of_config_file() {
    const char *config_files[] = {
        "/etc/panel.conf",
        "/usr/local/etc/panel.conf",
        "panel.conf"
    };

    char config_file[PATH_MAX];
    if (getcwd(config_file, PATH_MAX) == NULL) {
        return NULL;
    }

    sprintf(config_file + strlen(config_file), "/%s", config_files[2]);
    if (access(config_file, F_OK) == 0) {
        return strdup(config_file);
    }

    if (access(config_files[1], F_OK) == 0) {
        return strdup(config_files[1]);
    }

    if (access(config_files[0], F_OK) == 0) {
        return strdup(config_files[0]);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    FCGX_Stream *in, *out, *err;
    FCGX_ParamArray envp;
    config_t config;

    const char *title = "Postfix Mail Queue";

    config_init(&config);

    char *config_path = get_path_of_config_file();
    if (config_path == NULL) {
        perror("get_path_of_config_file");
        config_destroy(&config);

        return EXIT_FAILURE;
    }

    if(!config_read_file(&config, config_path))
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&config), config_error_line(&config),
            config_error_text(&config));
        config_destroy(&config);

        return EXIT_FAILURE;
    }

    free(config_path);

    const char *postqueue_command;
    if(!config_lookup_string(&config, "postqueue_command", &postqueue_command)) {
        fprintf(stderr, "No 'postqueue_command' setting in configuration file.\n");
        config_destroy(&config);

        return EXIT_FAILURE;
    }

    while (FCGX_Accept(&in, &out, &err, &envp) >= 0)
    {
        FILE *stream = popen(postqueue_command, "r");
        if (stream == NULL)
        {
            perror("popen");
            return EXIT_FAILURE;
        }

        char *json = NULL;
        read_lines(stream, map_to_json_array, (void **)&json);
        pclose(stream);

        struct jzon *jzon = jzon_parse(json, NULL);
        free(json);

        FCGX_FPrintF(out, "Content-type: text/html\nStatus: 200\r\n\r\n");

        char *postqueue = render_postqueue(jzon);
        char *layout = render_layout("templates/layout.tmpl", title, postqueue);

        FCGX_FPrintF(out, layout);

        free(layout);
        free(postqueue);
        jzon_free(jzon);
    }

    config_destroy(&config);

    return EXIT_SUCCESS;
}
