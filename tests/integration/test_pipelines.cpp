#include <catch2/catch_test_macros.hpp>
#include <unistd.h>
#include <string>
#include <cctype>

extern "C" {
#include "command.h"
#include "simple_command.h"
}

static std::string run_and_capture(Command* cmd) {
    int p[2];
    REQUIRE(pipe(p) == 0);

    int saved = dup(STDOUT_FILENO);
    REQUIRE(saved >= 0);

    REQUIRE(dup2(p[1], STDOUT_FILENO) >= 0);
    close(p[1]);                

    cmd_execute(cmd);           

    dup2(saved, STDOUT_FILENO); 
    close(saved);

    char buf[512];
    ssize_t n = read(p[0], buf, sizeof buf);
    close(p[0]);

    return (n > 0) ? std::string(buf, buf + n) : std::string();
}

static std::string trim_ws(std::string s) {
    auto L = s.find_first_not_of(" \t\r\n");
    auto R = s.find_last_not_of(" \t\r\n");
    if (L == std::string::npos) return "";
    return s.substr(L, R - L + 1);
}

TEST_CASE("printf hello | wc -c [integration][pipes]") {
    Command cmd; cmd_init(&cmd);

    auto* a = static_cast<SimpleCommand*>(std::malloc(sizeof(SimpleCommand)));
    sc_init(a);
    sc_insert_arg(a, "/usr/bin/printf");
    sc_insert_arg(a, "hello");

    auto* b = static_cast<SimpleCommand*>(std::malloc(sizeof(SimpleCommand)));
    sc_init(b);
    sc_insert_arg(b, "/usr/bin/wc");
    sc_insert_arg(b, "-c");

    cmd_insert_sc(&cmd, a);
    cmd_insert_sc(&cmd, b);

    auto out = trim_ws(run_and_capture(&cmd));
    REQUIRE(out == "5");  

    cmd_clear(&cmd);
}