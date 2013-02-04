#include <assert.h>

static int exitcode = -1;
#define TESTING
#include "snot.c"
#undef exit



static void test_die() {
    exitcode = -1;
    die("die test");
    assert(exitcode == 1);
}

static void test_remove_markup() {
    exitcode = -1;

    char str1[] = "";
    remove_markup(str1);
    assert(!strcmp(str1, ""));
   
    char str2[] = "no markup";
    remove_markup(str2);
    assert(!strcmp("no markup", str2));

    char str3[] = "normal <b>bold<\\b> normal";
    remove_markup(str3);
    assert(!strcmp("normal bold normal", str3));

    char str4[] = "<img src='foo'>bar</img>";
    remove_markup(str4);
    assert(!strcmp("bar", str4));

    char str5[] = "a>b<c";
    remove_markup(str5);
    assert(!strcmp("a>b<c", str5));

    char str6[] = "<<a>><<b>";
    remove_markup(str6);
    assert(!strcmp(">", str6));

    char str7[] = "<\n><\n>\n<\n>";
    remove_markup(str7);
    assert(!strcmp("\n", str7));

    assert(exitcode == -1);
}

static void test_remove_special() {
    exitcode = -1;

    char str1[] = "";
    remove_special(str1);
    assert(!strcmp(str1, ""));

    char str2[] = "foo\nbar\r";
    remove_special(str2);
    assert(!strcmp(str2, "foo bar "));
    
    char str3[] = "\n\r\0\b";
    remove_special(str3);
    assert(!strcmp(str3, "  "));

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
