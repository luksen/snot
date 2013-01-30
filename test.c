#include <assert.h>

static int exitcode = -1;
#define TESTING
#include "snot.c"
#undef exit


#define BUFSIZE 100


static int compare_string(const char *a, const char *b) {
    if (strcmp(a, b)) {
        printf("expected %s got %s\n", a, b);
        return 0;
    }
    return 1;
}

static void test_die() {
    exitcode = -1;
    die("die test");
    assert(exitcode == 1);
}

static void test_remove_markup() {
    exitcode = -1;
    char string[BUFSIZE];

    strncpy(string, "", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string("", string));
   
    strncpy(string, "no markup", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string("no markup", string));

    strncpy(string, "normal <b>bold<\\b> normal", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string("normal bold normal", string));

    strncpy(string, "<img src='foo'>bar</img>", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string("bar", string));

    strncpy(string, "a>b<c", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string("a>b<c", string));

    strncpy(string, "<<a>><<b>", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string(">", string));

    strncpy(string, "<\n><\n>\n<\n>", BUFSIZE - 1);
    remove_markup(string);
    assert(compare_string("\n", string));

    assert(exitcode == -1);
}

static void test_remove_special() {
    exitcode = -1;
    char string[BUFSIZE];

    strncpy(string, "", BUFSIZE - 1);
    remove_special(string);
    assert(!strcmp(string, ""));

    strncpy(string, "foo\nbar\r", BUFSIZE - 1);
    remove_special(string);
    assert(!strcmp(string, "foo bar "));
    
    strncpy(string, "\n\r\0\b", BUFSIZE - 1);
    remove_special(string);
    assert(!strcmp(string, "  "));

    assert(exitcode == -1);
}

static void test_snot_id() {
    exitcode = -1;

    int id = snot_id();
    int id2 = snot_id();
    assert(id2 > id);
    id = snot_id();
    assert(id2 < id);

    assert(exitcode == -1);
}

static void test_snot_config_parse_cmd() {
    snot_config_init();
    
    char *argv[2] = {"foo", "-r"};
    snot_config_parse_cmd(2, argv);
    assert(config.raw);
}

int main(int argc, char **argv) {
    printf("Running tests...\n");
    test_die();
    test_remove_markup();
    test_remove_special();
    test_snot_id();
    test_snot_config_parse_cmd();
    return EXIT_SUCCESS;
}
