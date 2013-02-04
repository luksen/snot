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

static void test_next_id() {
    exitcode = -1;

    int id = next_id();
    int id2 = next_id();
    assert(id2 > id);
    id = next_id();
    assert(id2 < id);

    assert(exitcode == -1);
}

static void test_config_parse_cmd() {
    exitcode = -1;
    config_init();
    char *argv1[7] = {"foo", "-r", "-1", "-f", "%a->%s", "-t", "5000"};
    config_parse_cmd(7, argv1);
    assert(config.raw);
    assert(config.single);
    assert(config.timeout == 5000);
    assert(!strcmp(config.format, "%a->%s"));
    assert(exitcode == -1);

    exitcode = -1;
    config_init();
    char *argv2[7] = {"foo", "--format", "%a->%s", "--raw", "--single", "--timeout", "5000"};
    config_parse_cmd(7, argv2);
    assert(config.raw);
    assert(config.single);
    assert(config.timeout == 5000);
    assert(!strcmp(config.format, "%a->%s"));
    assert(exitcode == -1);

    exitcode = -1;
    config_init();
    char *argv3[2] = {"foo", "-v"};
    config_parse_cmd(2, argv3);
    assert(exitcode == 0);

    exitcode = -1;
    config_init();
    char *argv4[2] = {"foo", "-f"};
    config_parse_cmd(2, argv4);
    assert(exitcode == 1);

    exitcode = -1;
    config_init();
    char *argv5[3] = {"foo", "-t", "-r"};
    config_parse_cmd(3, argv5);
    assert(exitcode == 1);

    exitcode = -1;
    config_init();
    char *argv6[3] = {"foo", "-t", "-1"};
    config_parse_cmd(3, argv6);
    assert(exitcode == 1);
}

int main(int argc, char **argv) {
    printf("Running tests...\n");
    test_die();
    test_remove_markup();
    test_remove_special();
    test_next_id();
    test_config_parse_cmd();
    return EXIT_SUCCESS;
}
