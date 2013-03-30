#include <cgreen/assertions.h>
#include <cgreen/mocks.h>
#include <cgreen/reporter.h>
#include <cgreen/suite.h>
#include <cgreen/internal/runner_platform.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "runner.h"

#ifdef __cplusplus
#include <stdexcept>

namespace cgreen {
#endif

static const char* CGREEN_PER_TEST_TIMEOUT_ENVIRONMENT_VARIABLE = "CGREEN_PER_TEST_TIMEOUT";

static void run_every_test(TestSuite *suite, TestReporter *reporter);
static void run_named_test(TestSuite *suite, const char *name, TestReporter *reporter);

static void run_test_in_the_current_process(TestSuite *suite, CgreenTest *test, TestReporter *reporter);

static int per_test_timeout_defined(void);
static int per_test_timeout_value(void);
static void validate_per_test_timeout_value(void);

int run_test_suite(TestSuite *suite, TestReporter *reporter) {
    int success;
    if (per_test_timeout_defined()) {
        validate_per_test_timeout_value();
    }

    setup_reporting(reporter);
    run_every_test(suite, reporter);
    success = (reporter->failures == 0) && (reporter->exceptions==0);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int run_single_test(TestSuite *suite, const char *name, TestReporter *reporter) {
    int success;
    if (per_test_timeout_defined()) {
        validate_per_test_timeout_value();
    }

    setup_reporting(reporter);
    run_named_test(suite, name, reporter);
    success = (reporter->failures == 0);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void run_every_test(TestSuite *suite, TestReporter *reporter) {
    int i;

    run_specified_test_if_child(suite,reporter);

    (*reporter->start_suite)(reporter, suite->name, count_tests(suite));
    for (i = 0; i < suite->size; i++) {
        if (suite->tests[i].type == test_function) {
            run_test_in_its_own_process(suite, suite->tests[i].Runnable.test, reporter);
        } else {
            (*suite->setup)();
            run_every_test(suite->tests[i].Runnable.suite, reporter);
            (*suite->teardown)();
        }
    }
    send_reporter_completion_notification(reporter);
    (*reporter->finish_suite)(reporter, suite->filename, suite->line);
}

static void run_named_test(TestSuite *suite, const char *name, TestReporter *reporter) {
    int i;
    (*reporter->start_suite)(reporter, suite->name, count_tests(suite));
    for (i = 0; i < suite->size; i++) {
        if (suite->tests[i].type == test_function) {
            if (strcmp(suite->tests[i].name, name) == 0) {
                run_test_in_the_current_process(suite, suite->tests[i].Runnable.test, reporter);
            }
        } else if (has_test(suite->tests[i].Runnable.suite, name)) {
            (*suite->setup)();
            run_named_test(suite->tests[i].Runnable.suite, name, reporter);
            (*suite->teardown)();
        }
    }
    send_reporter_completion_notification(reporter);
    (*reporter->finish_suite)(reporter, suite->filename, suite->line);
}


static void run_test_in_the_current_process(TestSuite *suite, CgreenTest *test, TestReporter *reporter) {
    (*reporter->start_test)(reporter, test->name);
    run_the_test_code(suite, test, reporter);
    send_reporter_completion_notification(reporter);
    (*reporter->finish_test)(reporter, test->filename, test->line);
}

static int per_test_timeout_defined() {
    return getenv(CGREEN_PER_TEST_TIMEOUT_ENVIRONMENT_VARIABLE) != NULL;
}

static int per_test_timeout_value() {
    char* timeout_string;
    int timeout_value;

    if (!per_test_timeout_defined()) {
        die("attempt to fetch undefined value for %s\n", CGREEN_PER_TEST_TIMEOUT_ENVIRONMENT_VARIABLE);
    }

    timeout_string = getenv(CGREEN_PER_TEST_TIMEOUT_ENVIRONMENT_VARIABLE);
    timeout_value = atoi(timeout_string);

    return timeout_value;
}

static void validate_per_test_timeout_value() {
    int timeout = per_test_timeout_value();

    if (timeout <= 0) {
        die("invalid value for %s environment variable: %d\n", CGREEN_PER_TEST_TIMEOUT_ENVIRONMENT_VARIABLE, timeout);
    }
}

static void run_setup_for(CgreenTest *spec) {
#ifdef __cplusplus
    va_list no_arguments;
    char message[255];
    TestReporter *reporter = get_test_reporter();

    memset(&no_arguments, 0, sizeof(va_list));

    try {
#endif
        spec->context->setup();
#ifdef __cplusplus
    } catch(std::exception& exception) {
        snprintf(message, sizeof(message) - 1, "an exception was thrown during setup: [%s]", exception.what());
        reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::exception* exception) {
        snprintf(message, sizeof(message) - 1, "an exception was thrown during setup: [%s]", exception->what());
        reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::string& exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during setup: [%s]", exception_message.c_str());

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::string *exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during setup: [%s]", exception_message->c_str());

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(const char *exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during setup: [%s]", exception_message);

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    }
#endif

}

static void run_teardown_for(CgreenTest *spec) {
#ifdef __cplusplus
    va_list no_arguments;
    char message[255];
    TestReporter *reporter = get_test_reporter();

    memset(&no_arguments, 0, sizeof(va_list));

    try {
#endif
        spec->context->teardown();
#ifdef __cplusplus
    } catch(std::exception& exception) {
        snprintf(message, sizeof(message) - 1, "an exception was thrown during teardown: [%s]", exception.what());
        reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::exception* exception) {
        snprintf(message, sizeof(message) - 1, "an exception was thrown during teardown: [%s]", exception->what());
        reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::string& exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during teardown: [%s]", exception_message.c_str());

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::string *exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during teardown: [%s]", exception_message->c_str());

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(const char *exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during teardown: [%s]", exception_message);

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    }
#endif

}


/**
   run()
   
   N.B. Although this is neither an API or public function, it is
   documented as a good place to put a breakpoint. Do not change the
   name or semantics of this function, it should continue to be very
   close to the test code. */
static void run(CgreenTest *spec) {
#ifdef __cplusplus
    va_list no_arguments;
    char message[255];
    TestReporter *reporter = get_test_reporter();

    memset(&no_arguments, 0, sizeof(va_list));

    try {
#endif
        spec->run();
#ifdef __cplusplus
    } catch(std::exception& exception) {
        snprintf(message, sizeof(message) - 1, "an exception was thrown during test: [%s]", exception.what());
        reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::exception* exception) {
        snprintf(message, sizeof(message) - 1, "an exception was thrown during test: [%s]", exception->what());
        reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::string& exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during test: [%s]", exception_message.c_str());

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(std::string *exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during test: [%s]", exception_message->c_str());

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    } catch(const char *exception_message) {
            snprintf(message, sizeof(message) - 1, "an exception was thrown during test: [%s]", exception_message);

            reporter->show_fail(reporter, spec->filename, spec->line, message, no_arguments);
    }
#endif

}

void run_the_test_code(TestSuite *suite, CgreenTest *spec, TestReporter *reporter) {
    significant_figures_for_assert_double_are(8);
    clear_mocks();

    if (per_test_timeout_defined()) {
        validate_per_test_timeout_value();

        die_in(per_test_timeout_value());
    }

    // for historical reasons the suite can have a setup
    if(has_setup(suite)) {
        (*suite->setup)();
    } else {
        if (spec->context->setup != NULL) {
            run_setup_for(spec);
        }
    }

    run(spec);

    // for historical reasons the suite can have a teardown
    if (suite->teardown != &do_nothing) {
        (*suite->teardown)();
    } else {
        if (spec->context->teardown != NULL) {
            run_teardown_for(spec);
        }
    }

    tally_mocks(reporter);
}

void die(const char *message, ...) {
    va_list arguments;
    va_start(arguments, message);
    vprintf(message, arguments);
    va_end(arguments);
    exit(EXIT_FAILURE);
}



#ifdef __cplusplus
} // namespace cgreen
#endif

/* vim: set ts=4 sw=4 et cindent: */
